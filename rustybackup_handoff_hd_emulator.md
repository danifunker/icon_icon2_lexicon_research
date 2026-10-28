# Handoff to RustyBackup: ICON/LEXICON hard disks, and verifying QNX 2 writes with icon-emu

This is for the RustyBackup (RB) agent. It is self-contained: you don't need the ICON research repo to act on it.

It covers three things:
1. How LEXICON fileserver hard disks (10 MB and 64 MB) are laid out, so RB can find and open their QNX 2 volumes.
2. How to use **icon-emu**, an emulator of the real ICON/LEXICON hardware that runs the real QNX 2.05b (ICON 4.10.01), as an oracle. RB writes to an image, then real QNX checks and reads it.
3. What RB does today with these disks, as measured with `rb-cli` built 2026-10-28.

Terms used below:
- **Block** = a 512-byte sector. QNX 2 numbers blocks from **1**, so block N is at byte offset (N − 1) × 512.
- **LBA** = 0-based sector index.

## 1. Hard-disk layout

All values below were written by the real ICON tools (`hdformat`, `bbt`, `fdisk`, `dinit` from the 4.10.01 Configuration diskette), running in the emulator.

### Physical block 1 (LBA 0): the disk header

| Offset | Size | 10 MB disk | 64 MB disk | Meaning |
|---|---|---|---|---|
| +00 | u16 | 0 | 0 | — |
| +02 | u16 | 3 | 4 | Loader start block (= 2 + BBT length) |
| +04 | u16 | 14 | 14 | Loader length in blocks |
| +06 | u16 | 18 | 18 | Unknown (constant) |
| **+10** | 32 bytes | `02 01 01 04 …` | `02 01 03 08 …` | **Geometry record** (below) |
| **+30** | 16 bytes | `02 00 00 0B 00 02 00 00 00 01 … 03` | `02 00 00 45 00 02 00 00 00 02 … 03` | **Bad-block-table record** (below) |
| **+1BE** | 4 × 16 | 1 entry | 3 entries | **PC-style partition table** |
| +1FE | u16 | `55 AA` | `55 AA` | Signature |

**Geometry record at +10.** This is how to recognise an ICON hard disk: byte +10 = `02` and byte +2F = `03`.

| Offset | Size | 10 MB | 64 MB | Meaning |
|---|---|---|---|---|
| +10 | u8 | 02 | 02 | Record type |
| +11 | u8 | 01 | 01 | ? |
| +12 | u8 | 01 | 03 | Probably the high byte of the last cylinder (305 = 0x131, 1023 = 0x3FF). Unconfirmed |
| +13 | u8 | 4 | 8 | Heads |
| +14 | u32 | 20,808 | 139,264 | Total blocks on the drive |
| +18 | u32 | 68 | 136 | Blocks per cylinder |
| +1C | u16 | 17 | 17 | Sectors per track |
| +24 | u32 | 3 | 4 | Loader start block (again) |
| +28 | u32 | 14 | 14 | Loader length |
| +2F | u8 | 03 | 03 | End marker |

Cylinders = total / blocks per cylinder: 306 for 10 MB, 1,024 for 64 MB. The 64 MB geometry, 1024 × 8 × 17 (Micropolis 1335 class), is what we gave `hdformat`. The original drive's exact geometry is still unconfirmed.

**BBT record at +30.**

| Offset | Size | 10 MB | 64 MB | Meaning |
|---|---|---|---|---|
| +30 | u8 | 02 | 02 | Record type |
| +33 | u8 | 11 | 69 | Number of spare blocks for remapping |
| +35 | u16 | 2 | 2 | BBT start block |
| +39 | u16 | 1 | 2 | BBT length in blocks |
| +3F | u8 | 03 | 03 | End marker |

So the disk is laid out as:

```
block 1        header (this table)
block 2..      bad block table (1 or 2 blocks)
then           loader, 14 blocks (the ROM loads it; QNX's HDos_ldr)
then           spare blocks (11 or 69)   <- inferred from the counts; layout fits exactly
then           2 unused blocks
then           partition 1
```

10 MB: 2 + 1 + 14 + 11 + 2 = **30**. 64 MB: 2 + 2 + 14 + 69 + 2 = **89**. Both equal the first partition's start.

On freshly formatted disks the BBT has no entries, so there is no remapping to handle yet. The entry format of a non-empty BBT is unknown. If RB ever meets a BBT block that isn't all zero, it should warn rather than guess.

**Partition table at +1BE.** It uses the PC layout: boot flag, CHS start, type, CHS end, u32 start, u32 length. Two traps:

- **The types are QNX 2's: `07`, `08` and `09`.** These are partitions 1, 2 and 3; the type number is the partition's slot, not a filesystem ID. Type 07 is **not NTFS** on these disks.
- **The start field is a 1-based block number.** The partition's first sector is at **LBA = start − 1**. The length is in blocks.

| Disk | Entry | Type | Start (field) | LBA | Length | Mounted as |
|---|---|---|---|---|---|---|
| 10 MB | 1 | 07 (boot) | 30 | 29 | 20,779 | drive 1 |
| 64 MB | 1 | 07 (boot) | 89 | 88 | 65,535 | drive 1 |
| 64 MB | 2 | 08 | 65,624 | 65,623 | 65,535 | drive 3 |
| 64 MB | 3 | 09 | 131,159 | 131,158 | 8,106 | not mounted by default |

- Drive 2 is the floppy.
- QNX 2 volumes max out at 32 MB, which is why a 64 MB disk is split.
- Each partition is a complete, ordinary QNX 2 volume. Carved out on its own, it starts with its own block 1 (root directory) and block 2 (bitmap), exactly like a floppy. All three partitions above carve out and list cleanly in RB.
- QNX's `query` reports 65,536 total blocks for a 65,535-block partition, which looks like an off-by-one in its own count. RB should trust the partition table's length.

**Detection order RB should use:** check the geometry record first (+10 = 02 and +2F = 03, plus 55 AA). If it's there, treat partition types 07/08/09 as QNX 2, use start − 1, and probe each partition for QNX 2 before trusting the type byte.

### Making these disks yourself

`hdformat 1 c=1024 h=8 -p l=/sys/HDos_ldr; mount disk 2 /drivers/hd p=1; bbt 1 +u <$null; mount disk 2 d=2 p=1; fdisk 1 +p`, then `dinit` on each partition. icon-emu has this as `scripts/prep-64mb.txt`, and its result as `disks/hd64-factory.bin`, a sparse template of the 22 non-zero blocks.

## 2. Test images

