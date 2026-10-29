# Handoff to RustyBackup: a real ICON hard disk (Seagate ST-251)

This follows `rustybackup_handoff_hd_emulator.md`, which describes the hard-disk layout. That layout has now been confirmed on a disk read from real hardware. This drive is the best real-world test case so far.

## The files (from the user)

| File | Size | SHA-256 | What it is |
|---|---|---|---|
| `ICON4_ST251_final.emu` | 102,572,354 | `ae87461ac50452f1acf02fce84b9c5a7fa493f8fd5b5183d6cee67af6f3a18ed` | Flux-level capture, David Gesswein MFM reader/emulator format (`--heads 6 --cylinders 820 --sectors 17,0 --format Intel_iSBC_214_512B`) |
| `ICON4_ST251_final.bin` | 57,098,240 | `2aacb80754b53209b597357a649cbe24dede51f58dcedf70c26b5f2dd7aa0092` | Sector data, **extracted as 820 × 8 × 17**: heads 6 and 7 of every cylinder are zero padding |
| `icon4-st251-4.10.01.img` (made from the `.bin`) | 42,823,680 | `b0d5ecb1734feb0ff244e477ed5a47a0460ab959204feb68c4e0cbe1e59d1d4b` | The same data as a plain 820 × 6 × 17 image |

The real geometry is **820 cylinders × 6 heads × 17 sectors = 83,640 blocks**.

## What RB should handle

1. **Padded-head images.**
   - When block 1's geometry record says *h* heads (byte +13) and *T* total blocks (u32 at +14), and the file is exactly `T / (h × 17) × 8 × 17 × 512` bytes, then each 8-head cylinder holds *h* real heads followed by padding.
   - Keep the first `h × 17` blocks of every `8 × 17`. icon-emu now does this on load.
2. **Partitions:**

   | Partition | Type | Start field | LBA | Length | Notes |
   |---|---|---|---|---|---|
   | 1 | **07** | 60 | **59** | 65,535 blocks | QNX 2 volume, ICON 4.10.01 |
   | 2 | **08** | 65,595 | **65,594** | 18,046 blocks | Never formatted (all zero). Report it as empty, not as an error |

   The start formula is 2 + BBT length + 14 + spares + 2. Here the BBT is 1 block and there are 41 spares (BBT record byte +33).
3. **The bad block table** is at block 2: 8-byte entries, each a u32 bad block number and a u32 replacement block number.
   - Bad block 0 means the entry is unused.
   - On this drive every entry is unused, so no remapping is needed. RB should still apply any entries it finds.

## Expected results (to test against)

- **Partition 1:** 431 files and 20 directories, including the root, `bitmap` and `user/icon`, `user/teacher`. The `fsck` count, taken from a carved copy with today's rb-cli, is 431 files / 20 dirs.
- **Every file is dated 1980:** the install was done with the clock unset. That is correct data, not a date-decoding bug.
- **Root files:** `QNX_1`–`QNX_5`, `package` and `convert_4.02` are in the root, plus the normal system tree.
  - The other 425 files are byte-identical to the 4.10.01 install in `lexicon-4.10.01-10mb.img`, apart from `/bitmap`.
  - That image also has `/config/net.config`, `/tmp` and `/user_cmds`, which this drive lacks.
- **QNX's own view:** icon-emu boots the drive (raw `.bin` or repacked `.img`) and logs in. `query 1` then reports `58698 free, 6838 used of 65536 total blocks`. That run used a copy, so its block count includes files the boot itself writes.
