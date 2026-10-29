# QNX 2 Filesystem (ICON / LEXICON): Working Spec for a RustyBackup Reader

This spec combines four sources. Each fact carries a tag in the confidence column.

| Tag | Source |
|---|---|
| **[CC]** | *ICON Series C Compiler Programming Reference Manual* (ISSSW 3.00, 1987), with `/lib/fsys.h` excerpts |
| **[SSOG]** | *ICON Series System Software Operations Guide* (ISSSW 3.00), especially the `chkfsys`, `dinit`, `bbt` and `mount` error messages in Appendix A |
| **[AG]** | *Administration Guides* Vols. 1–3 |
| **[ROM]** | Disassembly of the ICON boot ROM ([ICON_firmware.md](ICON_firmware.md) §7) |
| **[Q2]** | `q2files.c` / `q2read.c`: Mitchell Schoenbrun's public-domain QNX 2 readers (2010), copied to [reference/q2tools/](reference/q2tools/) |

Full manual extracts with page citations are in [manual_notes/](manual_notes/).

> **Update (2026-10-28): verified against real images.** RustyBackup's QNX 2 reader (`src/fs/qnx2.rs`) exports all 34 QNX disks in the YorkU `Disk_1` set, 2,498 entries, with no errors. What it established:
> - **Extent `size_xtnt` counts bytes:** the data bytes after the 16-byte header. `bound_xtnt` is the number of extra blocks in the extent. **A file's length is the sum of its extents' `size_xtnt`.** The Q2 formula in §4 overshoots.
> - `fnum_blks` = total blocks − 1. `fnum_chars_free` is 0 and the "fseconds" bytes are 0 on every file checked.
> - The root `dir_entry` is at block 1 + 68 on every disk. On fileserver boot disks it points to a root directory at **block 12 or 15**, after the boot loader.
> - The directory header is 4 bytes, or 6 when bit 15 of `dir_index` is set; this works as specified.
> - **Every YorkU QNX disk is 800K** (80×2×10). The bitmap is at block 2: 200 bytes for 1,600 blocks, 1 bit per block. No 640K disk has been tested.
> - The disks labelled "QDOS" are QNX 2 volumes, so detection must go by content, not by label.
> - Still unverified: the date layout (§6), the bitmap bit order, the meaning of the permission bits on disk, and hard disks.
>
> The rest of this file is the original pre-verification spec. Where it conflicts with the box above, the box wins.
>
> ⚠️ *(Original note)* **Nothing here has been checked against a real image yet.** The disk images are being handled separately. Items marked **VERIFY** are the ones to settle first against a known-good floppy, such as the 4.10.01 Help diskette.

## 1. Basics

| Fact | Confidence |
|---|---|
| Block = **512 bytes**. Block numbers are **32-bit** and **1-based**: block *n* is at byte `(n−1)×512` of the volume | CC, SSOG, ROM |
| All multi-byte integers are **little-endian**. `int`/`unsigned` = 2 bytes, `long` = 4 bytes, and structs are packed | CC |
| **Block 1** = root block. It holds the boot loader pointer, the floppy format table and the root directory's `dir_entry` | ROM, SSOG, Q2 |
| **Block 2** = first block of `/bitmap`, which is a real file with its own extent header | SSOG ("Unable to read first block of bitmap (BLK 2)") |
| A fresh `dinit` 640 KB floppy has 1280 blocks and 1276 free, so there are **4 blocks of overhead**: root block, bitmap and root directory | Book + SSOG |
| Filenames are 1–16 characters from `A–Z a–z 0–9 . _ $` and **case-sensitive**. A directory holds at most **682 entries** | SSOG |
| Text files end lines with **RS `0x1E`**, not LF. There is no EOF marker | SSOG, CC |
| Maximum volume size is **32 MB**, so a 64 MB disk is split into two partitions | SSOG, AG |

## 2. Block 1 (the root block)

| Offset | Size | Field | Confidence |
|---|---|---|---|
| +0x02 | u16 | **Boot image start block** (1-based). Must be ≥ 1 and below the ROM limit | ROM |
| +0x04 | u16 | **Boot image length in blocks.** 0 means no boot image (the ROM reads nothing more) | ROM |
| ? | ? | **Format table** (heads, sectors per track, tracks), written by `fdformat` from QNX 2.01 on. "This information always overrides that specified by the mount command." Disks formatted before 2.01 don't have it | SSOG p313 — **VERIFY offsets** |
| ? | ? | Root name prefix (`dinit p=/name`, floppies only) | SSOG — **VERIFY** |
| **+0x44 (68)** | 48 | **Root directory `dir_entry`.** Its `ffirst_xtnt` points to the root directory's first extent | Q2 (`struct first_sector { char stuff[68]; struct dir_entry root; }`) |

