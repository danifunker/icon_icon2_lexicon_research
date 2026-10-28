# Notes: System Software Operations Guide (ISSSW 3.00) and Operations Guide Vol. 2

Sources (source PDFs: YorkU `UNISYS_ICON/Manuals/`):

- **SSOG**: *UNISYS ICON Series – System Software Operations Guide*, relative to release I.S.S.S. 3.00, July 1987, form 1211109. 546 PDF pages. This is the real command reference. Section 9 is the full utility reference, Appendix A lists the utility error messages, and there is a glossary. Page mapping: printed **9-N = PDF 108+N**, **5-N = PDF 64+N**, **3-N = PDF 34+N**, **A-N = PDF 456+N**.
- **OG2**: *ICON Series Operations Guide Volume 2: Using the System*, July 1987. This is a **student and teacher tutorial** for the Editor (`edit`), the House (`house`) and basic QNX commands. It has almost no low-level technical content. Page mapping: **printed N = PDF N+16**.

Citations look like "SSOG p171 (9-63)". Neither manual covers the boot ROM, its self-test codes (09–5a), the ROM monitor or loader options. I searched both for "loader", "diagnost", "ROM", "EPROM" and "jumper", and the only hits are the disk "loader area" and the `dinit l=` loader file (see §1.6). Both manuals refer the reader to the *Administration Guide Vol. 3: Diagnostics* for error and diagnostic material (OG2 p19).

> **Version note.** SSOG p5 says it "details the QNX 3.00 operating system", and p111 (9-3) says "QNX release 3.00". That is the **Unisys ISSSW release number**. The underlying kernel is QNX **2.x**: the manual refers to "QNX 2.01" for the fdformat format table (p226–227) and "QNX 2.1 / 2.11 or later" for the bad block table and partitions (p464–465).

---

## 1. Filesystem facts (for the QNX 2 reader)

### 1.1 Blocks, files and names

| Fact | Source |
|---|---|
| Allocation unit = **one 512-byte sector ("block")**. "The smallest unit of storage allocation for a file is a 512-byte disk sector." Files are allocated "as contiguously as possible" and grow automatically | SSOG p36 (3-2); glossary "block: One sector on a disk or diskette containing 512 bytes" p505 |
| Max file size "about 32 million bytes" | SSOG p36 (3-2) |
| A file is an untyped byte sequence. Text lines end in the **ASCII record separator, hex 1E** (Enter is mapped to RS), not CR/LF | SSOG p36 (3-2), p28 (2-4) |
| Filenames: **1–16 chars**, `A–Z a–z 0–9 . _ $` (`$` not first, because a leading `$` means a device). Case-sensitive. `.` has no special meaning | SSOG p35 (3-1); chattr p170 (9-62) |
| `files +v` columns: **Blk** (blocks, 512 B), **X** (number of extents; contiguous = 1), **Loc** (starting block, **hex**), **Grp**, **Mem** (0–255), **Attr**, **Perm** (g and o), **Date**, **Time**, **Name** | SSOG p231 (9-123) |
| Real `files +v` output (OG2 p254–256, printed 238–240): `Blk X Loc Grp Mem Attr G-Perm-O Date Time Name` / `1 1 6a4a 001 002 meawr ----r ----r 27-Aug-87 12:05pm .login` / `1 1 f975 001 002 m-awr ----r ----r 27-Aug-87 2:10pm chart`. The attribute string is printed in the order **m e a w r**, and dates as `dd-Mon-yy`. Loc values up to `f975` (63861) appear on a home directory in `[1]1:` | OG2 p254, p256 |
| `files -used` "Lists those files that used to be in the directory (i.e., they have been erased)". **Deleted directory entries stay recoverable**, which matters for an undelete/forensic mode | SSOG p229 (9-121) |
| `files +modified` lists files modified since the last backup, using the per-file **modified** status bit | SSOG p229, p233 |

### 1.2 Directories

- A directory is a file. It is written only by privileged programs, and it can't share a name with a file in the same directory (SSOG p37, 3-3).
- **Max 682 entries per directory** (SSOG p37, 3-3; mkdir p308, 9-200). For the reader, 682 ≈ 32736/48 or ≈ 65536/96, so check this against the real entry size (the C manual should give `struct dir_entry`).
- `mkdir dir [size]` preallocates space for **10 entries** by default. Directories grow, but "may become fragmented if the initial size is not made large enough" (p307–308, 9-199/200). So **directories can have multiple extents**, just like files.
- `dinit p=/prefix` gives a disk's root a name other than `/`. That disk can then only be reached by its exact name, and `query` stops working "when it looks for /bitmap" (p208, 9-100). The `p=` option is for floppies only (A-21). **Root-name/prefix field in the root block or root directory.**

### 1.3 Status, attribute and permission bits

| Kind | Letters | Meaning | Source |
|---|---|---|---|
| File status (`chattr s=`) | **b** busy, **m** modified, **u** used | busy = open for write. It stays set in the directory entry after a crash. `s=-u` "is the equivalent of deleting the file with the zap command" | p168 (9-60) |
| File attr/perm | **r w a e m** | read, write, append, execute, modify | p45–46 (3-11/12), p169 |
| Directory attr/perm | **r c b m** (+ **d** directory, invisible) | read, create, block (no access below), modify, is-directory | p47 (3-13), p169, p232 |
| Three fields per entry | owner **attributes**, **group permissions**, **other permissions** | | p45 (3-11) |
| Defaults, new file | attr r w a m; perm r. Linker output also gets **e** in both | | p46 (3-12) |
| Defaults, new directory | attr r c m d; perm r d | | p47 (3-13) |
| Owner | **group.member**, each 0–255. **255.255 = Site Administrator**; **member 255 of group G = Classroom Administrator** for group G | | p49 (3-15) |

