# Known Gaps

A single list of what we still don't know, with what would close each gap. Every item has an ID so commits and notes can point to it (for example "closes FS-2"). When something is resolved, set the **Status** to ✅ and say where the answer now lives.

**Priority:** **P1** blocks the RustyBackup QNX 2 reader or a bootable emulator · **P2** is needed for a complete picture · **P3** is nice to have.

**Most of the P1 filesystem gaps should fall to the first readable floppy image.** A good test disk is a clean 640 KB one, such as the 4.10.01 Help diskette or the C Compiler 3.00 diskette.

---

## 1. Filesystem: what the RustyBackup reader needs

Spec: [ICON_qnx2_filesystem.md](ICON_qnx2_filesystem.md).

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| FS-1 | **Is `size_xtnt` (and `bound_xtnt`) counted in blocks or bytes?** | Getting this wrong truncates or overruns every multi-block file. `q2files.c` treats it as bytes, which may be its "sometimes crashes" bug | Find a file with one extent and more than one block on a real image, and compare `size_xtnt` with `fnum_blks` | P1 | open |
| FS-2 | **What's in block 1, bytes 0–67:** format-table offsets (heads, spt, tracks) and the root name prefix (`dinit p=`) | Needed to pick the geometry and to recognise non-`/` roots | Hex-diff block 1 across several disks with different formats and boot flags. Only +2/+4 (the boot start and count) are known, from the ROM | P1 | open |
| FS-3 | **What are `fspare[6]` (bytes 17–22)?** Is it `fnum_chars_free` + `fseconds` as in `q2files.c`, or spare as the 1987 header says? | The logical file size formula depends on it | Compare 3.00 and 4.x disks. Check against `/lib/fsys.h` on the C Compiler diskette | P1 | open |
| FS-4 | **Directory header:** is `parent_xtnt` really 16-bit? What does `dir_index` mean, and how is the extended form (bit 15) used? | Needed to walk directories correctly | Dump a root directory and a subdirectory | P1 | open |
| FS-5 | **Bitmap bit order**, and does bit 0 = block 1? | Needed for orphan-block and undelete features. Not needed for plain extraction | Compare with a freshly `dinit`-ed disk, where only the first 4 blocks are used | P2 | open |
| FS-6 | **The `/lib/fsys.h`, `io.h`, `dev.h` and `task.h` headers themselves** | Would settle FS-1 to FS-5 and EXE-1 straight from the source | Extract them from the C Compiler Diskette 3.00.00 once a basic reader works | P1 | open |
| FS-7 | **64 MB hard disk layout:** the master partition record (up to 4 entries), the **Bad Block Table** format and the loader area | Without the BBT remap, hard-disk images return garbage in remapped blocks | Needs a 64 MB fileserver hard-disk image (archeobits has imaged server drives; see PPL-1) | P2 | open |
| FS-8 | **The 10 MB "old" vs "new" format:** 20808 blocks (old) vs 20208 (the 4.x glossaries) | Tells hard-disk images apart | A 10 MB hard-disk image, or the 4.x `dinit` code | P2 | open |
| FS-9 | **The `arch` multi-volume archive format** | Some floppies may be `arch` volumes, not filesystems | Find an `arch` disk among the images, or create one on an emulator | P3 | open |
| FS-10 | **Is the 800 KB 10-sector format ever used** on real media? | Image-size detection | Survey image sizes in Disk_1 | P3 | open |
| FS-11 | **Sector stagger** of 2 or 3 (the manual contradicts itself) | Only matters for decoding raw flux | Read the sector IDs from the KryoFlux streams | P3 | open |

## 2. Executables and object files

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| EXE-1 | **Load-module (executable) header format**, including the `Code_flags` bits (shared, setuid, remote_ok) | Needed to identify, list or emulate programs. "Available to registered developers" only | `/lib/task.h` (FS-6) and comparison of several `/cmds` binaries | P2 | open |
| EXE-2 | **`.o` object format** and the **library directory/index file** format | Developer tooling and identifying files | Look at `/lib` on the C Compiler diskette | P3 | open |
| EXE-3 | **Contents and format of `ws_boot`, `hd_boot` and `fs_net_boot`** (the images the ROM loads at `3000:0000`) | The link between ROM and OS, and needed for emulation | Extract them from a 4.x fileserver boot diskette and disassemble them | P2 | open |

## 3. Firmware and hardware

