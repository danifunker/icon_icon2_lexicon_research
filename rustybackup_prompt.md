You are building or verifying a **QNX 2 filesystem reader in RustyBackup** for disk images from Unisys/Burroughs/CEMCORP **ICON** workstations and **LEXICON** fileservers. These were Ontario school computers from 1984 to 1994, running Unisys "ICON System Software" (ISSSW) releases 3.00 through 4.10, which are built on QNX 2.x.

Everything you need is below. The spec was put together from the official Unisys manuals, a disassembly of the ICON boot ROM, and an old public-domain QNX 2 reader. **None of it has been checked against a real disk.** Your images are the ground truth: wherever an image disagrees with this spec, the image wins, and you should report the disagreement. Treat every disk image as read-only.

Confidence tags used below: **[CONFIRMED]** = from official C headers or the boot ROM code · **[LIKELY]** = strong indirect evidence · **[VERIFY]** = unknown or contradictory; settle it from the images.

---

## 1. Spec

### 1.1 Basics
- A block is **512 bytes**. Block numbers are **32-bit and 1-based**: block *n* is at byte `(n-1)*512` of the volume. [CONFIRMED]
- All integers are **little-endian**, `int`/`unsigned` = 16-bit, `long` = 32-bit, and structs are **packed** (no padding). [CONFIRMED]
- **Block 1** is the root block. **Block 2** is the first block of the file `/bitmap`, which is a normal file with an extent header. A freshly initialised 1280-block floppy has 1276 free blocks, so there are 4 blocks of overhead: root block, bitmap and root directory. [CONFIRMED]
- Filenames are 1–16 characters from `A–Z a–z 0–9 . _ $`, case-sensitive. A directory holds at most 682 entries. [CONFIRMED]
- Text files end each line with **RS = 0x1E**, not LF or CR/LF, and have no EOF marker. [CONFIRMED]
- A volume is at most 32 MB.

### 1.2 Block 1 (root block)
| Offset | Type | Meaning | Tag |
|---|---|---|---|
| +0x02 | u16 | Boot image **start block** (1-based). The boot ROM loads from here | CONFIRMED (boot ROM) |
| +0x04 | u16 | Boot image **length in blocks** (0 = not bootable). These blocks belong to no file | CONFIRMED (boot ROM) |
| ? | ? | **Format table** (heads, sectors/track, tracks), written by `fdformat` from QNX 2.01 on. "Always overrides the mount command." Older disks don't have it | VERIFY offsets |
| ? | ? | **Root name prefix** (`dinit p=/name`; floppies only) | VERIFY |
| **+0x44 (68)** | 48 bytes | **Root directory `dir_entry`** (§1.3). Its `ffirst_xtnt` points to the root directory's first extent | LIKELY (old reader) |

### 1.3 Directory entry: 48 bytes, verbatim from the official `/lib/fsys.h`
```c
struct dir_entry {            /* off size */
    char     fstat;           /*  0  1  0x10 modified-since-backup, 0x40 USED, 0x80 BUSY (open at crash) */
    long     ffirst_xtnt;     /*  1  4  block of first extent's header */
    long     flast_xtnt;      /*  5  4  block of last extent's header */
    long     fnum_blks;       /*  9  4  blocks in file */
    unsigned fnum_xtnt;       /* 13  2  number of extents */
    char     fowner;          /* 15  1  member number 0-255 */
    char     fgroup;          /* 16  1  group number 0-255   (255.255 = site administrator) */
    char     fspare[6];       /* 17  6  VERIFY: old reader uses as  u16 fnum_chars_free @17, u32 fseconds @19 */
    char     ftype;           /* 23  1 */
    char     fgperms;         /* 24  1  group permissions */
    char     fperms;          /* 25  1  other permissions */
    char     fattr;           /* 26  1  owner attributes */
    unsigned fdate[2];        /* 27  4  see 1.6 */
    char     fname[17];       /* 31 17  NUL-terminated, max 16 chars */
};
```
- **Bits in fattr, fgperms and fperms:** `0x01` read · `0x02` write · `0x04` append (file) / create (directory) · `0x08` execute (file) / block (directory: nothing below it is accessible) · `0x10` modify · `0x20` **directory**. They are conventionally printed in the order `m e a w r` for files and `r c b m` for directories.
- **Ownership** is shown as `group.member`. A new file defaults to attributes `m-awr` and permissions `r`.
- **Deleted entries:** a deleted entry has `fstat & 0x40` clear, with the entry and its data blocks left in place. The `zap` command does not even free the blocks in the bitmap, so **undelete is feasible**.

