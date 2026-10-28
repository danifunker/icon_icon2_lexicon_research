# Known Gaps

A single list of what we still don't know, with what would close each gap. Every item has an ID so commits and notes can point to it (for example "closes FS-2"). When something is resolved, set the **Status** to ✅ and say where the answer now lives.

**Priority:** **P1** blocks the RustyBackup QNX 2 reader or a bootable emulator · **P2** is needed for a complete picture · **P3** is nice to have.

**Most of the P1 filesystem gaps should fall to the first readable floppy image.** The YorkU disks turned out to be 800K, and the RustyBackup reader already exports all 34 of them (see §5b).

---

## 1. Filesystem: what the RustyBackup reader needs

Spec: [ICON_qnx2_filesystem.md](ICON_qnx2_filesystem.md).

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| FS-1 | **Is `size_xtnt` (and `bound_xtnt`) counted in blocks or bytes?** | Getting this wrong truncates or overruns every multi-block file. `q2files.c` treats it as bytes, which may be its "sometimes crashes" bug | Find a file with one extent and more than one block on a real image, and compare `size_xtnt` with `fnum_blks` | P1 | ✅ **bytes**: the data after the 16-byte header. `bound` = extra blocks. File length = sum of `size_xtnt`; the q2files formula is wrong (RustyBackup, 2026-10-28) |
| FS-2 | **What's in block 1, bytes 0–67:** format-table offsets (heads, spt, tracks) and the root name prefix (`dinit p=`) | Needed to pick the geometry and to recognise non-`/` roots | Hex-diff block 1 across several disks with different formats and boot flags. Only +2/+4 (the boot start and count) are known, from the ROM | P1 | partly: the root entry is at +68 on all 34 disks, and on boot disks the root dir is at block 12/15. Format-table offsets and the +2/+4 boot fields are still unchecked |
| FS-3 | **What are `fspare[6]` (bytes 17–22)?** Is it `fnum_chars_free` + `fseconds` as in `q2files.c`, or spare as the 1987 header says? | The logical file size formula depends on it | Compare 3.00 and 4.x disks. Check against `/lib/fsys.h` on the C Compiler diskette | P1 | partly: `fnum_chars_free` = 0 and the seconds bytes = 0 on every file seen; `fnum_blks` = total − 1. Date words not yet decoded |
| FS-4 | **Directory header:** is `parent_xtnt` really 16-bit? What does `dir_index` mean, and how is the extended form (bit 15) used? | Needed to walk directories correctly | Dump a root directory and a subdirectory | P1 | mostly: the 4/6-byte header works as specified on all disks. The meaning of `dir_index` is still open |
| FS-5 | **Bitmap bit order**, and does bit 0 = block 1? | Needed for orphan-block and undelete features. Not needed for plain extraction | Compare with a freshly `dinit`-ed disk, where only the first 4 blocks are used | P2 | partly: 1 bit per block confirmed (200 bytes for 1600 blocks). Bit order still open |
| FS-6 | **The `/lib/fsys.h`, `io.h`, `dev.h` and `task.h` headers themselves** | Would settle FS-1 to FS-5 and EXE-1 straight from the source | Extract them from the C Compiler Diskette 3.00.00 once a basic reader works | P1 | found: `/lib/fsys.h`, `task.h`, `io.h`, `dev.h`, `magic.h` and others are on the C Compiler 3.00 diskette. `fsys.h` confirms `xtnt_hdr`, `dir_entry` (with `fspare[6]`) and `dir_xtnt`. Headers not yet transcribed into the spec |
| FS-7 | **64 MB hard disk layout:** the master partition record (up to 4 entries), the **Bad Block Table** format and the loader area | Without the BBT remap, hard-disk images return garbage in remapped blocks | Needs a 64 MB fileserver hard-disk image (archeobits has imaged server drives; see PPL-1) | P2 | partly: the 10 MB layout is now confirmed from a real `configure` run in the emulator: block 1 holds the loader pointer, the geometry record (+10), the BBT record (+30) and a **PC-style partition table** (+1BE, AA55). The QNX 2 partition is **type 0x07**, and its start field is a **1-based** block number (start 30 → partition block 1 at LBA 29). Real 64 MB geometry still unknown |
| FS-8 | **The 10 MB "old" vs "new" format:** 20808 blocks (old) vs 20208 (the 4.x glossaries) | Tells hard-disk images apart | A 10 MB hard-disk image, or the 4.x `dinit` code | P2 | ✅ new-format 10 MB = 306×4×17 (20,808 blocks) with a geometry record; one 20,779-block partition (icon-emu docs/INSTALL.md) |
| FS-9 | **The `arch` multi-volume archive format** | Some floppies may be `arch` volumes, not filesystems | Find an `arch` disk among the images, or create one on an emulator | P3 | open |
| FS-10 | **Is the 800 KB 10-sector format ever used** on real media? | Image-size detection | Survey image sizes in Disk_1 | P3 | ✅ all 34 YorkU QNX disks are **800K** (80×2×10). The 2 MS-DOS disks are 360K FAT12. No 640K disk yet |
| FS-11 | **Sector stagger** of 2 or 3 (the manual contradicts itself) | Only matters for decoding raw flux | Read the sector IDs from the KryoFlux streams | P3 | open |