The ROM loads `count` contiguous blocks from `start` to `3000:0000` and jumps there. On a bootable "solo" or fileserver floppy those blocks are the loader (`dinit l=<loader>`), and they are not part of any file. That explains the "special hidden boot files" mentioned in the install guides.

## 3. Directory entry (48 bytes, verbatim layout from `/lib/fsys.h` [CC p317])

```c
struct dir_entry {            /* off size */
    char     fstat;           /*  0  1  0x10 modified-since-backup, 0x40 USED, 0x80 BUSY (open for write) */
    long     ffirst_xtnt;     /*  1  4  block of first extent (its header block) */
    long     flast_xtnt;      /*  5  4  block of last extent */
    long     fnum_blks;       /*  9  4  blocks in file */
    unsigned fnum_xtnt;       /* 13  2  number of extents */
    char     fowner;          /* 15  1  member number (0–255) */
    char     fgroup;          /* 16  1  group number  (0–255); 255.255 = site administrator */
    char     fspare[6];       /* 17  6  see note */
    char     ftype;           /* 23  1 */
    char     fgperms;         /* 24  1  group permissions */
    char     fperms;          /* 25  1  other permissions */
    char     fattr;           /* 26  1  owner attributes; 0x20 = directory */
    unsigned fdate[2];        /* 27  4  see §6 */
    char     fname[17];       /* 31 17  NUL-terminated, NCPFN = 16 */
};                            /* = 48 */
```

- **fspare:** Q2 treats bytes 17–22 as `unsigned fnum_chars_free` (bytes unused in the last block; Q2 masks it with `0x1FF`) followed by `long fseconds`. The 1987 header calls them spare, so a later QNX 2 probably started using them. **VERIFY** on a 4.x disk.
- **Size check:** 682 × 48 = 32736 ≤ 32767 < 683 × 48. That fits a 48-byte record addressed with a signed 16-bit offset, so the 48-byte packed layout is very likely right.
- **Permission and attribute bits** (fattr, fgperms, fperms): `0x01` read · `0x02` write · `0x04` append (file) / create (directory) · `0x08` execute (file) / block (directory: no access below) · `0x10` modify · `0x20` directory. `files +v` prints them in the order **m e a w r**.
- **Owner and group:** the owner is shown as **group.member**. The default new file is attributes `m-awr` with permissions `r`.
- **Deleted entries:** `zap` and `chattr s=-u` clear `_USED` (0x40) and leave the entry and its blocks alone. `files -used` lists them. **An undelete mode is feasible**, because `zap` doesn't even free the blocks in the bitmap.

## 4. Extents

Every file, including directories and `/bitmap`, is a doubly-linked chain of contiguous **extents**. The **first block of each extent starts with a 16-byte header**:

```c
struct xtnt_hdr {      /* off */
    long prev_xtnt;    /*  0  block of previous extent's header, 0 for first */
    long next_xtnt;    /*  4  block of next extent's header, 0 for last */
    long size_xtnt;    /*  8  size of this extent  (VERIFY: blocks or bytes) */
    long bound_xtnt;   /* 12  allocated bound of extent (VERIFY) */
};
```

Field names are confirmed by the `chkfsys` messages [SSOG A-11..15]: "prev/next", "size_xtnt must be ≠ 0", "Bound of extent inconsistent with size", "Bound past end of disk", and "Non-zero extent extension". The last suggests a reserved field that must be zero, possibly in the header block or in an extended form.

- **Data layout** [Q2]: an extent's data begins at byte 16 of its first block and runs through the remaining blocks of the extent, which are contiguous, using all 512 bytes of each. Then it continues with the next extent, again after a 16-byte header.
- **Logical file size: ✅ VERIFIED.** It is the **sum of `size_xtnt` over all extents**, which are byte counts of the data after each 16-byte header. `bound_xtnt` = the extra blocks allocated in the extent. Q2's formula `fnum_blks*512 + 512 − (fnum_chars_free & 0x1FF) − fnum_xtnt*16` is **wrong**: it overshoots.
- *(Superseded)* Q2 decrements `size_xtnt` once per byte read. If the field is really a block count, Q2 would stop after N bytes per extent. That would explain the "sometimes crashes / sometimes works" reports, so don't copy this part of Q2 blindly. Check it with a multi-block, single-extent file.