*Inference:* the m-e-a-w-r print order suggests a bit mask like r=0x01, w=0x02, a=0x04, e=0x08, m=0x10. Confirm this against the C manual's header file.

### 1.4 Bitmap, extents and on-disk structures named in error messages (Appendix A)

- "There is a file called **bitmap** on each disk which contains the sector allocation of the disk" (SSOG p37, 3-3). It shows up in `ls 2:/` as a normal-looking file; OG2 p272–273 tells students that `bitmap` "always appears on diskettes".
- `chkfsys` walks the tree, checks "the **directory entry** of each file and the **extents** that make up the file", builds a bitmap in memory and compares it with the one on disk (p172, 9-64).
- **`chkfsys` messages that name the on-disk fields** (SSOG p467–470, A-11..A-15). These are the most useful text in either manual for the reader:
  - `Unable to read first block of bitmap (BLK 2)` → **the bitmap starts at block 2.** QNX block numbering looks **1-based**, with block 1 as the root/format block (see 1.5).
  - `Corrupt extent header for bitmap.` → the bitmap file itself has an extent header.
  - `Bitmap on disk is larger / smaller than the mounted disk size`
  - Directory-entry fields: **`fnum_blks`** ("Number of blocks (fnum_blks) incorrect"), **`ffirst_xtnt` / `flast_xtnt`** ("First or last extent … is zero"; "Last extent pointer (flast_xtnt) incorrect"), **`fnum_xtnt`** ("Number of extents … incorrect").
  - Extent-header fields: **`prev_xtnt`, `next_xtnt`** (doubly linked list of extents), **`size_xtnt`** (must be ≠ 0), **`bound_xtnt`** ("Bound of extent inconsistent with size of extent"; "Bound … past end of disk"), plus "Non-zero extent extension" (a reserved field that must be 0) and "Corrupt extent header or bad input in b= file (extent block 0)".
  - `Corrupt disk root directory.`
  - Consistency rules the reader can apply: no block in two files; no file block marked free in the bitmap; bitmap blocks marked used but in no file = zapped files or bad blocks.
- The same names appear in the C manual (`long ffirst_xtnt; long flast_xtnt; unsigned fnum_xtnt;`, around line 10329 of its text), so **block pointers are 32-bit**.
- `zap` clears the directory entry **without freeing blocks**. `chkfsys` later reclaims them (p455, 9-347).
- `dinit` messages (p477–478, A-21/22): "write error … attempting to write out one of the **root block, bitmap or root directory**. These are the **first few blocks** of the disk"; "A disk must contain **at least 4 blocks**"; "Disk contains nn blocks (nn K) … each K is 2 blocks".
- Consistency check: the Book of ICON's freshly `dinit`ed 640K floppy shows **1276 free blocks**. 640K = 1280 blocks, so 1276 free means exactly **4 blocks of overhead**: block 1 (root/format block), block 2 (bitmap; 1280 bits = 160 bytes fits in one block), and probably 2 blocks of root directory. This last split is an inference.

### 1.5 Floppy formats (fdformat / mount)

SSOG p225–227 (9-117..9-119), p312–313 (9-204/205):

- Media are 5¼" double-density disks with **512-byte sectors**. The only valid values are **8, 9 or 10 sectors per track**, **1 or 2 heads**, and **40 or 80 tracks** (A-24: "These are the current standards supported for 5 1/4" diskettes with 512 byte sectors").
- **The default is 80 tracks × 2 heads × 8 sectors = 640K** (1280 blocks). `fdformat drive` with no options always gives 640K, whatever the mount says.
- Table 9-1, verbatim:

```
               Sectors/Track
Sides Tracks    8      9      10
  1    40     160K   180K   200K
  2    40     320K   360K   400K
  1    80     320K   360K   400K
  2    80    *640K   720K   800K
* default if no parameters are specified
```

- **Interleave:** `-stagger` "Suppresses the staggering of sectors. **Default stagger is 2**" (p225). The description then says "By staggering sectors by 3, a program has two sector times (about 12 msec) to process one sector" (p227). The manual contradicts itself, so read sector IDs from the image instead of assuming. For a reader working on logical images this only matters for flux or raw dumps.
- **The format table lives in block 1.** "The fdformat command places a table in **block 1** of the diskette, which describes how the diskette was physically formatted. This information always overrides that specified by the mount command" (p313, 9-205). `fdformat +Patch` "Writes the diskette size information indicated by h=, n=, and t= onto **block 1** of the diskette without modifying the rest" (p225). Disks formatted before **QNX 2.01** lack this table, and the mount defaults apply (p227). **The reader should parse heads, sectors and tracks from block 1 (the first 512 bytes) and fall back to the image size.**
- `dinit 3 k=320` → "320-Kbytes (640 blocks)" (p207).
- A 40-track disk can be read in an 80-track drive by double stepping, but not written (p226).
- The ICON workstation floppy driver `ws_floppy` is "double sided, double density, up to 800K capacity" (p18, 1-2).
- `make_solo_disk` must use **80-track** disks; heads and sectors don't matter (p305, 9-197).

### 1.6 Hard disk layout and partitions