## 2. Executables and object files

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| EXE-1 | **Load-module (executable) header format**, including the `Code_flags` bits (shared, setuid, remote_ok) | Needed to identify, list or emulate programs. "Available to registered developers" only | `/lib/task.h` (FS-6) and comparison of several `/cmds` binaries | P2 | open |
| EXE-2 | **`.o` object format** and the **library directory/index file** format | Developer tooling and identifying files | Look at `/lib` on the C Compiler diskette | P3 | open |
| EXE-3 | **Contents and format of `ws_boot`, `hd_boot` and `fs_net_boot`** (the images the ROM loads at `3000:0000`) | The link between ROM and OS, and needed for emulation | Extract them from a 4.x fileserver boot diskette and disassemble them | P2 | partly: boot images have a 0x40-byte header plus a stub that expands a 9Dh zero-run RLE body from file 0x180; a task table at image 0x138 lists task, CS and size. Kernel boot-parameter block still unknown |

## 3. Firmware and hardware

Analysis: [ICON_firmware.md](ICON_firmware.md).

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| FW-1 | **The keyboard firmware.** The current dump is bad: the 8035 is ROM-less and the program is on an external EPROM (70109-135D?) | Needed for keyboard emulation (scancodes) | Re-dump the external EPROM on the keyboard board | P2 | partly: protocol decoded from the 4.10 kernel: parallel byte at F920 (complemented), PA7 → INT0, write to ack, codes 01–66 make / +80h break / 7Fh repeat tick, full keymap (re_notes/input_video.md). Only the MCU firmware and repeat rate are still missing |
| FW-2 | **The "Rel. 2.11" ROM pair** (U33 LOW chk 2373, U36 HIGH chk 048C) | It probably adds larger hard-disk support and is what the LEXICON and later ICONs run | Dump it from the LEXICON or the 1 MB ICON board (Eckert, vintagecomputer.ca or archeobits) | P2 | open |
| FW-3 | **The ICON2 boot ROM.** It's a different ROM, with codes 40, a1 and fa–fc | ICON2 emulation | Dump it (archeobits owns an ICON2) | P2 | open |
| FW-4 | **NVRAM words 1–4 and 13–15.** Also word 0 bits 3, 4 and 5–11, and the meaning of the word 11 nibbles | Needed to build a working NVRAM image for an emulator | Trace the rest of the ROM, plus `nv_config` and `attach_net` from a disk image. A real NVRAM dump (93C06/NMC9306) would be best | P1 for emulation | ✅ all 16 words mapped from the kernel `int 5Ch` NVRAM functions; known-good images for a fileserver and a workstation (re_notes/nvram_rtc_disk_install.md §1). A few word 0/13 bits are still open |
| FW-5 | **Video hardware:** frame-buffer address and plane layout (the ROM writes text at segments 3ABA, 5ABA and 7ABA), what F900–F906 are for, and the CRTC values | Needed for the display in an emulator | Trace the ROM text routine; ICON2 modes are in manual_notes/devtools.md | P2 | mostly: linear 80×240 frame buffer in RAM, base from NVRAM word 0 bits 0–1 (3ABA0/5ABA0/7ABA0/1ABA0), page flip = F983 bit 0, mode = F983 bit 2, F900–F906 = RGBI palette, CRTC defaults 38 28 2C 44 41 00 3C 3D 00 03. 2 bpp bit order inferred |
| FW-6 | **Trackball and baud-rate counters** (codes 2b–30): which ports and chips? | Needed for an emulator; MAME lacks the trackball | Trace the code under those error codes | P2 | ✅ the trackball uses two Microwire counter chips (COP452-like) on the NVRAM serial bus, selected by F983 bits 7..5 (000 = Y, 010 = X), polled by timer 2 at ~40 Hz. Baud-rate counter select still open |
| FW-7 | **What PPI A port A bits 0–4 (board straps) mean**, including the values 00/1D/1E/1F that turn on the PPI B tests | Tells LEXICON, standalone and network boards apart | Photos of the board or schematics, plus the ROM | P3 | open |
| FW-8 | **What the ROM-loader slot (FFFC0+10) was for, and the empty "FUTURE" sockets at E0000–F7FFF** | Historical: a ROM-resident OS? | The Rel. 2.11 ROM (FW-2), or CEMCORP documents | P3 | open |
| FW-9 | **`MARTHAHANSMS` signature** at ROM offset 7FD4 | Historical | Ask former CEMCORP staff | P3 | open |
| FW-11 | **How were 800K (10-sector) boot disks booted?** The 5053 ROM's floppy loaders hard-code 8 sectors/track, so they can only read blocks 1–8 correctly on a 10-sector disk. Yet the fileserver boot disks are 800K, with their root directory at block 12/15 after a loader. Either the LEXICON's Rel. 2.11 ROM handles 10 spt, or the loader fits in blocks ≤ 8 and reads the rest itself | Says which ROM booted fileservers. Also important for an emulator | Read the block-1 +2/+4 fields on the boot disks: is start+count ≤ 9? Then dump Rel. 2.11 (FW-2) | P2 | ✅ **corrected:** the boot loader is laid out for the ROM's 8-sector mapping (linear blocks 9–10 are empty), so the stock 5053 ROM boots 800K disks. Confirmed in the emulator (QNX 2.05b to Login:) |
| FW-10 | **Schematics / the CEMCORP ICON Hardware Manual** | Would answer FW-4 to FW-7 in one go | Search archives and ask contacts (PPL) | P2 | open |