### 1.4 Extents [field names CONFIRMED, semantics VERIFY]
Every file, including directories and `/bitmap`, is a doubly-linked chain of contiguous extents. **The first block of each extent starts with a 16-byte header:**
```c
struct xtnt_hdr { long prev_xtnt; long next_xtnt; long size_xtnt; long bound_xtnt; };
/* prev = header block of previous extent (0 for first); next = header block of next (0 for last)
   size_xtnt must be != 0;  bound_xtnt: "bound of extent inconsistent with size" / "bound past end of disk" */
```
- The filesystem checker also has a "Non-zero extent extension" error, which implies a reserved field that must be 0.
- **Data layout** (old reader): file data starts at byte 16 of an extent's first block and continues through that extent's remaining blocks, using all 512 bytes of each. It then resumes 16 bytes into the next extent's first block.
- **Logical size** (old reader): `fnum_blks*512 + 512 - (fnum_chars_free & 0x1FF) - fnum_xtnt*16`. This depends on the §1.3 fspare interpretation. [VERIFY]
- ⚠️ The old reader decrements `size_xtnt` **once per byte**. If the field is really a block count, that reader stops after N bytes of each extent. It is reported to "sometimes work, sometimes crash", so **don't copy this behaviour**; determine the true unit.

### 1.5 Directory files [VERIFY]
A directory is an extent-chained file. Read it as one byte stream across extents:
```c
u16 parent_xtnt;      /* VERIFY: really only 16 bits? */
u16 dir_index;        /* if bit 15 set, one more u16 follows: ext_parent_xtnt */
struct dir_entry entries[];   /* 48 bytes each, until logical end of directory data */
```
`mkdir` preallocates 10 entries, so directories often span several extents.

### 1.6 Dates [CONFIRMED, from the C manual's get_date]
```
fdate[0]: bits 15-12 = year-1980 (1980-1995 only)  bits 11-8 = month  bits 6-1 = day  bit 0 = PM flag
fdate[1]: seconds since 12:00 (0..43199)
hour24 = fdate[1]/3600 + 12*(fdate[0]&1); minute = (fdate[1]%3600)/60; second = fdate[1]%60
```

### 1.7 Bitmap [VERIFY bit order]
One bit per block. The data starts in block 2, after a 16-byte extent header. It is **derived data**: the filesystem checker can rebuild it, and `zap` leaves stale bits set. **Use it only as a cross-check; trust the directory tree.**

### 1.8 Geometry and image sizes
| Media | Geometry | Blocks | Raw image size |
|---|---|---|---|
| **Standard ICON floppy (the default)** | 80 cyl × 2 heads × **8** sectors × 512 B | 1280 | **655,360** |
| 800K floppy | 80 × 2 × **10** | 1600 | 819,200 |
| Other `fdformat` options | 40/80 tracks × 1/2 sides × 8/9/10 spt | 320–1600 | 163,840 – 819,200 |
| 10 MB fileserver hard disk ("old format") | 306 cyl × 4 heads × 17 spt | 20,808 | 10,653,696 |
| 64 MB fileserver hard disk | Physical block 1 = master partition record (up to 4 entries), then a **Bad Block Table** (bad blocks remapped to spares) and a **loader area**, then **two 32 MB partitions**, each with its own block 1 and bitmap | — | — |

- Logical images are in cylinder → head → sector order, with block *n* = sector index *n−1*. The ROM maps floppy blocks as `cyl=(n-1)/(2*spt), head=((n-1)/spt)%2, sector=(n-1)%spt+1`. Sector stagger is 2 or 3 (the manual contradicts itself); it only matters for raw flux decoding.
- **Hard disks:** the Bad Block Table format is unknown. Without its remap, remapped blocks will read as garbage.

### 1.9 Other formats you may find on ICON floppies
- **FAT12 PC disks:** labelled "MS-DOS Boot Diskette 3.2" and "GW-BASIC Interpreter 3.2". Hand these to a FAT reader.
- **QDOS pseudo-disks** stored *as files inside QNX*: `*.pf` = a DOS floppy image of 160/180/320/360 KB, and `*.ph` = a DOS hard-disk image of up to 32 MB. They are probably raw FAT volumes; offer nested extraction.
- **`arch` multi-volume archives:** no valid QNX root or bitmap, and a directory of fixed slots in multiples of 8 (default 128, max 816). Detect them and report them rather than misparse them.
- **"Netcard Fileserver Boot" diskettes** (for PCs with an ARCNET card) probably have a PC-BIOS boot sector in front of a QNX filesystem. Compare their block 1 with an "ICON Fileserver Boot" diskette.

### 1.10 Expected system tree (ISSSW 3.00, for sanity checks)
`/boot` (`ws_boot`, `hd_boot`, `fs_net_boot`), `/cmds`, `/util`, `/user_cmds`, `/config` (`pass`, `sys.init`, `net.config`, `u_sys.init.<4-hex-digit node>`, `print/`), `/drivers` (`ws_floppy`, `hd`, `fd`, `ramdisk`), `/fonts`, `/icons`, `/sys` (`bootsnd`, `bootrec`, `win_mgr`, ...), `/tmp`, `/user/<login>`, `/spool`, plus the file `/bitmap`. Release diskettes hold subsets of this tree.