- Hard disks are **10 MB or 64 MB** (driver `hd`; SSOG p18). "**10-Mbyte disks have only one partition; 64-Mbyte disks have two partitions**" (p313). The glossary says a 64-Mbyte disk "is divided into two partitions of 32 Mbytes each. Each partition works as a separate hard disk" (p511). Glossary "drive: a partition or division of the fileserver's hard disk" (p508).
- `bbt` (Bad Block Table; p149–150, 9-41/42; errors p463–465, A-7..A-9):
  - "If the bad blocks are in a cylinder, one of the physical disks, or the **first block or the bitmap of any of the partitions**, a warning…"
  - Error text: "Block … is in **Physical Block One, Bad Block Table or Loader Area** — DISK UNUSABLE". So the physical start of a hard disk holds **physical block 1 (partition record), the Bad Block Table and a loader area**, and then the partitions.
  - "Block … is in **logical block one** or the bitmap of partition n". Each partition has its own block 1 (root block) and bitmap.
  - "ERROR: 'p=' parameter not in range **1..4**", "must be 1 or 2 for QNX 2.1". The partition table format allows **4 entries**.
  - "ERROR: Disk does not contain Bad Block Table … If your hard disk was formatted in an earlier version of QNX [before **2.11**] you do not have a bad block table."
  - "ERROR: Unable to write out altered block one". Bad-block remapping is recorded in block one.
  - "Block … in cylinder one, which manufacturer usually guarantees to be defect free". The disk is cylinder-addressed, and `bbt +save` copies the contents to a **replacement block**: **the drive remaps bad blocks to spare blocks**. A raw image of a hard disk may therefore need BBT remapping applied.
  - Bad-block lists (`/tmp/bbfile`) are text files with one **hex** block number per line (p150, p172).
- `mount` errors (p486–489): "MOUNT: unable to read **master partition record**", "MOUNT: Bad partition entry", "MOUNT: unable to find partition".
- `mount s=` sizes are in 512-byte blocks unless suffixed k/K, m/M or T (tracks per platter, i.e. cylinders); "Tracks are the preferred method of specifying an offset and size" (p310–311). `o=nnT` and `s=nnT` need h= and n= (A-31). There are also driver type numbers ("d= … must lie within 1 to 15", A-31, probably a different field than the d= drive option).
- `dcopy +hard … so= do=` (offset in blocks) can split a whole hard disk across several floppies as raw images. `dcopy` copies **up to 112 blocks** per pass (p189–191, 9-81..83). **Raw-sector images made this way may turn up among archived disks.** (dcopy p190 says "1 block = 500 bytes", which is a typo for 512.)
- `dinit +hard` initializes a hard disk. `dinit +suppress` "rewrites the bitmap, not the root directory" after "a disaster that has destroyed the first few blocks" (p207–209).
- `dinit l=<ICON_loader_file>` "allows you to put a **loader file** on a diskette so that you can make a boot diskette for a solo workstation". The example is `dinit 2 l=/sys/FDos_ldr`; the OCR reads "I= /sysjFDos_ldr", so the exact name is uncertain. `DINIT: Loader can not be specified for RAM disk` (A-22). **Bootable solo floppies carry a loader in reserved blocks, so expect non-file data near the start of solo boot disks.**
- `park` (10 MB disk only) moves the heads to the innermost track and hangs the machine. It must run on the fileserver as `[1] park` (p337, 9-229).
- Fileserver RAM disk cache: `/sys/set_cache` "controls the RAM-based hard disk cache on a fileserver" (p20). `mount cache`/`xcache` are listed but marked "Not available" (p309).

### 1.7 Drive numbers (logical drives 1–8)

`mount`, `fdformat` and `dcopy` all accept **drives 1–8** (A-24, A-31).

| Drive | Meaning | Source |
|---|---|---|
| **1** | Hard disk (first partition) on a fileserver | mount p310 |
| **2** | Fileserver floppy | mount p310 |
| **3** | **Second partition of the 64 MB hard disk** (examples: "chkfsys 3 … checks partition 2 of the hard disk (disk 3)"; "dcheck [1]3 … disk 3 (partition 2)") | p310, p171, p185 |
| **5** | Workstation's own floppy (`mount disk 5 /drivers/ws_floppy`) | p310–312 |
| **8** | RAM disk, e.g. `mount ramdisk 8 s=128k` (needs `set_boot_type +fsys` on ICONs and 512K ICON2s) | p311–312 |
| Workstation view | At boot, `mount remdisk 1 n=1 d=1`, `remdisk 2 n=1 d=2` and `remdisk 3 n=1 d=3` are "done automatically during system initialization". A workstation sees the fileserver's drives as its own 1, 2 and 3 | p311–312 |
| Default search order | Workstation "drive 1 then drive 3"; fileserver "drive 1". "When you boot over the network, this search order includes the remote node you booted from" | search p363 (9-255) |

Path syntax is `[node]drive:/path`. Node 0 or no node means the local node (p52–53, 3-18/19). Search orders can chain to other nodes (`search 3 [4]`, `search [14] +remote` to redirect to a backup fileserver), with at most 2 levels of redirection (p54–55).

### 1.8 Archive and backup formats

- **`backup`** is a file-level copy between two QNX filesystems (no container format). Options: `+all`, `-create`, `+date`, `d=dd-mm-yy`, `t=hh:mm:ss`, `l=levels`, `+owner`, `-prompting`, `p=`/`pd=` patterns (with `^` to exclude), `r=record_file` (logs the date, time, files and a disk sequence number prompted per diskette), `-verbose` (p141–145, 9-33..37). **The backup floppies are ordinary dinit'ed QNX disks.**
- **`arch`** is a different, **non-QNX-filesystem multi-volume archive format** (p131–135, 9-23..27):
  - `arch drive create [nfiles]` creates an archive directory on the first diskette. nfiles defaults to **128** and is rounded up to a **multiple of 8**. "Each archive has a limit of **816 entries**."
  - Saved files are **appended** to the end of the archive, which "may span many diskettes". Earlier versions are kept and addressed by **index**. `restore` by name takes the most recent "saved date".
  - "Archived diskettes are **not accessible with commands such as ls**", so they have no QNX directory/bitmap and are a raw format. The full stored pathname is restored unless output is redirected.
  - **A RustyBackup reader that finds a floppy without a valid QNX root/bitmap may be looking at an `arch` volume.** Directory slots of a fixed size, a multiple of 8, are a likely signature (inference).