## 3b. Hardware inventory

Inventory: [ICON_hardware.md](ICON_hardware.md).

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| HW-1 | **ICON2 CPU and clock speed** (it runs MS-DOS code natively, so it's x86; is it an 80186 or an 80286?) | Emulation | Disassemble `ws2_boot` and `machine_test` (the `+c` code); ICON2 board photos | P2 | open |
| HW-2 | **ICON3 hardware:** CPU, RAM, video, and how `set_cpu_speed` works | Completeness and emulation | Disassemble `ws3_boot` and `set_cpu_speed`; archeobits owns one | P3 | open |
| HW-3 | **Exact PW2 300 / PW2 800 models** behind "ICONNET 286 / 386" | Fileserver history | Unisys PW2 product literature | P3 | open |
| HW-4 | **The `machine_test` return codes in full** (`+c`, `+b`, `+k`, `+e`), and where it reads them from (NVRAM? the ROM?) | Would fill in NVRAM words 1–4 and 13–15 (FW-4) | Disassemble `/sys/machine_test` (v91.08) and `/sys/check_machine` | P1 for emulation | partly: `machine_test` and `check_machine` go through kernel `int 5Ch` NVRAM calls; codes come from NVRAM word 0 and word 12 (re_notes/nvram_rtc_disk_install.md) |
| HW-5 | **The `/config/hw_dbase.*` hardware database format** (an indexed database with `.kya`–`.kyc` keys) | It may record each node's hardware | Dump it from a configured fileserver image | P3 | open |
| HW-6 | **The ICON2 I/O Module and "external card"** (`machine_test +e`) | Completeness | Manuals or hardware | P3 | open |

## 4. Boot and network

Docs: [ICON_boot_and_network.md](ICON_boot_and_network.md), ICON_firmware.md §6.

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| NET-1 | **The server side of the network boot:** `/sys/bootsnd` and `bootrec`, the meaning of the 5 skipped reply bytes, `arg0`/`arg1`, and the image selector values (NVRAM word 12) | Needed to emulate a station booting from a fileserver | Disassemble `bootsnd` from a disk image, or capture a real boot | P1 for emulation | partly: `bootrec` → `bootsnd` broadcasts the image chosen from `/config/boot_config` by type (ICON_firmware §6b). The byte layout of the reply header still needs disassembly of `bootsnd` |
| NET-2 | **The QNX 2 network message protocol after boot** (remote disk `remdisk`, tasks, `[node]` paths) | Multi-node emulation | Disk-image binaries; QNX 2 documentation | P3 | open |
| NET-3 | **The `net.config` format, and a full example `sys.init` / `u_sys.init.<hex node>`** | Needed to reconstruct and configure a network | Read them from a configured fileserver image (archeobits) or a Configuration diskette | P2 | partly: `boot_config` format known (selector → file). `configure` runs on a workstation and executes on node 1 remotely; the fileserver console isn't used |
| NET-4 | **What drive 8 (the RAM disk) is for in the boot path.** Is the station OS image really served from it, as [JE1] says? | Fixes the boot story | Check `sys.init` on a fileserver image | P3 | open |
| NET-5 | **Netcard (PC ARCNET card) fileserver and workstation boot:** the PC boot sector on the Netcard boot diskette | Explains the PC-hosted servers of the 4.x era | Compare sector 0 of the ICON and Netcard boot diskettes | P3 | open |

## 5. Releases and software

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| REL-1 | **The 4.10 series** (4.10.01 Exception Release, the Upgrade Release, the 4.10.05 patch): what changed, and the role of the "Upgrade" diskettes | None of the manuals cover it | Disk_1 contents, and the release notes on the disks | P2 | open |
| REL-2 | **Which QNX 2 version each ISSSW release is built on** (3.00 = QNX 2.05b per one source; what about 4.x?) | Explains format differences | Version strings in the images' kernel or `/boot` files | P3 | open |
| REL-3 | **The System 5.00.02 (QNX 3.15g?) / ICON 3 era** | Covers the end of the timeline | archeobits (System 5.00.02 server) | P3 | open |
| REL-4 | **Damaged images in Disk_1** (marked "bad sectors"): 4.01.00 fileserver boot, 4.10.01 Upgrade 2 and 3, Configuration, Help, WATCOM BASIC | Some may be recoverable from the second copies or by re-reading the KryoFlux streams | Compare "Copy 2" images sector by sector; merge the good sectors | P2 | partly: all "(bad sectors)" .img files export. Flux fully recovers the 4.01 NetCard boot disk (37 vs 15 entries); flux gives 12 vs 17 files on one upgrade disk (prefer the .img); 4.10.01 Upgrade 3 sector 164: flux copy has a valid CRC. No per-file damage report yet |
| SW-1 | **Imaging status of the 220 catalog titles** | Preservation tracking | Fill in the CSV's have_media, imaged and boots columns as images turn up | P2 | ongoing |
| SW-2 | **OCR-uncertain titles** (dBase version, "Smart (1986)") | Catalog accuracy | Check the PDF page images | P3 | open |

## 5b. RustyBackup reader: feature backlog

Status as reported on 2026-10-28 (`src/fs/qnx2.rs`). Things already working: detection by structure, the root at block 1+68, extent chains, subdirectories, correct file sizes, FAT12 hand-off, and tolerance of bad-sector .img and KryoFlux images (34 QNX disks, 2,498 entries, all exporting).

| ID | Missing feature | Depends on | Status |
|---|---|---|---|
| RB-1 | Decode dates (`fdate`), using the §6 layout in the spec | FS-3: check the layout against known release dates (e.g. 4.10.01 = Sep 1991) | open |
| RB-2 | Show the owner as `group.member` (`fgroup`.`fowner`) | — | open |
| RB-3 | Map the attribute and permission bits (`meawr` / `rcbm`) | — | open |
| RB-4 | List and recover deleted entries (`fstat & 0x40` clear) | — | open |
| RB-5 | Optional RS (0x1E) → LF conversion on extract | — | open |
| RB-6 | Bitmap cross-check and orphan report, growing into a QNX 2 fsck | FS-5 | open |
| RB-7 | Hand off QDOS `.pf`/`.ph` image files and `arch` floppies | FS-9 | open |
| RB-8 | Per-file report of damaged sectors | REL-4 | open |
| RB-9 | Hard-disk images: a 10 MB single volume, and 64 MB with the partition record and BBT | FS-7, FS-8; needs sample images (PPL-1) | open. Concrete test case: an installed 10 MB disk (icon-emu `disks/lexicon-4.10.01-10mb.img`). RustyBackup currently reads partition type 0x07 as NTFS; it must probe QNX 2 and treat the partition start as 1-based. Extracted from LBA 29, it reads fine |
| RB-10 | Test on 640K disks and LEXICON-made disks | needs samples | open |
| RB-11 | Optionally use a size table (655,360 / 819,200 bytes) as a fallback when the bitmap is damaged | — | open |

## 6. People and leads that could close several gaps at once

| ID | Lead | Could close |
|---|---|---|
| PPL-1 | **archeobits (Reddit) / "edgecase" (Giant Bomb)**, a Toronto collector who reportedly has ROM dumps, a memory map, a working LEXICON, an ICON 1/2/3, server hard-disk images and Archives of Ontario floppies | FW-2, FW-3, FW-4, FS-7, FS-8, NET-3, REL-3 |
| PPL-2 | **The York University Computer Museum (YUCoM)**, the source of this material. They may have more: hardware, the original boards for re-dumping, or other manuals | FW-1, FW-2, FW-10 |
| PPL-3 | **Jason Eckert** (owns the LEXICON with the Rel. 2.11 ROMs) | FW-2 |
| PPL-4 | **The Archives of Ontario** ICON-era floppies | SW-1, REL-2 |
| PPL-5 | **The manuals known to exist but not in hand:** the *CEMCORP ICON Hardware Manual*, *QNX for the ICON*, the *ICON User's Guide*, and the *MS-DOS Reference Manual* (1215969) | FW-10, NET-2 |

## 7. Resolved (kept for reference)

| Former question | Answer | Where |
|---|---|---|
| Where is the node ID stored? | NVRAM word 10, high byte; the low byte is the boot server | ICON_firmware.md §5 |
| How does a station choose its boot source? | NVRAM word 0 bits 12–15. The order is floppy → network → hard disk | ICON_firmware.md §4 |
| What is in `/boot`? | `ws_boot`, `hd_boot`, `fs_net_boot` (formats still open: EXE-3) | ICON_disk_layout.md §2b |
| ICON boot selector → boot image | `/config/boot_config`: 0 = fileserver, 1/2 = ICON, 3 = ICON3, 16/18/20 = ICON2, 22–24 = Netcard PC | ICON_hardware.md §1 |
| Installing 4.10.01 onto a hard disk | Done in the emulator: 4.01 fileserver boot diskette, 4.10 ws_boot (Netcard boot disk), 4.10.01 Configuration + SW 1–5. The 4.01 workstation kernel has an NVRAM/trackball interrupt race that makes `configure` mistake the workstation for a storage fileserver; 4.10 fixed it with cli | icon-emu docs/INSTALL.md |
| Extent size units / file length | `size_xtnt` = bytes; length = sum of the extents' sizes | ICON_qnx2_filesystem.md (update box) |
| Block size and floppy geometry | 512 bytes; 80×2×8 = 1280 blocks | ICON_qnx2_filesystem.md §1, §8 |
| The directory entry layout | 48 bytes, verbatim from `/lib/fsys.h` | ICON_qnx2_filesystem.md §3 |
| ROM self-test error codes | The full table, 09–5a | ICON_firmware.md §8 |