These come from the user (they can't be published: they hold copyrighted QNX/Unisys software). Check them with SHA-256:

| File | Size (bytes) | SHA-256 | Contents |
|---|---|---|---|
| `lexicon-4.10.01-10mb.img` | 10,653,696 | `c61413e729ff5b616fd972e7397d5d543f655b35d0bc1f6c5fb1305feaf058fd` | 306×4×17, ICON 4.10.01 installed: 450 files incl. 23 dirs, 6,804 blocks used |
| `lexicon-4.10.01-10mb-help.img` | 10,653,696 | `990708b4ea54c1a4180ef1dc7a5ed15b51eacb24ba2a457ec9b66bf8005d91d5` | The same plus the Help package |
| `lexicon-4.10.01-64mb.img` | 71,303,168 | `e5e42abe46b3fc352723ef1579ec85f9bc8807596ace5bd90e42f698bf328baa` | 1024×8×17, 4.10.01 on partition 1 (drive 1); partitions 2 and 3 freshly `dinit`ed |
| `hd64-factory.bin` | 11,364 | `a1f5d88d74a744c3bb10a10c145db4e2b2640bb61c401f34402fa03ec39c44b1` | The prepared-but-empty 64 MB disk, sparse: `"ICONHD64"`, u32 size, then (u32 block index, 512 bytes) per non-zero block |

## 3. What RB does today (rb-cli, 2026-10-28)

| Check | Result |
|---|---|
| `rb-cli ls lexicon-4.10.01-64mb.img /` | Asks for a partition and lists **1 = "NTFS/HPFS/exFAT"**, 2 and 3 = "Unknown". It should say QNX 2 for all three |
| Carve at LBA 88 / 65,623 / 131,158 → `rb-cli ls` | All three read correctly as QNX 2 (partition 1 shows `bitmap`, `config`, `tmp`, `cmds`, `util`, …) |
| 10 MB, carved at LBA 29 → `ls`, `get`, `fsck` | Correct. `fsck`: 427 files / 23 dirs, which matches QNX's `chkfsys` (450 "files", counting the dirs) |
| `rb-cli put` / `mkdir` on QNX 2 | **`unsupported: editing not yet supported for filesystem type 'qnx2'`** |
| A file QNX wrote in the emulator → `rb-cli get` | Byte-exact. A 2,014-byte binary copied by QNX matches the original; text from `echo` is `hello from qnx` + `1E` (QNX's RS line end) |

So the next RB work items are:
- (a) recognise the ICON hard-disk header and the 07/08/09 partitions, with start − 1;
- (b) QNX 2 write support;
- (c) verify (b) with the emulator, as follows.

## 4. Verifying RB's writes with icon-emu

icon-emu is a Rust emulator of the ICON's 80186 board: WD1002 disk controller, ARCNET, NVRAM, video, keyboard. It boots the real ROM and real QNX from these images. Its headless mode lets a program log in and run shell commands, and read their output as text. That makes real QNX the judge of whether RB wrote a valid filesystem.

### Getting it

icon-emu has no public repository. The user will give you the repo, for example a git bundle (`git clone icon-emu.bundle icon-emu`), plus the ROM file `icon1_rom_combined.bin` (32,768 bytes, SHA-1 `5d7f7b173f599edf1f0e8655852c3adafa61fd89`). Then:

```
cd icon-emu
cargo build --release -p icon-cli      # gives target/release/icon-cli and target/release/icon-ci
```

You need Rust stable; the repo pins it with `rust-toolchain.toml`. The emulator itself needs nothing else.

### One-shot: `icon-cli session`

This is best for tests. It boots the fileserver from the disk, logs in as `root` / `superuser`, runs each `--cmd` in order and prints the output, then optionally saves the disk:

```
icon-cli session --rom icon1_rom_combined.bin --hd test.img \
    --cmd "chkfsys 1" --cmd "files 1:/user/rbtest" --cmd "cat 1:/user/rbtest/hello.txt" \
    [--save-hd after.img] [--transcript log.txt]
```

- Output comes as `=== CMD <command>` blocks, ending with `=== END`. A timeout exits with status 1.
- It takes about 5–10 s of wall time per run. It is deterministic: same image and commands, same output, apart from the clock.
- The input image is never modified; use `--save-hd` to get QNX's changes.

### Interactive or long-running: `icon-cli serve` + `icon-ci`

This works like a CI mode: a server holds the machines, and you send commands over a Unix socket.

```
icon-cli serve --socket /tmp/icon.sock --rom icon1_rom_combined.bin --hd test.img &
icon-ci --socket /tmp/icon.sock power 0        # boot the fileserver, wait for Login:
icon-ci --socket /tmp/icon.sock login
icon-ci --socket /tmp/icon.sock run chkfsys 1
icon-ci --socket /tmp/icon.sock hd-save after.img
icon-ci --socket /tmp/icon.sock quit
```

- The wire format is one JSON object per line: `{"cmd":"run","args":{"slot":0,"command":"chkfsys 1"}}` gets back `{"ok":true,"data":{"output":"…"}}`. You can talk to the socket directly from Rust test code.
- `icon-ci script FILE` runs a file of commands.
- In the repo, `docs/HEADLESS.md` has the full command list. `icon_core::session::Session` is the same thing as a library, if you'd rather link it.

### Recommended verification recipe

For each RB write operation (put, mkdir, rm, mv, overwrite, grow a file across extents, fill a directory past one block):

1. Copy a test image and carve partition 1 (10 MB image: LBA 29, 20,779 blocks). Or have RB write through the partition table once (a) is done.
2. Do the operation with RB. Splice the partition back at the same offset if you carved it.
3. Run real QNX over it:
   - `chkfsys 1`: must end with `Rebuilt bitmap agrees with existing bitmap` and report no errors. This is the main check.
   - `dcheck 1`: optional, for bad blocks.
   - `files 1:/path`: QNX's own listing of what RB created (blocks, date, name).
   - `cat 1:/path/file`, or `wc`, `size`: QNX reads the contents back.
   - `diff 1:/path/a 1:/path/b`: compare against a copy QNX made itself (`copy`).
4. Reverse direction: have QNX write (`mkdir`, `echo text >1:/x`, `copy 1:/cmds/date 1:/x/date.copy`), save with `--save-hd`, then have RB `get` and `fsck`. This has been done once by hand and gave byte-exact results (section 3).

Practical notes:
- **Prefer short, deterministic commands.** The emulator reads text off the emulated screen. Plain output is exact, but very long, fast-scrolling listings (`chkfsys` on a busy disk prints hundreds of lines) can occasionally drop or garble a line. A stray `_` line is the cursor. Assert on the summary lines (`Rebuilt bitmap agrees…`, `Total blocks: N   Total files: N`), not on whole transcripts.
- QNX keeps a disk cache. `--save-hd` and `hd-save` capture what has reached the disk. Running one more command after the last write (e.g. `chkfsys 1`) before saving has been enough in practice.
- Line ends in QNX text files are RS (0x1E); `echo` writes one.
- Commands available in `/cmds` include: `cat chkfsys copy date dcheck diff dinit files ls mkdir query size wc zap`.
- Paths take a drive prefix: `1:/user/…`. On the 64 MB image, the second 32 MB partition is drive `3:`.