---

## 2. Part A: answer these from real images

For each answer give the **image filename, block number, byte offset and a hex dump** as evidence.

- **FS-1:** Is `size_xtnt` (and `bound_xtnt`) counted in blocks or bytes? Show one file with a single extent of more than one block, and one file with several extents.
- **FS-2:** The layout of block 1, bytes 0–67. Confirm that +2/+4 are the boot start and count, and find the format table and the root name prefix. Diff block 1 across at least 5 disks, both bootable and non-bootable.
- **FS-3:** What do bytes 17–22 of a `dir_entry` hold? Does the §1.4 size formula give exact sizes? Check it on text files, whose true ends are easy to see. Compare a 3.00-era disk with a 4.x disk.
- **FS-4:** The directory header. Is `parent_xtnt` 16-bit? What is `dir_index`? Is the bit-15 extended form ever used? Do 48-byte entries straddle block or extent boundaries?
- **FS-5:** The bitmap's bit order, and whether bit 0 means block 1. Check it on the emptiest disk.
- **FS-6:** Extract the C headers (`/lib/fsys.h`, `io.h`, `dev.h`, `task.h`, or whatever exists) from the **C Compiler diskette 3.00.00** and **paste their full text** in your reply. They should define the structures above authoritatively.
- **FS-10:** Make a table of every image: name, size, implied geometry, and type (QNX / FAT / arch / unknown).
- **EXE-1 / EXE-3:** From a 4.x fileserver boot diskette, extract `/boot/ws_boot`, `hd_boot`, `fs_net_boot` and `/sys/bootsnd` and `bootrec` if present. Hex-dump the first 64 bytes of each and of three `/cmds` binaries, so the executable header can be worked out. Say which block range the block 1 boot fields point at, and whether those blocks belong to any file.
- **REL-1 / REL-2:** List the root directory of every 4.10.x image (Exception Release, Upgrade Release, 4.10.05 patch). Report every QNX or ISSSW version string you find.
- **REL-4:** For each image marked "(bad sectors)" that has a second copy, report which blocks differ or are unreadable, whether merging the two copies recovers them, and which files any remaining bad blocks belong to.

## 3. Part B: acceptance checklist for the reader

Report pass, fail or n/a for each item, with how you tested it.

- [ ] **Geometry:** chosen from the image size (§1.8). A format table in block 1 overrides the size when present.
- [ ] **Addressing:** block *n* is at byte `(n-1)*512`; integers are little-endian and structs packed.
- [ ] **Root entry:** read at block 1 + 68 and sanity-checked (`fstat&0x40`, `fattr&0x20`, extent block in range, `fnum_xtnt ≥ 1`).
- [ ] **Extent chains:** followed through `next_xtnt`, with `prev_xtnt` back-links checked, loop detection and range checks. The 16-byte header is skipped in each extent's first block.
- [ ] **Directory entries:** parsed per §1.3, with names up to 16 characters, case-sensitive.
- [ ] **Recursion:** works through directories, including directories with several extents.
- [ ] **File extraction:** byte-exact, at the correct logical size.
- [ ] **Metadata:** owner shown as `group.member`, attribute and permission letters (`meawr` / `rcbm`), the modified (backup) bit, the busy bit, and dates decoded per §1.6.
- [ ] **Text conversion:** an optional RS (0x1E) → LF conversion. The default is raw bytes.
- [ ] **Undelete:** deleted entries can be listed and, where their blocks are intact, recovered.
- [ ] **Bitmap:** used only as a cross-check. Report blocks marked used that no file owns, and file blocks marked free.
- [ ] **Non-QNX hand-off:** FAT12 disks, QDOS `.pf`/`.ph` container files and `arch` volumes are detected and handed off rather than misparsed (§1.9).
- [ ] **Damaged images:** unreadable blocks are reported per file, and the rest of the tree still extracts.
- [ ] **Hard disks:** at least a design stub for 10 MB single-volume images and for 64 MB images (partition record, Bad Block Table remap, two partitions).

## 4. What to send back

Reply in markdown with these sections:

1. **Answers by gap ID:** a table with the ID, the answer, a confidence level, and a pointer to the evidence (image, block, offset).
2. **The Part B checklist** with results.
3. **Contradictions:** every place where the images contradict this spec. Quote the spec line and give what you observed.
4. **The image inventory table** (FS-10).
5. **The full text of any C headers** extracted under FS-6.
6. **New open questions or surprises** worth tracking.