- **`dcopy`** produces sector-exact copies, including partial copies with `so=`/`do=` offsets (above).
- `talk` has a block-send file-transfer protocol over serial: SOH/STX framing, checksums, sequence numbers, NAK/ACK (A-42..A-45). It is not a disk format.

### 1.9 File types and headers

These manuals say little about this.

- Executables get the **e** attribute from the linker. A text file with **e** is run as a shell command file (p232).
- `mount lib` loads shared libraries. The error "**Invalid header for shared library** — The first few bytes of a shared library are not in the correct format" and "bad load module — not in the proper format" (A-33) show that load modules and shared libs have **magic headers**. The format is not described here; see the C Compiler manual (other reader).
- Disk drivers (`/drivers/hd`, `fd`, `ws_floppy`, `ramdisk`) are **loadable driver files** passed to `mount`.
- Object files use a `.o` suffix (`eo` example "Releases all files ending in .o", p221). `cc`, `cco`, `link` and `linkobj` exist only "if C Compiler installed".
- Pipes are temp files `/tmp/..pipexxxxyyyy1`/`2` (xxxx = node, yyyy = task ID) (p74, 5-10).
- Printer init files in `/config/print` are text with `comment:data` lines (p107–108, sample below).

---

## 2. Boot, network and NVRAM

- **`/boot`** (SSOG p18, 1-2): "various boot files used to load (boot) the operating system … Examples … **`ws_boot`, `hd_boot`, and `fs_net_boot`**, for booting up a workstation, a fileserver from a hard disk, and a storage fileserver, respectively."
- **`/sys/bootsnd`, `/sys/bootrec`**: "Tasks that run on the fileserver to **transmit the operating system to workstations during bootup**" (p19, 1-3).
- **`/sys/poll`** (the poller) monitors the network, releases a dead node's resources, and tells every node which nodes are up, down or busy (p20). `alive` (p121–125) is the interface to it. The poller marks a node down after **3 unanswered tries**. **`/config/sys.init` contains `alive +n`** ("+newboot") for each node.
- **`set_boot_type ±fsys`** (p367, 9-259): "When a workstation is turned on, a file referred to as the **boot file**, which contains the operating system, is obtained from the fileserver and loaded into the workstation's memory." The normal workstation boot file has a **reduced File System Administrator**, which can't mount a local floppy or RAM disk. `+fsys` selects a boot file with the full one, at the cost of memory. It "has no effect on a **1-Mbyte workstation**". This setting is per workstation; where it is stored (NVRAM or net.config) isn't stated.
- **NVRAM.** **This answers open question 1** (where the node ID is stored).
  - `attach_net` (p137–138, 9-29/30): "Workstations are shipped from the factory with **preset unit node numbers** … The preset unit node number is changed to a unique network node number using the attach_net command. This unique number is **stored in the non-volatile RAM** so that it is not lost when the workstation is turned off." The unit must be rebooted afterwards.
  - `nv_config` (p333, 9-225): "A non-volatile random access memory (NVRAM) chip informs the operating system of a unit's **hardware configuration** after the unit is turned on … Its **serial number and booting information** are also displayed."
  - mount errors (A-32): "the amount of memory specified in the NVRAM is automatically mounted". **NVRAM also records installed memory size.**
  - This matches the ROM diagnostic "11 : Missing or failed NVRAM". The NVRAM holds node number, hardware configuration (memory), serial number and boot info, so the ROM needs it to boot.
- **Node numbering** (attach_net p138, detach_net p199, alive A-1):

| Item | Value |
|---|---|
| Network Fileserver | **node 1** always (glossary p510); holds the master `/config/pass`; can't be detached |
| Workstation valid nodes | **16–255, except 245** |
| Fileserver valid nodes | **1–245** |
| Factory/default fileserver node | **245** (`[245] attach_net +s`) |
| Default/reset workstation node | **254** (`detach_net +reset`) |
| Max nodes per network | **128** |
| `alive` valid range | 1–254 |
| Automatic assignment | `attach_net` without `n=` picks the lowest free node from **`[1]1:/config/net.config`** |

- **Fileserver roles.** **Network Fileserver** (node 1, master password file); **cluster fileserver** (workstations boot from it; `attach_net +cluster`, `c=cluster_node_id`, and `new_user c=` records which cluster a user boots off); **storage fileserver** (`+storage`, booted with `fs_net_boot`). A workstation boots off node 1 unless `c=` is given. `login` searches the password file on the boot cluster first, then the Network Fileserver (p295). `a=`/`e=` options on `alive` let **clusters share one physical network** without interfering (p125).
- **System tasks at boot** (p60–62, 4-2..4-4): Task Administrator (priority 1), Device Admin (2), File System Admin (4), Idle (15), Network Administrator, and **Disk Administrator (fileserver only)**. Extra tasks: Snap, Timer, Speech (ICON), LPC Speech (ICON2), ICON Emulator (ICON2), print Daemon (fileserver). The nameserver is TASK_ADMIN (task ID 0001) on node 1 (`def_server`, p195). A tick is **1/20 s** (`slice`, p379).
- **Remote execution.** `[4] task` runs on node 4; `[4]task` loads from node 4 and runs locally; `[1] sh` gives a virtual console on node 1 (p72, 5-8). It needs Site Administrator rights or `nacc cpu +w` on the target. By default after boot, network access to local disks and devices is denied and CPU write is allowed for remote creates (nacc table p320, partly garbled). Site Administrators always have access.
- `kill_vcs node` kills virtual circuits to a crashed node (p265). `net` shows per-node ttys, ports, CPU flags, **QNX version**, file count, free memory, tasks and virtual circuits (p325–326). `task +i` shows the maximum number of tasks, workstations and ports, the timeslice, the nameserver and "approximate speed of the processor" (p435).
- The shell variable `#n` is the node number in hex. Example: `type #n` prints `001e`, i.e. node 30 (p78, 5-14).
- Restart key **CTRL-SHIFT-RESET** restarts QNX on an ICON. Open files are left busy. **Not available on the ICON2** (p29, 2-5).
- The **solo ICON** boots from a floppy made by **`/config/make_solo_disk`**, a shell script that copies selected `/cmds` etc. to `#1:/cmds/...`. It is equivalent to `dinit … l=<loader>` (p305, p208–209).

