# Unisys ICON Series Administration Guide, Volumes 1–3 (ISSSW 3.00, July 1987)

Notes for the ICON/ICON2 research repo. Unisys part no. **1211042** for all three volumes. The three volumes form one continuously paginated book: Vol 1 = Sections 1–2, Vol 2 = Sections 3–10 + Appendices A–B, Vol 3 = Sections 11–17 + Appendix C. Each PDF repeats the full combined table of contents and its own glossary/index.

Short names used in citations:
- **AG1** = Administration Guide Vol 1 Planning (60 PDF pages)
- **AG2** = Administration Guide Vol 2 Configuration (174 PDF pages)
- **AG3** = Administration Guide Vol 3 Diagnostics (182 PDF pages)

Citations are "AGn p<PDF page> (<printed page>)".

**Overall verdict:** these are site-administrator guides, not technical references. There are **no on-disk structure layouts** (no superblock, directory entry or extent formats). What they do give: the **power-on diagnostic error codes and the beep encoding** (which match the dumped ROM's style), **floppy geometry** (512-byte sectors, 80 tracks × 2 sides × 8 or 10 sectors), the **hard-disk partition and drive numbering scheme**, the **bad block table** mechanism, file attribute letters, the **`/config` file inventory**, and a strong answer to "where is the node ID stored" (NVRAM, set by `attach_net`).

---

## 1. Filesystem-reader facts (priority 1)

### 1.1 Floppy format
- Media: "5-1/4 inch, double-sided, double-density diskette with 96 tracks per inch, and 80 tracks per side" (AG2 p62 (5-2); repeated AG3 p93 (14-17), p125 (15-11)).
- `fdformat` default: "**8 sectors per track** and a total storage capacity of **640 Kbytes**". Alternative: "**10 sectors per track** and a total storage capacity of 800K" (AG2 p63 (5-3)).
  - 80 trk × 2 hd × 8 spt × **512 B** = 655,360 B = 640 KB, so **sector = block = 512 bytes**. 10 spt gives 819,200 B = 800 KB.
- Command syntax (AG2 p64 (5-4)):
  ```
  $ fdformat 2                    (640K default)
  $ fdformat 2 t=80 n=10 h=2      (800K: t=tracks, n=sectors/track, h=heads)
  $ dinit 2                       (writes the bitmap; destroys data)
  $ dcheck 2 +m                   (bad-block scan, ~5 min)
  ```
  As each track is formatted, its number is displayed. `-p` suppresses prompts (AG2 p66 (5-6)).
- **A reader must handle both 8-spt (640K) and 10-spt (800K) images.** Interleave and skew are not documented here.
- Head-cleaning uses `fdformat` on a cleaning disk; the expected output is "`Track 45: Error 01.`" (AG2 p120 (9-6)). This hints at the format of fdformat's error message: per-track, with a 2-digit hex/decimal controller status.
- Workflow: "format, initialize, and check". Reformatting is rarely needed: "the quickest way [to clear a disk] is to re-initialize it with dinit" (AG2 p63 (5-3)). So **`dinit` = mkfs**, which writes the bitmap and root. Formatting is purely low-level.

### 1.2 Bitmap
- "Initialization puts a **bitmap** on the diskette; a bitmap keeps track of which parts of the diskette have been used" (AG2 p63 (5-3)). Glossary: "An address map that keeps track of which parts of a disk or diskette are used and which are not" (AG3 p155).
- `chkfsys` can **rebuild the bitmap**. It asks "do you want to rebuild the bitmap" (AG2 p80 (5-20)). So the bitmap is derived data that can be regenerated from the directory tree. A reader can cross-check it but should not need to trust it.
- **`zap` leaks blocks**: "When a file is zapped, the system does not list the newly-vacated space as free blocks" until `chkfsys` rebuilds the bitmap (AG2 p77–78 (5-17/18)). **Expect real images to have bitmap bits set for blocks no file owns.** That is normal, not corruption.

### 1.3 Hard disk: partitions and drive numbers
- "The QNX operating system cannot address any more than **32 Megabytes** of disk space. Thus, if you have a 64-Mbyte hard disk, it is divided into two sections, each with 32-Mbytes. These sections are called partitions … a **virtual disk**" (AG2 p75 (5-15)). A 10 MB disk has one partition.
- Drive numbering on a fileserver (AG2 p75–76 (5-15/16)):

  | Drive | Device |
  |---|---|
  | `1:` | hard disk partition 1 |
  | `2:` | fileserver diskette drive |
  | `3:` | hard disk partition 2 (64 MB fileserver only) |
  | `5:` | diskette drive of a **solo workstation** (AG2 p87–88 (6-7/8): `copy filename [19]5:/filename`; AG2 p120 (9-6): `fdformat [w/s_node_ID] 5`) |

  "Mounting means assigning a drive number to the virtual disk or diskette drive" (AG2 p75 (5-15)).
- RAM disks exist on workstations and "can be detected by typing: mount" (AG3 p20 (11-4)). The drive number is not stated here.
- The `bbt` utility addresses partitions as `p=1` and `p=2`, while the filesystem tools address drives `1` and `3` (AG2 p80 (5-20)). **So a physical partition table exists with at least 2 entries.** Its on-disk format is not given.
- `dinit [node_id]3 +hard_disk` initializes partition 2. It prompts twice for confirmation. Install of 3.00+ "automatically cleans up and initializes partition 1 … It does not … initialize partition 2" (AG2 p76 (5-16)).

### 1.4 Bad Block Table (important for a raw-image reader)
Verbatim (AG2 p76 (5-16)):
> "The Bad Block Table is a space **outside of your file system** on a safe area of the disk where the list of bad blocks are stored. Each bad block has another block, called a **replacement block** in this safe area. Any time you reference a bad block, the replacement block is used instead. This replacement is done by the operating system so that it is completely invisible [to] the system utilities."

**Implication:** on a raw hard-disk image, any filesystem block listed in the BBT must be read from its replacement block instead. A naïve reader will return garbage for those blocks. The BBT's location and format are not documented here. Look for them in the System Software Operations Guide or the Installation Guides, or reverse-engineer them from a hard-disk image.
- `bbt` "Displays the contents of or adds new bad blocks to the Bad Block Table" (AG2 p23 (3-7)). Usage: `[node] bbt p=1 <bbfile1` (AG2 p80 (5-20)).
- A file named **`bad_blocks`** exists and should be backed up along with `pass` and `net.config` (AG2 p124 (10-2)). It may be a text copy of the BBT.
- Maintenance procedure (AG2 p78–80 (5-18…5-20)):
  1. `dcheck [n]1 +mark b=bbfile1` scans the surface, marks bad blocks as used and appends them to a text file.
  2. `ws [n]1:/ "chattr @ a=+r" +d l=99` makes every directory readable.
  3. `chkfsys [n]1 b=bbfile1`: "determines the blocks used by each file, and whether any two files are trying to use the same blocks".
  4. A second `chkfsys` pass, answering y to rebuild the bitmap.
  5. `bbt p=1 <bbfile1`.

  Weekly maintenance also lists "`chkfsys` and **`clcheck`**" (AG2 p123 (10-1)). `clcheck` is otherwise unexplained, possibly a cluster check.

### 1.5 Blocks, sectors, names
- Glossary: a sector is a subdivision of a track (AG3 p163). The text uses "blocks or sectors" interchangeably (AG2 p62 (5-2)). Consistent with 512-byte blocks.
- House file names: "a name with **more than 16 characters**" is rejected, as are the characters `!@#$%^&*()+=-{}[]':;<>?,"` (AG3 p24 (11-8)). This is the House UI limit. The QNX filename limit may differ.
- Shell command lines are limited to 256 characters ("Line too long", AG2 p71 (5-11)).

### 1.6 File and directory attributes and permissions
`files +v` shows "Attr" columns and "G-Perm-O" columns (group and other permissions) (AG2 p83 (6-3)). The owner has attributes. Group and others have permissions. Others cannot do anything the owner's attributes forbid.

| Letter | Files (AG2 p83 (6-3)) | Directories (AG2 p84 (6-4)) |
|---|---|---|
| `r` | read/print/copy | read/list |
| `w` | write (overwrite) | — |
| `a` | append | — |
| `e` | execute | — |
| `c` | — | create files under it |
| `b` | — | **blocks directory searches below this directory** |
| `m` | modify attrs/perms/name/owner/group/date | same |

- Default new file: attributes **modify, append, write, read** (displayed `m-awr`), permissions **read**. Default new directory: attributes **modify, create, read**, permissions **read** (AG2 p84 (6-4)).
- `chattr` syntax: `a=+w`, `a=-wa`, `p=+r`, `n=newname` (rename: `chattr /config/pass.old n=pass.keep`, AG2 p59 (4-29)). **Rename is done by `chattr`, so the name is metadata on the same record as the attributes.**
- "Busy" files: "write-busy or read-busy" (AG3 p156 glossary), cleared with `zap`.
- Owner IDs are **group.member** byte pairs (see §4.2). This suggests the directory entry holds a 2-byte owner (group, member).

### 1.7 Backup and archive
- `backup src dest [options]`. Options: `d=date t=time l=nlevels p=npattern pd=npattern r=record_file +a -c +d +o -v -p` (`backup?` output, AG2 p22 (3-6)). `+a` all files, `+d` keep dates, `+o` keep owner, `d=25-09-87` incremental, `p=*.bas` pattern (AG2 p69–70 (5-9/10)).
- **`backup` copies files filesystem-to-filesystem** (`backup 3:/user 2:/user`). It is a tree copy onto a normal QNX-initialized diskette, **not a stream format**. Restore is the reverse `backup` (AG2 p70 (5-10)). So backup diskettes can be read with the ordinary FS reader.
- `arch` "Archives files to diskette(s)" (AG2 p23 (3-7)). Glossary: archive is "normally used with files that span more than one back up diskette" (AG3 p155). **`arch` is the multi-volume format.** Its layout is not documented here.
- `dcopy` "Copies an entire diskette to a second diskette" (AG2 p24 (3-8)).

---

## 2. Boot ROM, diagnostics, error codes (priority 2)

### 2.1 Beep encoding (AG3 Appendix C, p151–153 (C-1…C-3))
- When a power-on diagnostic fails, the error code is shown on screen (if one is attached) **and** beeped. The **fileserver's yellow front-panel LED flashes in the same pattern**.
- "A longer, lower tone meaning **0**, and a shorter, higher tone meaning **1** … Each four beeps … are combined into one hexadecimal digit … There are **two hex digits** in the error code, meaning that **eight beeps** are produced for each error message." The first four beeps are the high nibble. "Most error codes are repeated at least twice", separated by a longer pause.
- Table C-1 maps LLLL→0 … SHSHSHSH→F (plain binary, MSB first).
- **Success indicator:** "You should hear a **single tone at the end of the power-up diagnostics**" (AG3 p74 (13-26), p110 (14-34)).
- Fileserver "**yellow light … comes on and stays on**" means "the processor is hung, or not working" (AG3 p130 (15-16)). A yellow light that flashes during boot means a diagnostic failure (AG3 p124 (15-10)). AG3 p35 (12-5) calls the appendix "ICON/Fileserver Error Codes" and says to "translate the error message number into hex".

### 2.2 ICON (original) power-on error codes (AG3 p83–88 (14-7…14-12))
Normal status line: **"Diagnostic report complete, loading boot file."** This is close to the ROM's "Diagnostics report complete" (code 55).

Table 14-1 (verbatim, OCR-corrected):
```
17: Keyboard-no interrupt detected
19: Keyboard self test failure (invalid keycode)
1e: Line printer interrupt present on PPI B (pc0)
21: Cpu INT0 line will not clear (possible keyboard error)
24: Cpu INT3 line will not clear
57: Network failure or fileserver not operational
```
Also documented on the same pages (and in the fileserver table):
```
56: Floppy not inserted or no boot file on disk
    or  Floppy disk loader has returned with an error
```
Causes given:
- 17: keyboard did not power up correctly (usually transient).
- 19: faulty keyboard.
- 1e: parallel port, cable or printer fault. Test by unplugging the printer. "PPI B (pc0)" is 8255 #B, port C bit 0.
- 21: keyboard or board fault.
- 24: "something plugged into the **expansion connector on the right-hand side of the ICON**".
- 56: "If you are not trying to boot from a diskette, ignore this error". Cause can be "the **floppy loader program on the diskette** may be corrupted or missing".
- 57: cabling, terminators, or fileserver down.

### 2.3 Fileserver power-on error codes (AG3 p119–122 (15-5…15-8))
Table 15-1 (verbatim, OCR-corrected; "21e" in the OCR is "1e"):
```
1e: Line printer interrupt present on PPI [B]
56: Floppy not inserted or no boot file on disk
56: Floppy disk loader has returned with an error
57: Network failure or fileserver not operational
57: Network loader has returned with an error
58: Hard disk loader has returned with an error
```
- **58:** "The fileserver is having difficulties booting from the **boot file on its hard disk**". Fix: boot from a **boot floppy**, `mount` the hard disk, check free space, back up, then "reformat, reinitialize, check the disk, and reinstall the operating system".
- **57 on a fileserver:** for a *storage* fileserver, check that it is not "cut off from its **cluster fileserver**" (cabling, or Network Expander keyswitch on "Isolate"). **So storage fileservers network-boot from a cluster fileserver.** For a network or cluster fileserver, call service.
- "If you attach a monitor to the fileserver, the error message will display on the screen" (AG3 p124 (15-10)). The fileserver therefore runs the same ROM with video, which is consistent with "LEXICON = same board".

**Mapping to the dumped ROM ("5053 ISS 2", codes 09–5a):** the manual confirms the ROM's design. There are **three loader options with their own error codes: 56 = floppy loader, 57 = network loader, 58 = hard-disk loader**. The ROM's **55** ("Diagnostics report complete, no loader specified") and **5a** ("no specified loader option has succeeded") bracket them. The manual's status line "Diagnostic report complete, loading boot file" is the success variant. Codes 17/19/1e/21/24 fall in the self-test range below 55. The manual does **not** document code 11 (NVRAM) or 09, or how loader options are specified. They are probably NVRAM settings: see `set_boot_type` and `nv_config` in §2.5. No ROM monitor is mentioned anywhere.

### 2.4 ICON2 power-on error codes (AG3 p53–57 (13-5…13-9)): a different ROM
Normal status line: **"Requesting operating system from specified fileserver"**.

Table 13-1 (verbatim, OCR-corrected):
```
40: interface failure with primary keyboard
a1: SCSI bus RESET IN line active
fa: unable to reach node
fb: node did not send operating system
fc: received incomplete operating system
Cluster fileserver Node id not available
Please investigate and try again
```
- 40: the ICON2 keyboard fuse may have blown. The keyboard gets +5 V from the ICON2 and a shorted keyboard blows the fuse (AG3 p33–34 (12-3/4), p142 (17-4)).
- a1 (OCR "al"): "only if there is an **I/O Module** installed in your ICON2". This is the SCSI port (text says "60 pin" then "50 pin").
- fa: network unreachable (same fixes as ICON 57).
- fb: "accessed the network successfully but has not received the **boot file it requested** … Perhaps the workstation has attempted to boot from the **wrong fileserver**".
- fc: "received only part of the boot file … **too many workstations may be attempting to boot at the same time**".

**ICON2 codes (40, a1, fa–fc) are in a different numbering space from the ICON/fileserver codes (17–58).** So the ICON2 has its own boot ROM. The ICON2 boots from a **"specified fileserver"**, which implies a configured boot-server node ID, probably in NVRAM.

### 2.5 Node numbers, NVRAM and boot-type configuration
- **Node ID is set in software and persisted:** `attach_net +w` on a new workstation "attaches it to the network. The **newly-assigned node number** appears on your screen" (AG2 p93 (7-3)). `detach_net +r +w` "detaches the unit, **resets it to the factory ID**, and specifies that it is a workstation. It also deletes that unit's entry from the **net.config** file" (AG2 p98 (7-8)).
- **Factory/unattached node ID appears to be 254.** In the duplicate-node procedure, "If two node numbers are both **254**", run `attach_net +w` on each. Otherwise `detach_net +r` one and re-attach (AG3 p59–60 (13-11/12), p90–91 (14-14/15)).
- The node number is shown "on the Login screen directly above the copyright" (AG3 p44 (12-14)).
- `nv_config`: "Displays the workstation or fileserver configuration **as set by the NVRAM chip**" (AG2 p27 (3-11)). Glossary defines NVRAM (AG3 p161).
- `set_boot_type`: "Sets the type of workstation boot file" (AG2 p28 (3-12)).
- `make_solo_disk`: "Creates a **boot diskette for the solo ICON workstation**" (AG2 p27 (3-11)).
- `+w` on attach/detach appears to mean "workstation". `+r` appears to mean "reset to factory ID".
- **Conclusion for open question #1:** the node ID lives in **NVRAM** (the "32-byte EEPROM"), written by `attach_net`, with factory default 254. It is *not* fixed in EPROM. This is consistent with the ROM's "11: Missing or failed NVRAM". [JE2]'s "set at order time in EPROM" is probably wrong for 3.x-era units, or describes the factory default.
- `/config/net.config`: "A file which contains a record of node numbers used in the network" (AG3 p160 glossary). It must be backed up before reinstall (AG2 p74 (5-14)).
- Fileserver roles (AG2 p39 (4-13)):
  - **Network Fileserver**: holds the password information for the whole network and "other important information".
  - **Cluster fileservers**: "have groups or clusters of workstations that **boot off them**".
  - **Storage fileservers**: boot from their cluster fileserver (AG3 p122 (15-8)).
  - A one-server network: node 1 is both network and cluster fileserver. Users have a **cluster id** (default 1) in `/config/pass`.
- Fileserver ordering: turn off workstations, then cluster/storage fileservers, then the Network Fileserver. Power up the Network Fileserver first (AG2 p78 (5-18)).
- Virtual circuits: a hung network may mean "the fileserver may have **run out of virtual circuits**". Reboot the fileserver and then all stations (AG3 p72 (13-24), p108 (14-32)). `kill_vcs` "Kills all virtual circuits to a node" (AG2 p26 (3-10)).
- Date: the fileserver has a battery-backed RTC. `[1] date 19 February 1988 16 30`, then `[1] /sys/set_rtc`, then `[1] date | /sys/set_date` (AG2 p73 (5-13)). `/sys/read_rtc` also exists (AG3 p92 (14-16)). Workstations take their time from the fileserver at boot ("If all ICONs boot up with the wrong time, your fileserver has an adjustment problem").
- Printer daemon: a printer on the fileserver "should appear to adjust itself twice, once when the fileserver is turned on, and once when the operating system is almost loaded" (AG3 p137 (16-5)).

### 2.6 Network (ARCNET / ICONET)
- Token-passing network with "reconfiguration bursts". This is ARCNET semantics: "The yellow light comes on at the port where the reconfiguration burst is generated, and at the **highest node number** in the network" (AG3 p45–46 (12-15/16)).
- Network Expander: 4 I/O ports, each either **ICONET** or **ARCNET**. Green activity, red error and yellow reconfiguration LEDs per port (Table 12-1, AG3 p40 (12-10)). A keyswitch isolates the expander. "No terminators are required on unused ICONET or ARCNET connectors of the Network Expander" (AG3 p32 (12-2)). ICONET = the workstation daisy-chain segment, ARCNET = the inter-expander link.
- Cabling: a daisy chain, terminated at both ends, never a loop. Bayonet connectors with "petals" (BNC-style, AG3 p72–73).
- Limits without an expander: "maximum of **61 meters (200 feet)** of cable and **17 nodes** (e.g., 16 workstations and one fileserver). Each fileserver can support no more than 16 workstations within this distance" (AG1 p24 (2-2)).
- A faulty network interface can make a node "appear on the network as a different node than its assigned number or even as multiple nodes" (AG3 p33 (12-3)).
- `nacc` controls network access to cpu, devices (`$plpr`, `$con`) and drives. By default "the average user [cannot] read or write to the CPU of a workstation other than the one they are logged in on" (AG2 p86–87 (6-6/7)).

---

## 3. Hardware (priority 3)

### Physical specifications (AG1 p25 (2-3), Table 2-1) and power (AG1 p27 (2-5), Table 2-3)
| Unit | H × W × D | Weight | Power |
|---|---|---|---|
| ICON2 | 42 × 39.5 × 39.4 cm | 16 kg | 120 V, 2.5 A max |
| ICON2 keyboard | 3 × 56 × 28 cm | 3 kg | (from ICON2, fused) |
| ICON with amber monitor | 53 × 53 × 56 cm | 19.5 kg | 120 V, 3 A max |
| ICON with colour monitor | 58 × 53 × 63.5 cm | 25.5 kg | 120 V, 3 A max |
| Fileserver, 10 MB | 21 × 38 × 54 cm | 18 kg | 120 V, 3 A max |
| Fileserver, 64 MB | 21 × 38 × 54 cm | 19 kg | 120 V, 3 A max |
| Network Expander | 16 × 25 × 5 cm | 2.5 kg | 117 V, 0.25 A |

Environment: 13–35 °C operating, 10–80 % RH, altitude up to 2000 m (AG1 p29 (2-7)).

### Hardware facts scattered through the diagnostics
- **Fileserver models: 10 MB and 64 MB** hard disks.
  - The 10 MB drive needs `park` before moving ("moves the drive heads … to the innermost track"). After `park` the machine hangs and must be power-cycled. The 64 MB drive does not need parking (AG2 p99 (7-9)).
  - The fileserver has a diskette drive, a hard-disk activity LED (red), a power switch with a lamp, a rear pushbutton **circuit breaker**, a yellow status LED, parallel and serial printer ports, and an optional monitor (AG3 §15).
- **ICON (original):**
  - Separate monitor. Types are set with `/config/monitor`: `mb121a` (Roland monochrome), `cc141` (Roland colour), `colour` (KIMTRON or ICON colour). Reboot after changing (AG3 p100–102 (14-24/26)).
  - Integrated keyboard with CAP LOCK and CUR LOCK lighted keys. Trackball. Rear circuit breaker. Volume knob ("on older models slide the control knob").
  - **Expansion connector on the right-hand side**. Parallel port on 8255 "PPI B". Error 21/24 names CPU INT0/INT3, which are 80186 interrupt pins.
  - The ICON section has a "Diskette drive problems" group, covering solo ICONs with a drive (drive `5:`).
- **ICON2:**
  - Built-in colour monitor with a degaussing circuit. External monitor support. Separate keyboard/trackball unit with **DIP switches underneath** that must all be "ON" for the correct mode. Scroll Lock does nothing on the ICON2 (AG3 p145–146 (17-7/8)).
  - Keyboard LEDs flash at power-on. An internal keyboard fuse protects the +5 V feed. Green power LED. Optional **I/O Module with a SCSI port**.
  - The ICON2 can run an **ICON emulator** (`icon_emul_start`/`icon_emul_stop`). It has a Sound Administrator and an LPC speech administrator (`sound_start`, `speech_start`). `convert_phrase` converts old **TMS5220 "codepack"** speech data to "the new ICON speech format" (AG2 p24–29).
- Printers supported in 3.00: **Epson FX80** and **Panasonic 1091** (AG2 p101 (8-1)).
- Fileserver keyboard pipe key: the `|` key is "to the right of the Rubout key on the ICON computer and above the Enter key on the ICON2" (AG2 p73 (5-13)).

---

## 4. System directory tree, config files and utilities for 3.00 (priority 4)

### 4.1 Root directories (AG2 p18–19 (3-2/3))
The text says "13 main system directories". The AG3 glossary says 12 (p163). Listed:
`/boot` (booting programs), `/cmds` (general QNX commands: copy, date, fdformat…), `/config` (password file, init files for fileservers/workstations/printers), `/drivers` (hardware routines), `/fonts` (Editor fonts), `/icons`, `/sys` (network and graphics tasks e.g. `gtask`), `/tmp`, `/user`, `/util` (ICON-specific commands e.g. `kill_print`, `print`), `/user_cmds` (site-wide custom commands). Also referenced: `/spool/parallel`, `/spool/serial`, `/help/editor` (should hold **65 files**, AG3 p18–19 (11-2/3)).

### 4.2 Known files
| Path | Purpose | Cite |
|---|---|---|
| `/config/pass` | Password file, **plain text** (see below) | AG2 p58 (4-28) |
| `/config/pass.lock`, `pass.old`, `pass.new` | Lock and temp files left by `new_user` etc. | AG2 p59 (4-29) |
| `/config/net.config` | Node-number registry | AG2 p74, AG3 p160 |
| `/config/edit.config` | Editor icon menu config + trackball speed (1–9, 4 normal). Per-user copy in `~/.<something>/edit.config` | AG2 p85–86, AG3 p114/148 |
| `/config/monitor` | Executable: `monitor colour\|mb121a\|cc141` | AG3 p100–102 |
| `/config/user_ws_sys.init`, `/config/user_fs_sys.init` | Site hooks run at workstation/fileserver start-up (must be `+e`) | AG2 p74, p105 |
| `/config/u_sys.init.NNNN` | Per-node startup file, **NNNN = node number in 4-digit hex** (node 19 → `u_sys.init.0013`) | AG2 p105 (8-5) |
| `/config/print/PAT.fx80`, `PAT.p1091`, `TDN.*`, `SNP.*` | Printer templates | AG2 p102 (8-2) |
| `/config/print/pprt.init[.node]`, `sprt.init[.node]`, `tdon_p.init[.node]`, `snap_p.init[.node]`, `tdon_s…`, `snap_s…` | Generated printer config. `.node` = workstation node (decimal, e.g. `.19`) | AG2 p102–105 |
| `bad_blocks` | Back up with pass/net.config | AG2 p124 (10-2) |
| `/sys/ui`, `/sys/data_mgr`, `/sys/win_mgr`, `/sys/ed_wmgr` | House and Editor tasks | AG3 p21–26 |
| `/sys/set_rtc`, `/sys/read_rtc`, `/sys/set_date` | Clock utilities | AG2 p73, AG3 p92 |
| `/icons/ui_icons` | House icons | AG3 p22 |

**`/config/pass` format** (AG2 p58 (4-28)). One entry per user. The first line is not indented. The rest are indented by a tab or 4 spaces. Empty fields are blank lines. The 6th line is optional.
```
login name
    password
    user number (group.member)
    home directory          e.g. [1]1:/user/history/parks
    entry program           e.g. .login
    cluster number
```
The backup warning, "unauthorized users could use it to read the login and password of every other user" (AG2 p52 (4-22)), confirms plaintext passwords.

**User IDs** (AG2 p33 (4-3)):
- Regular users: group 1–254, member 1–254.
- Classroom Administrator: group 0–254, member **255**.
- Site Administrator: **255.255**.

Defaults shipped with 3.00: Site Administrator `root` / `superuser`, home `[1]1:/`, plus an `icon` login with no password (AG2 p31 (4-1), p37 (4-7)).

### 4.3 Utilities list (QNX release 3.00), AG2 p23–30 (3-7…3-14)
‡ = Site Administrator only; † = shell built-in.
- **‡ admin commands:** alive, apb, attach_net, attach_printer, bbt, chattr, chkfsys, delete_user, detach_net, detach_printer, dinit, fdformat, kill_list, kill_print, kill_slist, kill_sprint, make_solo_disk, mount, nacc, new_user, park, set_boot_type, zap.
- **† shell built-ins:** back, base, break, cd, defpipe, ec, else, endif, exit, goto, if, kill, ontty, path, pause, pri, prompt, shift, stype, then, trap, type, verbose.
- **Other commands:** apl, arch, backup, basic, cancel_macro, cat, cc, cco, cobol, config(WATFOR), convert_phrase, copy, date, dcheck, dcopy, decode_phrase, def_server ("task ID and node number of the global nameserver"), diff, drel, dump, echo, edit, encode_phrase, eo, files, font_ed, frel, gks_start/stop, help, hi_res (80-col gtask), house, icon_emul_start/stop, ied, kill_vcs, led, link, linkobj, list, locate, login, logo, logoff, lo_res (40-col), ls, mkdir, msort, naplps_start/stop, net, new_password, nv_config, p, pascal, print, pwd, qdos ("Starts the MS-DOS emulator (if DOS installed)"), qnx_tty_start/stop, query, say, search, set, set_colour, sh, silence, size, slice, slist, sort, sound_start/stop, speech_start/stop, split, spool, sprint, stop_data_mgr, stop_mmgr, stop_win_mgr, stty, talk, task, watfor, wc, wedit, who, ws, xlat.
- `mount` "Mounts a new disk drive or new file in the **memory library**" (AG2 p27). This is the QNX 2 resident-command library.

### 4.4 Version and release notes
- The login screen shows the OS version, e.g. "ISSSW Release 3.00" (AG2 p74 (5-14)).
- "The ICON Series System Software is updated about once a year." A reinstall preserves `/config/pass`, `net.config`, `/user_cmds`, `/config/print`, and `user_*_sys.init` (AG2 p73–74).
- From 3.00 on, the installer auto-initializes partition 1 but not partition 2 (AG2 p76).

---

## 5. Corrections and contradictions versus existing repo docs (priority 5)

1. **Node ID storage (ICON_web_research.md §1 and §7 Q1; ICON_boot_and_network.md header).** The manuals show the node number is **assigned in software by `attach_net`**, reset to a "factory ID" (evidently **254**) by `detach_net +r`, and readable with `nv_config` ("as set by the NVRAM chip"). This favours the NVRAM/EEPROM answer over [JE2]'s "stored in the EPROMs at order time".
2. **Fileserver hierarchy.** The repo describes one LEXICON per site, plus "second LEXICON = node 2". 3.00 defines **Network Fileserver / cluster fileserver / storage fileserver** roles. Storage fileservers network-boot from a cluster fileserver (error 57). Users carry a **cluster id**.
3. **Workstations per fileserver.** The repo cites "ICONs 16–47 (up to 32 per LEXICON)" [JE2]. AG1 p24 says **max 16 workstations per fileserver and 17 nodes / 61 m per cable segment** without a Network Expander. Both could hold (different eras or with expanders), but flag it.
4. **Drive numbers.** The repo lists 1 = hard disk, 2 = floppy, 8 = RAM disk. Add **3 = second 32 MB partition of a 64 MB fileserver** and **5 = solo-workstation floppy**. **Hard disks of 64 MB exist** (the repo implies 10 MB only, from the 1.13 book).
5. **Floppy capacity.** The repo says 640 KB (48 tpi → 320 KB). 3.00 adds an **800 KB, 10-sector format** (`fdformat 2 t=80 n=10 h=2`).
6. **Default accounts.** The repo (1.13 / web) lists `root`, `supervisor`, `teacher`, `icon`. 3.00 ships **`root`/`superuser`** and `icon`.
7. **User numbering.** ICON_commands.md says "owner 255 is reserved … leaves 254 user numbers". In 3.00 the owner is **group.member** (two bytes): regular 1–254.1–254, class admin g.255, site admin 255.255.
8. **Directory tree.** ICON_disk_layout.md §2 (1.13) shows `/lib`, `/mathlib`, `/languages`, `/speech`, `/demo`, `/rel_1.13_info`. The 3.00 tree adds **`/drivers`, `/fonts`, `/user_cmds`, `/help`**, and `/sys` gains `ui`, `data_mgr`, `win_mgr`, `ed_wmgr` and clock tools. `/config` gains `net.config`, `edit.config`, `monitor`, `print/`, `*_sys.init`.
9. **Boot diagram (ICON_boot_and_network.md §2).** Refine it: the ROM runs self-tests (codes < 55), then tries a configured **loader option: floppy (56), network (57) or hard disk (58)**. The fileserver itself boots from a "boot file on its hard disk", or from a boot floppy as a fallback. ICON2s use a separate ROM with codes 40/a1/fa/fb/fc and request the OS "from specified fileserver".

---

## 6. Gaps (not in these manuals)
- On-disk formats: superblock/root location, directory entry layout, extents, the partition table, BBT layout, the `arch` format, executable headers.
- Boot file names in `/boot`, the network boot protocol, NVRAM layout, jumpers, the ROM monitor, ROM codes 09–16 and 59. Look in the System Software Operations Guide (ISSSW 3.00), the Software Installation Guides (4.00/4.01) and the Network Planning Guide.
