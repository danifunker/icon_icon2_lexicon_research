# Developer manuals: C Compiler + Developer's Toolkit (ISSSW 3.00)

Sources covered (short names used in citations):

- **CC**: *ICON Series C Compiler Programming Reference Manual*, ISSSW 3.00, Nov 1987, part 1211117 (719 PDF pages). Printed page = PDF page minus about 112 in Section 9 (e.g. 9-205 is PDF p317).
- **TK1**: *Developer's Toolkit Operations Guide Vol 1: The Window Manager*, ISSSW 3.00, June 1988, part 1212917 (443 PDF pages).
- **TK2**: *Developer's Toolkit Operations Guide Vol 2: Utilities*, ISSSW 3.00, June 1988, part 1212917 (381 PDF pages).

The OCR mangles `FILE` into `ALE` and `entry` into `enby`. Both are corrected silently below.

---

## 0. Summary for the QNX 2 filesystem reader

The C manual gives exactly one on-disk structure verbatim: **`struct dir_entry`** from `/lib/fsys.h`. It also gives the flag bits, the date format, the block-numbering rule and the floppy geometry fields of the long disk entry. Other structures are named but **not printed**: `struct ldisk_entry`, `struct deb`, `struct r_w_msg`, `struct xtnt`, the bitmap, and the load-file header. They live in `/lib/fsys.h`, `/lib/io.h` and `/lib/task.h` on a real system disk. **Pulling `/lib/fsys.h`, `/lib/io.h`, `/lib/dev.h` and `/lib/task.h` out of a 3.x disk image is the single best next step.** These are plain-text header files.

### 0.1 `struct dir_entry`, verbatim (CC p317–318, printed 9-205/9-206)

> "The structure of a directory entry is contained in the file /lib/fsys.h, which is duplicated below."

```c
struct dir_entry {
    char fstat;
    long ffirst_xtnt;
    long flast_xtnt;
    long fnum_blks;
    unsigned fnum_xtnt;
    char fowner;
    char fgroup;
    char fspare[6];
    char ftype;
    char fgperms;
    char fperms;
    char fattr;
    unsigned fdate[2];
    char fname[NCPFN + 1];
    };

fstat bit definitions:
#define _FILE_MODIFIED   0x10
#define _USED            0x40
#define _FILE_BUSY       0x80

fattr, fperms, and fgperms bit definitions:
#define _READ            0x01
#define _WRITE           0x02
#define _APPEND          0x04
#define _CREATE          0x04
#define _EXECUTE         0x08
#define _BLOCK           0x08
#define _MODIFY          0x10
#define _DIRECTORY       0x20
```

**Types:** char is 1 byte, int/unsigned 2 bytes and long 4 bytes. Multi-byte values are little-endian (Intel). A long is stored as word0 (low) then word1 (high) (CC p52, Fig 4-1).

**`NCPFN`** (the number of characters per filename) is not defined in the text. Filenames are "1 to 16 characters" (System Software Ops Guide; also CC p593 and the QDOS guide), so **NCPFN = 16 and `fname` is 17 bytes, NUL-terminated**.

**Offsets, derived by me and assuming no padding** (the manual says nothing about alignment):

| Off | Size | Field | Notes |
|---|---|---|---|
| 0x00 | 1 | fstat | 0x40 = entry in use, 0x80 = file busy/open, 0x10 = modified since backup |
| 0x01 | 4 | ffirst_xtnt | Block number of the first extent (or the first extent header block, see below) |
| 0x05 | 4 | flast_xtnt | Block number of the last extent |
| 0x09 | 4 | fnum_blks | File size in 512-byte blocks |
| 0x0D | 2 | fnum_xtnt | Number of extents |
| 0x0F | 1 | fowner | User number (255 = administrator) |
| 0x10 | 1 | fgroup | Group number |
| 0x11 | 6 | fspare[6] | |
| 0x17 | 1 | ftype | File type (values not given) |
| 0x18 | 1 | fgperms | Group permissions |
| 0x19 | 1 | fperms | Other/public permissions? (owner vs other not stated) |
| 0x1A | 1 | fattr | Attribute bits; 0x20 = directory |
| 0x1B | 4 | fdate[2] | QNX date format, see §0.3 |
| 0x1F | 17 | fname[17] | |
| = 0x30 | | | **48 bytes packed** |

**Caution.** 48 does not divide 512 evenly (10.67 per block). Either the compiler pads, the structure has more fields than the manual shows, or entries do not pack 512/48. Check a real directory block for a 48-byte or 64-byte stride. A 64-byte stride would mean 16 bytes are unaccounted for: NCPFN = 32 would give exactly 64, but that contradicts the 16-character filename limit. Treat the offsets above as a hypothesis to test against an image.