---

## 3. Hardware facts

- Models: **ICON** and **ICON2** workstations; the fileserver is "a box". There are **512K ICON2s** and **1-Mbyte workstations** (p311–312, p367). These manuals never use the name "LEXICON"; they say "fileserver" throughout.
- **Keyboards** (SSOG p25–29; OG2 p135–139). ICON: **CUR LOCK** key with a red light (above HELP) turns the keypad into cursor keys; the keypad has Paginate, Text Begin/End Line and Move Pointer keys. ICON2: PC-style **Num Lock** with a green light. The ICON2 has a slot above the keyboard for a function-key label strip (OG2 p20). Figures 2-1/2-2 show both keyboards (images only).
- **Serial and parallel devices** (p51, 3-17):

| Device | Meaning | tty | Default |
|---|---|---|---|
| `$con` | keyboard and display | $tty0 | |
| `$slpr` | serial printer, **RS-232-1** | $tty1 on the **ICON** | 1200 baud, 7 bits, 2 stop, odd |
| `$sec` | "future expansion", **RS-422** | $tty1 on the **ICON2** | 9600, 7, 2, odd |
| `$mdm` | sync/async comms, **RS-232-2** | $tty2 | 9600, 7, 2, odd |
| `$plpr` | parallel printer (fileserver, or workstation printer option) | $tty3 | |
| `$null` | null device | none | |
| `$tel` | created by the NAPLPS decoder | | |

- Each fileserver supports up to **two printers**: one Centronics/Epson parallel and one RS-232 serial (p105, 8-3). Workstations need a "printer port (communications) option" to print directly (p106).
- **Speech:** the ICON uses a **TI TMS5220** LPC chip (`say`, `silence`, `encode_phrase`/`decode_phrase` with 12 frame parameters; `convert_phrase` converts old "codepack" data to the "new ICON speech format", p179, p193–194). The ICON2 uses `lpc_admin` and a Sound Administrator (`sound_start`, ICON2 only).
- **Display:** `gtask` decodes **TELIDON**/PDI and `naplps.sys` decodes NAPLPS (x resolution 640 or 320; 1, 2 or 4 bits per pixel; p321). `set_colour` needs "1 × 640 × 240 or 2 × 320 × 240" modes (A-37). There is a 16-colour palette (p369), 80-column (`hi_res`) and 40-column (`lo_res`) text, and screen snaps with **Ctrl-Shift-8 / Ctrl-\***, sent to the printer at 640 dots / 80 dpi (p103).
- `mmgr` is a virtual-memory manager for programs over 64K (p20). The ICON emulator runs ICON applications on the ICON2 (p249).
- Fileserver floppy drive (OG2 p267–273): students put the disk in with the label facing left and turn the latch down. The fileserver is "shaped like a box".

---

## 4. System tree and config files (ISSSW 3.00)

"When the system software is installed, the fileserver contains **11 main directories**" (SSOG p18, 1-2): **/boot, /cmds, /config, /drivers, /fonts, /icons, /sys, /tmp, /user, /user_cmds, /util**. /spool is also used (created by `attach_printer`, p140).

| Path | Contents (SSOG p18–23) |
|---|---|
| `/boot` | `ws_boot`, `hd_boot`, `fs_net_boot` |
| `/cmds` | Quantum QNX commands (copy, date, fdformat, ...) |
| `/util` | Unisys/CEMCORP-specific commands (edit, house, print, new_user, delete_user, ...) |
| `/user_cmds` | site-written commands |
| `/config` | `pass` (password file), monitor configuration utilities, printer support files, **`sys.init`** (shell file run at boot; contains `alive +n`, mounts, daemons), **`net.config`** (node registry used by attach_net/detach_net), **`make_solo_disk`**, `print/` |
| `/config/print` | `pprt.init[.node]`, `tdon_p.init[.node]` (parallel: print/list), `sprt.init[.node]`, `tdon_s.init[.node]` (serial: sprint/slist), `snap_p.init`/`snap_s.init` (snap spool path), and templates `PAT.<printer>`, `TON.<printer>`, `SNP.<printer>` such as `pprt.init.PAT.fx80` (p139–140, p103–108) |
| `/drivers` | `ws_floppy`, `hd`, `fd`, `ramdisk`. The mount examples use `/config/ws_floppy` and `/config/hd`, which is inconsistent with this |
| `/fonts` | editor fonts; apl fonts |
| `/icons` | system icons (House etc.) |
| `/sys` | `bootsnd`, `bootrec`, `daemon`, `ed_help`, `ed_wmgr`, `gtask`, `lpc_admin`, `mmgr`, `naplps.sys`, `poll`, `print_file`, `set_cache`, `snap_admin`, `speech`, `sysint`, `win_mgr`, `win_mgr2` |
| `/tmp` | temp files; `/tmp/bbfile` (bad-block list) and pipe files |
| `/user/<login>` | home directories; `.login` file |
| `/spool/parallel`, `/spool/serial` | print spool directories (need the create attribute) |

The password file has one entry per user: login name, password, user number, home directory, entry command and (optionally) cluster number, with continuation lines indented (new_user f= format, p330). Logins are up to 16 characters and passwords up to 20 (p327, p330).