Analysis: [ICON_firmware.md](ICON_firmware.md).

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| FW-1 | **The keyboard firmware.** The current dump is bad: the 8035 is ROM-less and the program is on an external EPROM (70109-135D?) | Needed for keyboard emulation (scancodes) | Re-dump the external EPROM on the keyboard board | P2 | open |
| FW-2 | **The "Rel. 2.11" ROM pair** (U33 LOW chk 2373, U36 HIGH chk 048C) | It probably adds larger hard-disk support and is what the LEXICON and later ICONs run | Dump it from the LEXICON or the 1 MB ICON board (Eckert, vintagecomputer.ca or archeobits) | P2 | open |
| FW-3 | **The ICON2 boot ROM.** It's a different ROM, with codes 40, a1 and fa–fc | ICON2 emulation | Dump it (archeobits owns an ICON2) | P2 | open |
| FW-4 | **NVRAM words 1–4 and 13–15.** Also word 0 bits 3, 4 and 5–11, and the meaning of the word 11 nibbles | Needed to build a working NVRAM image for an emulator | Trace the rest of the ROM, plus `nv_config` and `attach_net` from a disk image. A real NVRAM dump (93C06/NMC9306) would be best | P1 for emulation | partly done |
| FW-5 | **Video hardware:** frame-buffer address and plane layout (the ROM writes text at segments 3ABA, 5ABA and 7ABA), what F900–F906 are for, and the CRTC values | Needed for the display in an emulator | Trace the ROM text routine; ICON2 modes are in manual_notes/devtools.md | P2 | partly done |
| FW-6 | **Trackball and baud-rate counters** (codes 2b–30): which ports and chips? | Needed for an emulator; MAME lacks the trackball | Trace the code under those error codes | P2 | open |
| FW-7 | **What PPI A port A bits 0–4 (board straps) mean**, including the values 00/1D/1E/1F that turn on the PPI B tests | Tells LEXICON, standalone and network boards apart | Photos of the board or schematics, plus the ROM | P3 | open |
| FW-8 | **What the ROM-loader slot (FFFC0+10) was for, and the empty "FUTURE" sockets at E0000–F7FFF** | Historical: a ROM-resident OS? | The Rel. 2.11 ROM (FW-2), or CEMCORP documents | P3 | open |
| FW-9 | **`MARTHAHANSMS` signature** at ROM offset 7FD4 | Historical | Ask former CEMCORP staff | P3 | open |
| FW-10 | **Schematics / the CEMCORP ICON Hardware Manual** | Would answer FW-4 to FW-7 in one go | Search archives and ask contacts (PPL) | P2 | open |

## 4. Boot and network

Docs: [ICON_boot_and_network.md](ICON_boot_and_network.md), ICON_firmware.md §6.

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| NET-1 | **The server side of the network boot:** `/sys/bootsnd` and `bootrec`, the meaning of the 5 skipped reply bytes, `arg0`/`arg1`, and the image selector values (NVRAM word 12) | Needed to emulate a station booting from a fileserver | Disassemble `bootsnd` from a disk image, or capture a real boot | P1 for emulation | partly done |
| NET-2 | **The QNX 2 network message protocol after boot** (remote disk `remdisk`, tasks, `[node]` paths) | Multi-node emulation | Disk-image binaries; QNX 2 documentation | P3 | open |
| NET-3 | **The `net.config` format, and a full example `sys.init` / `u_sys.init.<hex node>`** | Needed to reconstruct and configure a network | Read them from a configured fileserver image (archeobits) or a Configuration diskette | P2 | open |
| NET-4 | **What drive 8 (the RAM disk) is for in the boot path.** Is the station OS image really served from it, as [JE1] says? | Fixes the boot story | Check `sys.init` on a fileserver image | P3 | open |
| NET-5 | **Netcard (PC ARCNET card) fileserver and workstation boot:** the PC boot sector on the Netcard boot diskette | Explains the PC-hosted servers of the 4.x era | Compare sector 0 of the ICON and Netcard boot diskettes | P3 | open |

## 5. Releases and software

| ID | Gap | Why it matters | How to close it | Pri | Status |
|---|---|---|---|---|---|
| REL-1 | **The 4.10 series** (4.10.01 Exception Release, the Upgrade Release, the 4.10.05 patch): what changed, and the role of the "Upgrade" diskettes | None of the manuals cover it | Disk_1 contents, and the release notes on the disks | P2 | open |
| REL-2 | **Which QNX 2 version each ISSSW release is built on** (3.00 = QNX 2.05b per one source; what about 4.x?) | Explains format differences | Version strings in the images' kernel or `/boot` files | P3 | open |
| REL-3 | **The System 5.00.02 (QNX 3.15g?) / ICON 3 era** | Covers the end of the timeline | archeobits (System 5.00.02 server) | P3 | open |
| REL-4 | **Damaged images in Disk_1** (marked "bad sectors"): 4.01.00 fileserver boot, 4.10.01 Upgrade 2 and 3, Configuration, Help, WATCOM BASIC | Some may be recoverable from the second copies or by re-reading the KryoFlux streams | Compare "Copy 2" images sector by sector; merge the good sectors | P2 | open |
| SW-1 | **Imaging status of the 220 catalog titles** | Preservation tracking | Fill in the CSV's have_media, imaged and boots columns as images turn up | P2 | ongoing |
| SW-2 | **OCR-uncertain titles** (dBase version, "Smart (1986)") | Catalog accuracy | Check the PDF page images | P3 | open |

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
| Block size and floppy geometry | 512 bytes; 80×2×8 = 1280 blocks | ICON_qnx2_filesystem.md §1, §8 |
| The directory entry layout | 48 bytes, verbatim from `/lib/fsys.h` | ICON_qnx2_filesystem.md §3 |
| ROM self-test error codes | The full table, 09–5a | ICON_firmware.md §8 |