**Semantics** (CC p469–470, set_dir_entry):

- "Fields which relate to block numbers and extents may never be changed. The name, date, attributes and permissions of the file may always be changed."
- The fgroup field can only be changed by a Site Administrator, and fowner only by a Group Leader of the same group. "The USED and FILE_BUSY status bits may only be changed by a privileged task."
- The example clears `_FILE_MODIFIED` to mark a file as backed up. So fstat 0x10 is the archive bit that `backup` uses.
- Renaming through set_dir_entry does not check for duplicate names in the directory.
- "fsys.h … includes the contents of the only structured file in QNX - a directory" (CC p98). So **a directory is a file made of `dir_entry` records**, and fattr `_DIRECTORY` (0x20) marks a directory. The bit pairs `_APPEND`/`_CREATE` (0x04) and `_EXECUTE`/`_BLOCK` (0x08) share values. My reading is that the first name applies to files and the second to directories. That is an inference.
- `access()` uses **different, UNIX-style bits**: 0x04 = read, 0x02 = write, 0x01 = execute, 0x00 = exists (CC p121). Do not confuse these with the fperms bits.

**Extents.** The dir entry keeps the first and last extent block numbers plus an extent count, so files are chains of extents. The extent header format (`struct xtnt`) is **not** given in these manuals.

**How a directory is referenced at run time** (CC p552, task_info Table 9-2). A task's current directory is stored as "Current Directory Disk ID / Node ID / **Block (low word) / Index / Block (high word)** / Drive Number". So the OS addresses a directory entry as a **32-bit block number plus an index within the block**.

### 0.2 Raw disk access API (CC p197–208, printed 9-85…9-96)

- `struct deb *disk_open(unsigned nid, unsigned drive)` takes a node and a QNX drive number. It returns a disk control block (`deb`, defined in `/lib/fsys.h`), allocated with calloc, and opens any virtual circuits needed. "Disk drives do not have to be on the same CPU as the task making the open request, and the drive may have been adopted by some administrator other than the File System."
- `int disk_read_block(dp, long block, struct r_w_msg *buffer)` and `disk_write_block(...)`. The buffer is `char buf[sizeof(struct r_w_msg) + 512]` and the data lands at `buffer.r_w_data[0]`. "512 bytes of data are read … This call does not use the QNX file structure. **Blocks are numbered starting at one.** The number of blocks on a disk is device dependent but no more than 2^31-1." Only privileged tasks may call these.
  - **For the reader: QNX block N is at byte offset (N−1)×512 in a linear image.**
- `int disk_get_entry(dp, struct ldisk_entry *buf)` "returns the long disk entry … information such as the type of disk (diskette, hard disk, or RAMdisk), the size of the disk, the number of sectors per track, etc." See `/lib/io.h` and `/lib/fsys.h` for "disk_entries and adopt entries".
- `int disk_set_entry(dp, struct ldisk_entry *buf)` remounts a drive. Verbatim example (CC p205):
  ```c
  dp = disk_open(My_nid, 2);
  stat = disk_get_entry(dp, &buf);
  buf.num_sctrs = 640;   /* remount as 320k floppy */
  buf.sctr_cyl = 16;
  buf.sctr_trk = 8;
  set_disk_entry(dp, &buf);
  ```
  > "useful for remounting disks as single or double-sided or as containing 8 or 9 sectors per track … **QNX diskettes are automatically remounted when accessed; however, accessing data on non-QNX diskettes may require remounting**"
  - Field names in `ldisk_entry`: **`num_sctrs`, `sctr_cyl`, `sctr_trk`** (plus disk type and size).
  - **Floppy geometry this implies:** a 320K diskette has 640 sectors of 512 bytes, 8 sectors per track, 16 sectors per cylinder (2 heads), so 40 cylinders. A 640K diskette is therefore 1280 blocks: 80 cyl × 2 heads × 8 spt × 512 B. This matches the repo's "1276 free blocks after dinit 2", which leaves 4 blocks of overhead. The 9-sectors-per-track mode is for non-QNX (i.e. DOS) disks, so **native QNX floppies here are 8 sectors per track of 512 bytes**.

### 0.3 QNX date format (CC p315–316, printed 9-203, Figure 9-4)

`get_date(unsigned date[2])`. The `fdate[2]` field in the dir entry is `unsigned[2]`, almost certainly in the same format.

```
date[0]: bits 15-12 YYYY  year - 1980 (0-15)
         bits 11-8  MMMM  month (1-12)
         bit  7     X     (unused)
         bits 6-1   DDDDDD date (1-31)
         bit  0     P     0 = am, 1 = pm
date[1]: seconds since 12:00
day   = (date[0] >> 1) & 0x3f;
month = (date[0] >> 8) & 0xf;
year  = date[0] >> 12;
hour  = date[1] / 3600;
min   = (date[1] % 3600) / 60;
sec   = date[1] % 60;
am_pm = date[0] & 0x1;
```