## 5. Directory files

A directory is an ordinary extent-chained file, read as one byte stream across extents:

```c
struct dir_hdr {           /* first 4 bytes of the directory's data stream */
    unsigned parent_xtnt;  /* VERIFY: only 16 bits */
    unsigned dir_index;    /* if bit 15 is set, a third word follows: */
 /* unsigned ext_parent_xtnt; */
};
/* followed by struct dir_entry[ ] (48 bytes each) until the end of the data */
```

- An entry is in use if `fstat & 0x40`. Skip unused entries, but keep them available for undelete.
- A directory entry has `fattr & 0x20`. Recurse through its `ffirst_xtnt`.
- `mkdir` preallocates 10 entries, so directories fragment into several extents as they grow.

## 6. Dates (`fdate[2]`) [CC p315]

```
fdate[0]: bits 15-12 year-1980 (0–15 → 1980–1995)   bits 11-8 month (1–12)
          bit 7 unused?   bits 6-1 day (1–31)        bit 0 PM flag
fdate[1]: seconds since 12:00 (0–43199)
hour24 = fdate[1]/3600 + 12*(fdate[0]&1);  min = fdate[1]%3600/60;  sec = fdate[1]%60
```

The 4-bit year limits dates to 1980–1995. Files from the 4.10 era (1991) fit.

## 7. Bitmap

`/bitmap` starts at block 2, has a 16-byte extent header, and holds 1 bit per block. **VERIFY:** the bit order, and whether bit 0 is block 1. It is derived data: `chkfsys` can rebuild it, and `zap` leaves blocks marked used. A reader should **trust the directory tree, not the bitmap**, and can use the bitmap only as a cross-check or to find orphaned blocks.

## 8. Media geometry and image detection

| Media | Geometry | Blocks | Raw image size | Source |
|---|---|---|---|---|
| **Standard ICON floppy** | 80 cyl × 2 heads × **8** spt × 512 B | 1280 | **655,360** | ROM, SSOG (default) |
| 800K floppy | 80 × 2 × **10** | 1600 | 819,200 | AG2 p63 |
| Other `fdformat` options | 40/80 tracks, 1/2 sides, 8/9/10 spt | 320–1600 | 160K–800K | SSOG Table 9-1 |
| 10 MB fileserver disk, "old format" | 306 cyl × 4 heads × 17 spt | **20808** | 10,653,696 | ROM limit `5148h`, install guides |
| 64 MB fileserver disk (QNX 2.11+) | physical block 1 = master partition record (up to 4 entries), then the **Bad Block Table** and a **loader area**, then **2 × 32 MB partitions** (drives 1: and 3:). Each partition has its own block 1 and bitmap | — | — | SSOG `bbt`/`mount` errors |
| **40 MB Seagate ST-251** (real drive) | **820 cyl × 6 heads × 17 spt**, 102 blocks/cylinder | **83,640** | 42,823,680 | `ICON4_ST251_final` MFM capture (see below) |
| 64 MB (emulator) | 1024 × 8 × 17, 136 blocks/cylinder | 139,264 | 71,303,168 | icon-emu `hdformat` run |

**Hard-disk layout, confirmed on a real drive.** `ICON4_ST251_final` is an MFM-reader capture of a real ICON fileserver drive, a Seagate ST-251. The `.emu` file is a flux-level capture in David Gesswein's MFM reader/emulator format, read with `--format Intel_iSBC_214_512B --heads 6 --cylinders 820 --sectors 17,0`. The `.bin` was extracted as **820 × 8 × 17**, which pads two empty heads into every cylinder. Repack it to 6 heads (keep the first 102 of every 136 blocks) before use.

Its block 1 has the same structure as the disks icon-emu's real `hdformat` made:

| Item | ST-251 value |
|---|---|
| Header +02 / +04 | loader start block 3, length 14 |
| Geometry record +10 | `02 01 02 06`: 6 heads; total blocks 83,640; 102 blocks per cylinder; 17 sectors per track; `03` at +2F |
| BBT record +30 | 41 spare blocks (+33 = `29`); BBT at block 2, 1 block long |
| Loader, blocks 3–16 | byte-identical to the 4.10.01 `HDos_ldr` written in the emulator |
| Partition 1 | type **07**, start **60** (LBA 59), 65,535 blocks. 2 + 1 + 14 + 41 + 2 = 60, the same formula as the 10 MB (30) and 64 MB (89) disks |
| Partition 2 | type **08**, start 65,595 (LBA 65,594), 18,046 blocks; never `dinit`ed (all zero) |

**Bad Block Table entry format** (block 2): 8-byte entries, each a u32 **bad block number** (0 = entry unused) and a u32 **replacement block number**, assigned in sequence (21, 22, 23 …). On this drive and on emulator-formatted disks every entry is unused, so no remapping applies.

Partition 1 holds ICON System Software 4.10.01. All 425 installed files are byte-identical to icon-emu's install. It also has the five System SW install scripts (`QNX_1`–`QNX_5`), `package` and `convert_4.02` in its root (identical to the copies on the System SW diskettes), and no `/config/net.config`, `/tmp` or `/user_cmds`. Every file is dated 1980, so it was installed with the clock unset and never configured as a network fileserver.

- Sectors on a floppy image are stored in **cylinder → head → sector** order, and block *n* = sector index *n−1*. The ROM's mapping is `cyl=(n−1)/(2·spt)`, `head=((n−1)/spt)%2`, `sec=(n−1)%spt+1`. Sector stagger (2 or 3) only matters for flux images. KryoFlux decoders already give logical order.
- For **hard disk images**, apply the Bad Block Table's remapping (entry format above) before reading, or the remapped blocks will come back as garbage. Entries with bad block 0 are unused. The `bad_blocks` text file (one hex block number per line) is a helper.

## 9. Non-filesystem formats you may meet on ICON floppies

| What | How to recognise it | Source |
|---|---|---|
| **`arch` multi-volume archive** | No valid root or bitmap. A directory of slots in multiples of 8 (default 128, max 816), with files appended and spanning disks | SSOG p131–135 |
| **`dcopy` raw slices** | Raw sector copies of part of a hard disk (`so=`/`do=` offsets, at most 112 blocks per pass) | SSOG p189–191 |
| **QDOS pseudo-disks** (inside a QNX file) | `*.pf` = DOS floppy image (160/180/320/360 KB); `*.ph` = DOS hard-disk image (≤ 32 MB). Probably raw FAT volumes, so hand them to a FAT reader | QDOS Ops p23–25 |
| **MS-DOS 3.2 boot and GW-BASIC floppies** | Plain FAT12 PC disks | Disk labels |

## 10. Suggested reader algorithm

1. Choose the geometry from the image size (§8). If block 1 has a format table, let it override.
2. Read the root `dir_entry` at block 1 + 68. Sanity check: `fstat & 0x40`, `fattr & 0x20`, `1 < ffirst_xtnt < total blocks`, and `fnum_xtnt ≥ 1`.
3. Follow the extent chain using `next_xtnt`, and check each `prev_xtnt` back-link. Stop on a loop or an out-of-range block.
4. Skip the directory header (4 or 6 bytes) and walk the 48-byte entries, recursing into directories.
5. Use the logical size from §4 to extract each file. Optionally convert text files `0x1E → \n`, detecting text by the absence of control characters other than RS and TAB.
6. Report as metadata: owner (group.member), the attribute and permission strings `meawr`/`rcbm`, the date, the backup-modified bit, and the busy bit, which means the file was open at a crash.
7. Optionally list deleted entries and orphaned blocks.

## 11. Open items to settle with the first real image

1. The offsets of the format table and root prefix in block 1 (dump bytes 0–67 from several disks and diff them).
2. Whether `size_xtnt` and `bound_xtnt` count blocks or bytes.
3. The meaning of `fspare` (`fnum_chars_free` and `fseconds`?) on 3.x versus 4.x disks.
4. The bitmap's bit order.
5. Whether `parent_xtnt` is really 16-bit, and what `dir_index` is.
6. The format of the 64 MB partition record and the Bad Block Table, which needs a hard-disk image.
7. The executable (load module) and `.o` header formats. These are undocumented ("available to registered developers"); `/lib/task.h` on a disk image may define them.

Useful headers to extract from the C Compiler diskette once a reader works: `/lib/fsys.h`, `/lib/io.h`, `/lib/dev.h` and `/lib/task.h`. They should answer most of §11.