Sample `/config/print/pprt.init.PAT.fx80` (SSOG p107–108, verbatim, all decimal):
```
A description of the printers supported by the file: EPSON FX.80
The character that causes a form feed: 12
The default character width for printing (chars/line): 80
The default line spacing for printing (lines/inch): 6
...
Width #1 (chars/line): 80
Byte sequence for this character width: 20 18
Width #2 (chars/line): 132
Byte sequence for this character width: 20 15
End of width table: 0
Spacing #1: 6
Byte sequence for this spacing: 27 50
Spacing #2: 8
Byte sequence for this spacing: 27 48
End of spacing table: 0
Enable emphasized printing: 27 69
...
```

Prompts: `$` Site Administrator, `#` Classroom Administrator, `%` user (p66, 5-2). Keyboard modes: **ICON mode** (the default at login) and **QNX mode** (`qnx_tty_start`; needed for QDOS and French characters; "All new applications should use QNX mode", p26). Type-ahead/recall buffer is 254 characters (p30).

---

## 5. Corrections and contradictions to the existing repo docs

1. **The node ID is in NVRAM**, written by `attach_net` (SSOG p137–138; `nv_config` p333). This settles ICON_web_research §7 Q1 and the ICON_boot_and_network update box ("Eckert says the EPROMs; the book lists a 32-byte EEPROM"): it is the non-volatile RAM (the book's 32-byte EEPROM), **not the EPROMs and not jumpers**. Factory defaults: ws = 254, fs = 245.
2. **The node ranges are wider than "ICONs are nodes 16–47"** (ICON_boot_and_network.md). Workstations may be 16–255 (not 245), fileservers 1–245, and a network has at most 128 nodes. 16–47 may have been a site convention.
3. **More than one fileserver is possible**: Network (node 1), cluster and storage fileservers. Workstations can boot off a cluster fileserver other than node 1. The repo assumes a single LEXICON.
4. **Drive numbering** (ICON_disk_layout.md §1 lists only 1: and 2:): add **3: = second 32 MB partition of a 64 MB disk**, **5: = workstation floppy**, **8: = RAM disk**. Workstations mount the fileserver's 1–3 as remdisks at boot. **Hard disks are 10 MB or 64 MB (2 × 32 MB partitions)**, not only 10 MB.
5. The "about 512-byte blocks" inference in ICON_disk_layout.md is **confirmed: exactly 512 bytes**. The 1276 free blocks after dinit = 1280 − 4 overhead blocks.
6. **Renamed or replaced commands in ISSSW 3.00 compared with the 1.13 book:** `tasks` → **`task`**; `wmi` → **`pwd`**; `ui` → **`house`** (`vcd` is not listed); `attach`/`detach` → **`attach_net`/`detach_net`** (+ `attach_printer`/`detach_printer`); `rmt_park` gone, now `[1] park`, 10 MB disks only. `ed` is not in the 3.00 list; the line editor is **`led`**. The TOC entry "ed 5-15" is OCR for the `cd` builtin. `clearscreen`, `wait_for_char`, `natal`, `fged`, `fed`, `sysback`, `rmtshell` and `logo.français` are **not** in the 3.00 utility list; they may be add-ons or gone. `dcheck` checks for **bad blocks** (the repo says "check disk integrity"); filesystem integrity is **`chkfsys`**.
7. `files +d` lists **directories** (the repo says "with dates"). Dates are always shown; `+d` = `+directory` (p229).
8. `chattr` owner change: the repo's `chattr /user/name o=N` is wrong for 3.00. Use **`g=` and `m=`** (group and member, 0–255). The administrator is **255.255**, which refines the book's "owner 255". Member 255 of any group is that group's Classroom Administrator. User numbers are written `g.m` (e.g. 2.21).
9. The `/boot` contents question (ICON_disk_layout.md, ICON_web_research §7 Q4) is partly answered: `ws_boot`, `hd_boot`, `fs_net_boot`. The download is done by `/sys/bootsnd` + `bootrec`.
10. The 3.00 root tree has **no /lib, /mathlib, /languages, /speech, /demo or /sampler** in the official list. Those come from the 1.13 book. New in 3.00: `/drivers`, `/fonts`, `/user_cmds`, `/tmp`.
11. Line terminator: QNX text uses **RS (0x1E)**, not CR or LF. Worth adding to any text-export feature.

---

## 6. Command and utility inventory (SSOG Section 9 + Section 5 builtins)

Legend. **Adm** = ‡ Site-Administrator-only; the OCR renders the dagger marks as `+`, `:j:`, `:t:` or `:f:`, so double-check a few. **Sh** = shell builtin (Section 5). **New?** = not in ICON_commands.md (✱ = appears in ICON_web_research §3 only). PDF page = 108 + N for printed 9-N. "(opt)" = sold separately ("may not be bundled").

| Command | Purpose | Printed / PDF | Adm | New? |
|---|---|---|---|---|
| alive | Tell poller or node which nodes are up, down or busy; `+n` at boot | 9-13 / p121 | ‡ | new |
| apb | All-points bulletin to every screen (max 63 chars) | 9-19 / p127 | ‡ | ✱ |
| apl | WATCOM APL interpreter (opt) | 9-21 / p129 | | new |
| arch | Multi-diskette append-only archive: create, directory, save, restore | 9-23 / p131 | | new |
| attach_net | Assign a node number in NVRAM; +workstation/+cluster/+storage, n=, c= | 9-29 / p137 | ‡ | new (replaces `attach`) |
| attach_printer | Create /config/print init files, spool directory, daemon | 9-31 / p139 | ‡ | new |
| back | Toggle display of background task ID | 5-14 / p78 | Sh | new |
| backup | Copy trees by date or pattern (+a +d +o, d= t= l= p= pd= r=) | 9-33 / p141 | | have |
| base | Shell variable display base 16/10 | 5-14 / p78 | Sh | new |
| basic | WATCOM BASIC | 9-39 / p147 | | have |
| bbt | Show or add entries in the hard-disk Bad Block Table | 9-41 / p149 | ‡ | new |
| break | Send a break to a task | 9-43 / p151 | Sh | new |
| cancel_macro | Stop a NAPLPS macro | 9-45 / p153 | | new |
| cat | Concatenate files | 9-47 / p155 | | have |
| cc | Compile and link C (opt) | 9-49 / p157 | | new |
| cco | Compile C without linking (opt) | 9-55 / p163 | | new |
| cd | Change directory (`^` = up) | 9-57 / p165 | Sh | have |
| chattr | Change status (b/m/u), attributes, permissions, owner (g= m=), name (n=), date | 9-59 / p167 | ‡ | have (fix o=) |
| chkfsys | Filesystem consistency check; rebuild bitmap | 9-63 / p171 | ‡ | new |
| cobol | WATCOM COBOL (opt) | 9-67 / p175 | | new |
| config77 | WATFOR-77 configuration (opt) | 9-69 / p177 | | new |
| convert_phrase | Old TMS5220 "codepack" → new ICON speech format | 9-71 / p179 | | new |
| copy | Copy files or devices | 9-73 / p181 | | have |
| date | Show or set date and time | 9-75 / p183 | | have |
| dcheck | Read-test every block for bad blocks; +m marks them in the bitmap | 9-77 / p185 | | have (purpose corrected) |
| dcopy | Sector copy of a whole disk (so=/do= offsets, +hard) | 9-81 / p189 | | new |
| decode_phrase | TMS5220 data → ASCII frame parameters | 9-85 / p193 | | new |
| defpipe | Set the pipe temp-file path | 5-15 / p79 | Sh | new |
| def_server | Set the global nameserver (node, task ID) | 9-87 / p195 | | new |
| delete_user | Remove users from /config/pass (Network and cluster FS) | 9-89 / p197 | ‡ | have |
| detach_net | Remove a node; +reset → ws 254 / fs 245 | 9-91 / p199 | ‡ | new (replaces `detach`) |
| detach_printer | Remove printer files, spool dir and daemon | 9-93 / p201 | ‡ | new |
| diff | Compare files | 9-95 / p203 | | have |
| dinit | Initialize a QNX filesystem (+hard, b=, l=loader, k=, p=/prefix, +suppress) | 9-99 / p207 | ‡ | have |
| drel | Remove an empty directory | 9-103 / p211 | | have |
| dump | Hex + ASCII dump, 16 bytes per line | 9-105 / p213 | | new |
| ec | Shell takes input from a file (like `source`) | 5-15 / p79 | Sh | new |
| echo | Echo arguments | 9-107 / p215 | | have |
| edit | Icon-driven full-screen Editor | 9-109 / p217 | | have |
| else / endif / if / then / goto | Shell flow control (`if +f/+d/+m/+a node`, lt/ge/eq/ne) | 5-15..5-20 / p79–84 | Sh | new |
| encode_phrase | ASCII frame parameters → TMS5220 data | 9-111 / p219 | | new |
| eo | Execute a command on a list of files (`@` substitution) | 9-113 / p221 | | new |
| exit | Exit the shell with a status (hex) | 5-16 / p80 | Sh | new |
| fdformat | Format a floppy (h= n= t=, -stagger, +Patch block 1) | 9-117 / p225 | ‡ | have |
| files | Recursive listing; +v shows Blk/X/Loc/Grp/Mem/Attr/Perm; -used shows erased entries | 9-121 / p229 | | have |
| font_ed | Font Editor | 9-127 / p235 | | have |
| frel | Delete files | 9-129 / p237 | | have |
| gks_start / gks_stop | Start or stop the WATCOM GKS graphics task | 9-131/9-133 / p239/241 | | new |
| help | Online help, same text as the manual (opt) | 9-135 / p243 | | ✱ |
| hi_res | gtask 80-column mode | 9-137 / p245 | | have |
| house | Icon-driven House shell | 9-139 / p247 | | new (book's `ui`) |
| icon_emul_start / icon_emul_stop | ICON emulator on ICON2 | 9-141/9-143 / p249/251 | | new |
| ied | Icon editor | 9-145 / p253 | | have |
| kill | Kill a task | 9-147 / p255 | Sh | have |
| kill_list / kill_print / kill_slist / kill_sprint | Kill the current print job (ws parallel / fs parallel / ws serial / fs serial) | 9-149..9-155 / p257–263 | ‡ | new |
| kill_vcs | Kill virtual circuits to a crashed node | 9-157 / p265 | | new |
| led | Line editor | 9-159 / p267 | | new (replaces `ed`?) |
| link / linkobj | Link objects (with C libraries) (opt) | 9-169/9-173 / p277/281 | | new |
| list | Print on a workstation-attached parallel printer | 9-177 / p285 | | have |
| locate | Find patterns in files | 9-183 / p291 | | have |
| login | New login (+stack); searches cluster then network pass | 9-187 / p295 | | have |
| logo | Logo (opt) | 9-189 / p297 | | have |
| logoff | Log off (-z) | 9-191 / p299 | | have (as CTRL-d) |
| lo_res | gtask 40-column mode | 9-193 / p301 | | have |
| ls | List names (directories marked +) | 9-195 / p303 | | have |
| make_solo_disk | /config script: build a solo-ICON boot floppy (80-track) | 9-197 / p305 | ‡ | new |
| mkdir | Make a directory [initial entries, default 10] | 9-199 / p307 | | have |
| mount | Mount disk/ramdisk/remdisk/lib; list mounts | 9-201 / p309 | ‡ | new |
| msort | Merge and sort files | 9-207 / p315 | | new |
| nacc | Network access control for drives, devices, CPU | 9-211 / p319 | ‡ | new |
| naplps_start / naplps_stop | NAPLPS decoder on $tel | 9-213/9-215 / p321/323 | | new |
| net | Per-node resources (memory, tasks, VCs, QNX version) | 9-217 / p325 | | new |
| new_password | Change password (Network + cluster pass) | 9-219 / p327 | | ✱ |
| new_user | Add users (c= cluster, f= batch, g=, l= entry program, u= base dir) | 9-221 / p329 | ‡ | have |
| nv_config | Show NVRAM configuration (hardware, serial number, boot info) | 9-225 / p333 | | new |
| ontty | Create a task on another tty | 5-18 / p82 | Sh | new |
| p | Display a file (+raw) | 9-227 / p335 | | have |
| park | Park 10 MB disk heads and hang (run as `[1] park`) | 9-229 / p337 | ‡ | have |
| pascal | WATCOM Pascal (opt) | 9-231 / p339 | | have |
| path | Command search path (`!dir1!dir2!!`) | 9-233 / p341 | Sh | new |
| pause | Wait for Enter | 5-19 / p83 | Sh | new |
| pri | Priority for later commands | 9-235 / p343 | Sh | new |
| print | Spool to the fileserver parallel printer | 9-237 / p345 | | have |
| prompt | Show tty number in the prompt | 5-19 / p83 | Sh | new |
| pwd | Print current [node]drive:/path | 9-243 / p351 | | ✱ (book's `wmi`) |
| qdos | MS-DOS emulator (a=–d= drives, m= memory, default 128K) (opt) | 9-245 / p353 | | new |
| qnx_tty_start / qnx_tty_stop | QNX vs ICON keyboard mode | 9-247/9-249 / p355/357 | | new |
| query | Free space from the bitmap (+d graphic map) | 9-251 / p359 | | have |
| say | Speak TMS5220 files | 9-253 / p361 | | ✱ |
| search | Local/remote drive search order | 9-255 / p363 | | new |
| set | WATFOR-77 environment task (opt) | 9-257 / p365 | | new |
| set_boot_type | Workstation boot file with or without full FSA (±fsys) | 9-259 / p367 | ‡ | new |
| set_colour | Screen colours (16-colour palette) | 9-261 / p369 | | new |
| sh | Execute shell commands / new shell (`[1] sh`) | 9-263 / p371 | | new |
| shift | Shift arguments | 5-19 / p83 | Sh | new |
| silence | Reset the speech chip | 9-267 / p375 | | new |
| size | Character count | 9-269 / p377 | | have |
| slice | Timeslice in 1/20-s ticks (1–255) | 9-271 / p379 | | new |
| slist | Print on a workstation-attached serial printer | 9-273 / p381 | | have |
| sort | Sort | 9-279 / p387 | | have |
| sound_start / sound_stop | ICON2 Sound Administrator | 9-283/9-285 / p391/393 | | new |
| speech_start / speech_stop | ICON2 LPC Administrator | 9-287/9-289 / p395/397 | | new |
| split | Split a file | 9-291 / p399 | | new |
| spool | WATCOM print spooler (opt) | 9-293 / p401 | | new |
| sprint | Spool to the fileserver serial printer | 9-295 / p403 | | have |
| stop_data_mgr / stop_mmgr / stop_win_mgr | Stop stuck data manager / memory manager / window manager | 9-301..9-305 / p409–413 | | new |
| stty | Terminal and serial options (baud, bits, parity, edit, ...) | 9-307 / p415 | | new |
| stype | Type without newline | 9-313 / p421 | Sh | new |
| talk | Terminal emulator with block-send file transfer over $mdm | 9-315 / p423 | | new |
| task | List tasks (+i +m +c +s +v +net +registered, n=node) | 9-325 / p433 | | ✱ (book's `tasks`) |
| trap | Trap keyboard breaks | 5-20 / p84 | Sh | new |
| type | Print arguments | 9-329 / p437 | Sh | have |
| verbose | Shell verbose mode | 5-21 / p85 | Sh | new |
| watfor77 | WATFOR-77 compiler (opt) | 9-331 / p439 | | new |
| wc | Count characters, words, lines | 9-333 / p441 | | new |
| wedit | WATCOM full-screen editor | 9-335 / p443 | | new |
| who | Who is on this workstation / node / net (`who net`) | 9-337 / p445 | | new |
| ws | Walk a tree and run a command per file (date/time filters) | 9-339 / p447 | | new |
| xlat | Translate characters | 9-345 / p453 | | new |
| zap | Remove a busy or damaged file's directory entry without freeing its blocks | 9-347 / p455 | ‡ | new |

Help shortcut: `cmd?` prints usage, e.g. `backup?` → `use: backup src_directory dest_directory [options]*` (p111). Options can be shortened to their bold letters (`+date` → `+d`). Options with `*` can repeat.

**OG2 "Commands Learned"** (OG2 p336–339 and p361–363, printed 320–323 / 345–347): cat, cd (`^`, `^^`), chattr (a=/pg=/po=/n=), date, drel, edit, files +v [p=], frel (with `*`), `cmd?`, help, ls, mkdir, new_password, p, print, pwd, sort [+r], split (`split f1 f2 s=size <big`), wc, who, who net. Keys: Ctrl-k / Ctrl-j recall (ICON mode), Ctrl-x, Ctrl-c, Pause, Ins, Del. Every one of these is in the table above. OG2 adds nothing new apart from the beginner-level `house`/`edit` tutorials. Home directories appear as `[1]1:/user/heather`.

---

## 7. Gaps (not in these two manuals)

- No ROM, self-test code, ROM monitor or loader-option documentation. No ARCNET hardware details (address switches, jumpers). Try *Administration Guide Vol. 3: Diagnostics* and the *Network Planning Guide*.
- No byte-level layouts. The block-1 format table, root block, directory entry size and extent header layout need the C Compiler Programming Reference (structs `ffirst_xtnt`, ...) or the disk images.
- Contents of `ws_boot`/`hd_boot`/`fs_net_boot`, the `net.config` format, and the full `sys.init` text are not given; see *Admin Guide Vol. 2: Configuration*.