- Notes: this is a 12-hour clock. Seconds since 12:00 fit in 16 bits (at most 43199), and the P bit selects am or pm, so 24-hour time = hour + 12×P (with hour 0 = 12 o'clock). The **4-bit year covers 1980–1995 only.**
- `get_ticks()`: "Clock ticks occur **20 times per second**" (50 ms); the 16-bit counter wraps about hourly (CC p323).

### 0.4 Paths and drives

- Full path form is `[node]drive:/dir/…/file`, e.g. `[3]2:/user/dtdodge/abc` (CC p303, full_path).
- Drives are searched in order. With a search path of "1 3", `/user` resolves to `[1]1:/user` even if it exists only on drive 3.
- Example paths `3:/dir1/temp` (CC p163, p173) show that drive 3 exists as a normal drive.
- Device names are 1–5 characters with a leading `$`: `$con` (device 0, also `$tty0`), `$lpt`, `$mdm`, `$tel` (telidon, adopted by the NAPLPS decoder), `$opr`, `$mouse` (adopt_device examples). `[n]$ttyx` addresses a tty on a remote node (CC p127–128, p191, p264, p320, TK2 p307).
- Pathname length can be up to 128 characters (System Software Ops Guide; from the cross-manual grep).

### 0.5 File I/O semantics relevant to the on-disk format

- fopen modes (CC p263–264): `a` append, `c` cache (no effect on the ICON), `d` directory only, `e` must be executable, `f` files only, `m` modify the directory entry, `n` new (fail if it exists), `o` old, `q` quit on error, `r`, `rw`, `u` unbuffered, `v` verbose, `w`, and `*` no buffer. "Only privileged tasks may open directories for write." There is one writer per disk file at a time.
- "512 byte buffers are maintained for files and 80 character buffers … for devices" (CC p327).
- The QNX newline is the **record separator** (RS, 0x1E), not LF. The device option `ERS` "expands record separator (used as newline character in QNX, \n in C programs) into CR/LF on output" (CC p320). **Text files on disk therefore use 0x1E line endings**, which matters for any text export in RustyBackup.
- Error codes for file I/O (`Error_status`) are in `/lib/fsys.h` and device errors are in `/lib/dev.h`. They are not listed in the manual.
- `FILE->_dev_type` is `TYPE_FILE` or `TYPE_DEVICE` (CC p285, p301).

---

## 1. Executables, object files and libraries

### 1.1 Toolchain (CC §1–2, p21–36)

- Quantum (QNX) C Compiler, "designed for the range of INTEL chips from the 8088 to the 80386". It is a five-pass compiler: optional `cpp`, parser `/cmds/cc1`, code generator `cc2`, `asm`, then the linker. Temporary files go in `/tmp`.
- Predefined constants: `i8086`, `QNX` and `QUANTUM` are "Always 1 on the ICON" (CC p30).
- `asm` turns `.a` (assembler source) into **`.o` object modules**: "a machine-language version of your program with information stored at the front of it that is used by the linker" (CC p23).
- **Load module** = the executable: "even closer to an image of what the program looks like in memory than the object file was, and only needs a small amount of handling by the operating system … to be loaded" (CC p23). The default output name without `c=` is **`core`**.
- "Details on [the assembler] stage are available to registered developers from your Service Representative" (CC p35). **The object and load formats are not documented here.**
- Linker errors that reveal the format (CC p686–689):
  - "Each individual object module may only import **256 externals**"
  - "segment exceeds 64K"
  - "Medium model links can have up to **255** functions called outside the code unit" (stubs)
  - "`$overlay` and `$segment`" large-code directives, which are mutually exclusive
  - "name: bad format at offset"
  - "SRC_FNAME directive must be in segment 1"
  - the `g=` option exports common routines
- Startup objects:
  - `/lib/entry.a`: the `E=obj.o` option replaces "the system one in /lib/entry.a" (CC p29)
  - `entry.o` and **`farhdr.o`** ("linked in with large code applications")
  - profiler variants `/lib/profiler.o` and `/lib/prof_farhdr.o` (TK2 p289–290)
- Library layout: `/lib` and `/mathlib` "contain object files"; "entering ls of the /lib directory does not show the different libraries contained within. The library directory is designed on a single level to allow the linker to read it with speed" (CC p37). The linker refers to a "**library directory file**": "A library routine listed in a library directory file cannot be found. If it is on another disk you should insert it" (CC p687). **So a QNX 2 "library" is a directory of `.o` files plus an index file, not an archive.**
- Toolkit libraries: `/tkitlib`, `/sm_tkitlib` (24K smaller, no drawing) and `/demo_lib` (TK1 p24–25, TK2 p23–24); `/develop_lib/speech1.o` (TK2 p54). Libraries are linked with `l=/tkitlib`.
- Memory model:
  - small model is at most 64K code + 64K data (data shared with the stack)
  - medium model is up to 16 code segments of at most 64K each, with at most 25 levels of inter-segment calls; `main` and exception handlers must be in segment 1 (CC p58–60)
  - stack is 1024 bytes from the linker plus 1000 added by `cc`; `s=` adds more (CC p56–57)
- Shared libraries are mounted with `mount lib /config/qdb.slib` (TK2 p195). Also present: `/config/float.slib` (TK2 p209). The CS register in a debugged task "points into the shared library" (TK2 p286), so standard library code is shared and resident.

### 1.2 Load-file flags (`patch`, TK2 p301–304; `Code_flags`, CC p55)

- The data prefix holds `Code_flags` at 0x0060: "Flags in load file (/lib/task.h)". So **the load-file header contains a code-flags byte**, which `patch` edits:
  - `+c` change_user: "make command run with effective user number of user who owns the file" (setuid; e.g. `patch /cmds/date +c` lets anyone set the date)
  - `+r` remote_ok: allow execution on a remote node even when remote task creation is disallowed
  - `+s` shared: "allows multiple instances of this command to share the same code in memory … The program must not have any initialized global variables"
- The bit values are in `/lib/task.h`, which is not reproduced.

### 1.3 Debug files (TK2 p197–202)

- `cc +Q` makes the parser write a `.od` symbol file per source file. The linker with `+Q` writes `<load>.map` (global addresses), `<load>.lin` (source file names plus line-to-address map) and `<load>.sym` (a concatenation of `.od` files). `qdb` also reads `<load>.qin` init files and `.xod` lists of `.od` files. `plinkobj` writes `profiler.map`.

---

## 2. Process and memory layout (useful for emulation)

### 2.1 Data prefix ("Magic Memory"), the first 128 bytes of every task's data segment (CC p54–55, Table 4-1, printed 4-5), verbatim

```
Addr  Type         Name              Use
0000 unsigned      User_sp           (v) SP reg at last kernel call
0002 unsigned      User_ss           (v) SS reg at last kernel call
0004 unsigned      Exc_bits[2]       (v,u) Exceptions present
0008 unsigned      Exc_allow[2]      (v,u) Exceptions allowed
000C unsigned      Exc_pending[2]    (v,u) Exceptions pending
0010 unsigned      Exc_fp            (v) Exception frame ptr
0012 unsigned      Exc_pc            (v) Exception func addr
0014 unsigned      Exc_permit[2]     (v,u) Exceptions permitted
0018 int           Exit_status       (v) Task exit status
001A int           Error_status      (v) Error status
001C unsigned      My_tid            (c,u) Your task id
001E unsigned      Dads_tid          (c,u) Your parents task id
0020 char          Break_count       (v) Unbreakable level
0021 char          My_tty            (c,u) Your tty number
0024 char          Cmd_flags         (c,u) Creation flags (/lib/task_msgs.h)
0025 char          My_priority       (v,u) Your priority
0026 FILE *        stdin             (c,u) Standard input
0028 FILE *        stdout            (c,u) Standard output
002A FILE *        stderr            (c,u) Standard error
002C char *        Free_list         (v) Pointer into calloc's free list
002E char *        End_of_free       (v) Pointer to end of free list
0030 FILE *        File_list         (v) Pointer to linked list of open files
0032 int           Argc              (c) Number of command line arguments
0034 char *        *Argv[]           (c) Pointer to array of strings
0060 char          Code_flags        (c,u) Flags in load file (/lib/task.h)
0061 char          User_num          (c,u) Your user number
0062 char          Group_num         (c,u) Your group number
0063 char          Team_num          (c,u) Your team number
0065 unsigned      My_nid            (c,u) Your node id
0067 unsigned      Dads_nid          (c,u) Your parents node id
0069 unsigned      My_tty_nid        (c,u) The nid your tty is on
006B unsigned      My_cd_nid         (c,u) The nid your current directory is on
006D char          Release_num       (c,u) OS Version number * 10
```

(The scan prints the first Exit_status row and the last row's address as "0060". 006D follows from the sequence.) Note that node IDs are **16-bit `unsigned`**.

### 2.2 System task table (task_info, CC p551–553, Tables 9-1…9-3)

Task 0 = system information. Index/word, verbatim:

> 0 Base Address of Task Table · 1 Number of Tasks / Number of Events · 2 Number of Priorities / Number of Devices · 3 Memory Block Size · 4 System Flags (/lib/task.h) · 5 Slice Size / Number of Virtual Tasks · 6 QNX Version Number * 100 · 7 Node Number · 8 Number of Nodes · 9 Number of Free Tasks / Number of Free Virtual Tasks · 10 Number of Free Memory Blocks · 12 (not used) · 13 Node ID of Registered-Name Server · 14 Task ID of Registered-Name Server · 15 Number of Virtual TTY's / Number of Names · 16 Number of Files · 17 Poller Node ID (Global Poller) · 18 CPU Speed · 19 QNX Revision Number

The real-task entry (Table 9-2) holds:

- Task Link Ptr, Task ID, Blocked-on TID, Send Msg Ptr, Msg Length
- Data Segment, Data Size (in blocks), Task State / Priority, Eff. User / Task TTY Number, Eff. Group / Code Index, Task Flags
- Sender Queue, Reply Msg Ptr, Allocated Memory Link, Parent/Child/Sibling links, Node ID of TTY
- Current Directory Disk ID, Current Directory Node ID, **Current Directory Block (low word), Current Directory Index, Current Directory Block (high word), Current Directory Drive Number / Team Number**

Virtual tasks (Table 9-3) differ at words 13–16: Remote Node ID, Remote ID, Remote Virtual Task ID, Local Task ID.

### 2.3 Memory

- Memory is allocated in **16-byte blocks** (paragraphs). `alloc_segment(n)` gives n×16 bytes, "allocated as high as possible" (CC p129–132). `alloc_perm_segment` is never freed until power-off.
- "The sum of all allocated blocks … can never exceed the data address space of the processor. In the case of the **80186** this is 64K bytes" (CC p389). The CPU is confirmed as an 80186.
- "@" and "-}" are non-standard extra-segment dereference operators (CC p58).
- Exceptions (CC p472, set_exception), verbatim:

```
EXC_HANGUP  0x0001 Modem carrier no longer detected
EXC_BREAK   0x0002 Keyboard break
EXC_QUIT    0x0004 Reserved for program debugging
EXC_COM     0x0002 Communications error   [sic, as printed]
EXC_SHARE   0x0002 Missing shared library [sic]
EXC_FLOAT   0x0080 Floating point error
EXC_KILL    0x0100 Kill task (cannot be trapped)
EXC_MEM_V   0x0400 Memory violation (requires special hardware)
EXC_ALARM   0x2000 Set by system timer task
EXC_TERM    0x4000 Task termination (on task completion)
EXC_DIV_Z   0x8000 Divide by 0
```

### 2.4 I/O ports

- `unsigned io_in(int port)` and `unsigned io_out(int port, unsigned data)` read and write 8-bit I/O ports. "Programs containing io_in can only be compiled by a Site Administrator" (CC p353–355). **No port addresses are given anywhere in these manuals.**
- The low-level `debug` can display and write 8-bit and 16-bit I/O ports (TK2 p285).
- `dev_entry.tty_ioport`: "The I/O port of the controller for serial devices. **This is not valid on an ICON.**" (CC p314)

---

## 3. Network / IPC (CC §5, p65–82)

- Message passing uses `send`, `receive`, `reply`, `creceive`, `relay`, `read_msg` and `write_msg`, plus ports and semaphores (`attach`, `detach`, `signal`, `await`). Interrupt handlers signal a semaphore owned by the hardware administrator (CC p71).
- **Virtual circuits.** To talk to a task on another node, `vc_create(nid, tid, max_msg_size)` creates a local "image" (virtual task) of the remote task. Virtual circuits "are implemented at the very heart of the operating system". The number of virtual circuits limits open files across the network: the maximum `#include` depth "is limited by the number of virtual circuits on your workstation … On a workstation the maximum is **29**. If you can run your cc command on the fileserver, you can get up to **100**" (CC p668).
- Node IDs range over **1–254** (comment `int node_id; /* 1-254 */`, CC p80). `rem_create` creates tasks on other nodes.
- **Registered names:** up to 8 characters, storing node, task ID and one extra word. They are registered locally or globally. "A globally registered name is stored in the task administrator of **node 1**", "the Network Fileserver". There are 10 local and 10 global names per task administrator. "the system poller could remove them if any disruption happens in the network" (CC p74). Task-0 info holds the "Poller Node ID (Global Poller)" and the "Registered-Name Server" node and task.
- `systids.h` holds the fixed task IDs of OS tasks. "you might want these task ID's … to determine whether a node is up by trying to create a virtual circuit to a system task on it" (CC p99).
- `read_sn(node_id)` returns a node's hardware serial number. The example reads node 1 ("fileserver") (TK2 p317).
- No ARCNET-level details appear in these manuals.

---

## 4. Hardware facts

### 4.1 Models and memory

- "In this manual, the term ICON2 refers to a 1-Mbyte ICON2. The term ICON refers to a 512K ICON." (TK1/TK2 p9)
- **`get_machine_type()`** (TK2 p319): "1 ICON (Workstation or Fileserver)", "3 ICON2". The value 2 is unlisted. So the fileserver (LEXICON) reports as an ICON.
- **`read_sn(node)`** (TK2 p317): "The number returned matches the last nine digits of the hardware serial number as displayed by the **nv_config** command. For example, if nv_config reports that the serial number is **200-840505-004**, then read_sn returns the value 840505004."
  - **This ties the serial number to NVRAM** ("nv_" = non-volatile config). That fits ROM self-test code **"11: Missing or failed NVRAM"**: the NVRAM/EEPROM holds at least the unit serial. The `nv_config` command is probably documented in the Admin or System Software guides.
- **Timer:** 20 ticks per second (CC p323). "on all UNISYS ICON machines, the timer [task] is automatically started" (CC p100).
- `8087.h` "not currently used on the ICON"; floating point is a memory-resident emulator (CC p46, p100).

### 4.2 Display (TK1 p27, p39–51, p339–351; TK2 p187–193, p279; CC p635–654)

Graphic modes, from Table 1-1 (TK1 p27):

| Resolution | bpp | Colour registers | Machines |
|---|---|---|---|
| 640×240 | 1 | 2 | ICON and ICON2 |
| 320×240 | 2 | 4 | ICON and ICON2 |
| 640×240 | 2 | 4 | ICON2 |
| 320×240 | 4 | 16 | ICON2 (≥512K) |
| 640×240 | 4 | 16 | ICON2 (1 Mbyte) |

The OCR'd brace placement for the ICON2 rows is ambiguous. `set_monitor` (TK1 p349) is clear:

- monitor type 0 = B/W (640×240×1 or 320×240×2)
- 1 = colour (640×240×1 or 320×240×2)
- 2 = 640×240×2 or 320×240×4
- 3 = 640×240×4
- types 2 and 3 are ICON2 only

Colour:

- On the ICON, each of 2 or 4 registers chooses from **16 colours**. On the ICON2, 16 registers choose from **4096** (16 levels per R/G/B).
- Intensities are given as 0–10000 and snapped to standard values. On the ICON, values out of range wrap around; on the ICON2 they clamp to 10000.
- Table 8-5 (16 ICON colours): black 0/0/0, dark blue 0/0/5000, dark green 0/5000/0, dark yellow 5000/5000/0, dark red 5000/0/0, dark violet 5000/0/5000, dark turquoise 0/5000/5000, dark gray 3000×3, gray 7000×3, then blue, green, yellow, red, violet, turquoise and white at 10000.
- So ICON levels are roughly 0, 50%, 100%, plus 30% and 70% grays (TK1 p340).
- Default colour map (Table 8-6, TK1 p341). The first 4 entries are the ICON's:

```
0:     0     0     0
1: 10000 10000 10000
2: 10000     0     0
3:     0     0  1000   [OCR; probably 10000]
```

Other display facts:

- Screen aspect is 4:3. Graphics origin is bottom-left; "the number of pixels counting vertically is always 240" (TK1 p41).
- The paint pattern window is 16×16 at 1 bpp, 8×16 at 2 bpp and 4×16 at 4 bpp (TK1 p351). Each pattern is therefore 16 bits wide in memory, suggesting packed-pixel video RAM with 16-bit words.
- **The ICON2 has an "ICON Emulator"** that restricts it to ICON modes (TK1 p28).
- **Double-buffered display in system RAM** (TK2 p279): "The two display buffers in system RAM are referred to as display page 1 and display page 2. A register associated with the display hardware selects which display page will appear on the screen. Display page 1 … is used by the QNX operating system for the $con device. Display page 2 is used when the debugger is mounted." Control-Shift-! toggles between pages. `/sys/debug_screen` "reserves memory for display page 2"; the debugger costs 30K of RAM.
- **Text mode (QNX mode):** 80 columns × 24 rows. On the ICON each character takes 2 bytes (character, then attribute). `qnx_tty_start` allocates a 4K "screen text buffer" used by the QDOS video support and high-speed video routines (TK2 p188; CC p653–654).
- Attribute bits (CC p638): b0 dotted underline (OCR "b"), b1 highlight, b2 inverse, b3 underline, b7 "position cursor at end of text". Colour byte: ccc foreground, ccc background, e enable; all "not currently supported".
- **Two console modes:**
  - **ICON mode** (default at login): the `gtask` graphics display task interprets output as **TELIDON** sequences. The TTY type is `DEV_ASCII`.
  - **QNX mode** (`qnx_tty_start`): IBM PC character set and escape sequences, no graphics. The TTY type is `DEV_MONOCHROME`.
  - "All new applications should use the QNX mode" (TK2 p187–193).
- NAPLPS decoder: `naplps_start [-c] bpp x_res [xp yp w h]` adopts `$tel` and uses 100K of RAM; `cat file >$tel` (TK2 p305–308).

### 4.3 Sound and speech (TK2 p53–58, p97–103)

- **ICON:** "Texas Instruments' **TMS 5220** speech chip", fed LPC frames at 40 frames/s and synthesising 8000 samples/s.
- **ICON2:** no TMS chip; "a TMS 5520 [sic] speech chip emulator task which uses **Yamaha's YM-3014 DAC**".
- Administrators: `/sys/speech_admin` (ICON), `/sys/lpc_admin` (ICON2) and `/sys/sound_admin` (ICON2 only), "started by default at boot time" by `speech_start`.
- Speech files store 12 bytes per frame (energy, pitch, K1–K10), 480 bytes/s. There is also an older "TMS5220 codepack" packed format with converters.
- The ICON2 DAC uses 16-bit samples; tone generation runs at 12 kHz and raw DAC output at a user-set rate (`set_samp_rate`).

### 4.4 Keyboard and input (TK2 Appendix C/D p323–330)

- ICON-mode special keys: **Control-Shift-Esc invokes the debugger**, Control-Shift-! switches the debugger screen, Control-Shift-* takes a screen snap, Control-C sends a break, and Help signals the Help port. The ICON keyboard has Action keys (0x80/0x8d), F1–F10 (0x92–0x9b), a RESET key (#016), Cursor Lock and an unlabelled Alt key.
- **ICON and ICON2 keyboards differ.** The ICON2 table (C-2) has Alt, Scroll Lock, Print Screen and arrow keys; it is a PC-style layout.
- Accented characters arrive as Telidon 0x19 sequences.
- `get_option` bits (CC p320–321): 0 ECHO, 1 EDIT, 2 ETAB, 3 ERS, 4 EDEL, 5 IGATE, 6 OFLOW, 7 MAPCR, 8 OUTPUT_PAGED, 9 IFLOW, 12 EXPAND_FUNCTION (0x1000; "FNCT" before QNX 2.01), 13 FUNC_UP_CODES (0x2000) (TK2 p191–192).
- **Trackball** with an "ACTION" key acts as the pointer (the window manager's "tracker").

---

## 5. Boot, config files and system tree hints

- **`1:/config/sys.init`** is the boot-time init script. To enable the debugger: "Modify 1:/config/sys.init so that the command /util/debug_icon is executed before any other device or library is mounted. Reboot the ICON" (TK2 p280). A per-workstation **`user_ws_sys.init`** file is also mentioned, as a place to `mount lib /config/qdb.slib` (TK2 p195). Together these suggest the fileserver's `/config` holds a system init plus per-workstation init files.
- **Once the low-level debugger is running on an ICON, "that ICON cannot be accessed remotely by any other nodes in the network"** (TK2 p280). "It disables interrupts and freezes the whole system".
- Files named:
  - `/cmds/sys.debug`, `/sys/debug_screen`, `/util/debug_icon` (TK2 p280)
  - `/config/qdb.slib`, `/config/float.slib`
  - `/config/tcap.dbase` (terminal database, CC p577)
  - `/sys/speech_admin`, `/sys/lpc_admin`, `/sys/sound_admin`
  - `/util/ied` (icon editor), `/util/font_ed`, `/cmds/plinkobj`, `/cmds/cc1`, `/icons/…`
  - `/lib/*.h` headers: exc, fcntl, setjmp, stdio, io, dev, fsys, magic, systids, task, task_msgs, account, timer, 8087, math, errno, tcap, tcap_keys, routines, win_mgr, speech
  - `/lib/entry.a`, `/lib/profiler.o`, `/lib/prof_farhdr.o`, `/mathlib`, `/tkitlib`, `/sm_tkitlib`, `/demo_lib`, `/develop_lib`
- **Version references:**
  - "with the release of **QNX 2.01**, the expand_function option bit has been introduced" (TK2 p192), consistent with ISSSW 3.00 being built on QNX 2.0x
  - "To run win_mgr2, version 3.00.00, you must install System Software 3.00.00" (TK1 p25)
  - `Release_num` = OS version × 10 (data prefix) and task_info word 6 = "QNX Version Number * 100"
- Device table entry `struct dev_entry` (CC p312–314; `/lib/dev.h`). Field list in printed order, which may be incomplete: `char dev_name[6]; unsigned tty_options; char tty_page, tty_type, tty_BREAK, tty_ESCAPE, tty_RUBOUT, tty_CANCEL, tty_EOT, tty_UP, tty_RIGHT, tty_DOWN, tty_LEFT, tty_HOME, tty_CLEAR, tty_FUNCTION, tty_INSERT, tty_DELETE; unsigned tty_ioport;`. Defaults: BREAK ^C, ESCAPE ^Z, RUBOUT 0x7F, CANCEL ^X, EOT ^D, CLEAR ^L, INSERT 0xD2, DELETE 0xD3.
- `struct stty_entry`, verbatim (CC p325):
  ```c
  struct stty_entry {
     char      mtype;
     char      stty_devno;
     char      stty_type;
     unsigned  stty_baud;
     char      stty_parity;     /* 0 none,1 odd,3 even,5 mark,7 space */
     char      stty_stop_bits;  /* 1 or 2 */
     char      stty_data_bits;  /* 5..8 */
     };
  ```
- `struct state_entry` (video_get_state, CC p637): `char state_cursor_x, state_cursor_y, state_spare1..4, state_cursor_type, state_attr, state_colour, state_fill_colour, state_screen_width, state_screen_height, state_spares[4];` (16 bytes).
- `struct task_start_msg { char msg_type; unsigned tsm_my_tid; }` is a user example, not a system structure (CC p80).
- Accounting: teams are numbered 1–127 and each login creates a team. Accounting flags: bit 0 M (mail waiting), 1 C (chat request), 2 A (check appointment), 15 B (last APB). "The other flags are not implemented on the ICON" (CC p307–309).
- create() flags (`/lib/task_msgs.h`, CC p184): RUN_BACKGROUND 0x01, RUN_CONCURRENT 0x02, BEQUEATH_TTY 0x04, QUERY 0x08, DEBUG 0x10. Command plus arguments must total under 512 bytes.

---

## 6. Corrections and additions to the existing repo docs

1. **Node number range.** ICON_web_research §1 says nodes 16–47 per LEXICON [JE2]. The C manual only says node IDs are 1–254 and that node 1 is the "Network Fileserver" holding global names. This is **consistent** with the existing docs and adds that node 1 is also the registered-name server. Node IDs are 16-bit in the data prefix.
2. **Node ID storage (open question 1).** There is no direct answer, but `nv_config` reports a hardware serial ("200-840505-004" format) from NVRAM. **New:** a 3-part serial with a YYMMDD-like middle field. The repo's LEXICON serial "T-841201-219" has the same middle-field pattern; "200-" may be a model prefix where the LEXICON has "T-". Check `nv_config` in the admin manuals; it probably also shows the node ID.
3. **ICON2 speech.** The repo's hardware table implies the TMS5220 is standard. **On the ICON2 there is no TMS5220.** Speech is emulated in software through a Yamaha YM-3014 DAC (`lpc_admin`), and the ICON2 adds a DAC sound administrator. The repo's note that the LEXICON has "no speech chip" is not contradicted.
4. **ICON2 keyboard** is a different, PC-like layout with Alt, Scroll Lock, Print Screen and arrows. The ICON keyboard has Action keys, a RESET key, Cursor Lock and an unlabelled Alt.
5. **Graphics.** These manuals confirm the repo's 640×240×1 / 320×240×2 / 16-of-4096 facts and add more. The 4-bpp modes and 4096-colour palette belong to the **ICON2** (not just "a later model"). Monitor types 2 and 3, 640×240×2 and 640×240×4 (1 MB ICON2) are additional modes. The display is double-buffered in system RAM. Text mode is 80×24 with 2 bytes per character.
6. **Floppy geometry.** The repo infers "about 512-byte blocks". This is **confirmed**: blocks are exactly 512 bytes, numbered from 1. The 320K format is 8 spt × 2 sides × 40 cyl, and the 640K format is 8 spt × 2 × 80, so 1280 blocks with 4 overhead after dinit. 9 spt is for non-QNX disks.
7. **Filenames** of up to 16 characters are **confirmed** (NUL-terminated `fname[17]`).
8. **Clock.** The QNX date format can only hold years 1980–1995, a limit worth noting for any date conversion in RustyBackup. Ticks are 20 Hz.
9. **"Version 1 © 1983 CEMCORP" boot banner vs QNX 2.01.** These manuals reference QNX 2.01 features in ISSSW 3.00 (1988). That supports the brief's statement that 3.x/4.x are on QNX 2.x, versus QNX 1.13 in *The Book of ICON*.

## 7. Not found in these three manuals

- the bitmap layout
- the root-directory block number
- the extent header (`xtnt`) format
- the boot block
- the `/boot` contents
- the load-file or object-file binary layout
- ARCNET registers
- I/O port addresses
- ROM monitor or boot-loader options
- the ROM self-test codes 09–5a

The only ROM-adjacent facts are:

- Control-Shift-Esc enters the debugger
- the `nv_config` / NVRAM serial number
- the debugger must be mounted from `sys.init` before other devices
