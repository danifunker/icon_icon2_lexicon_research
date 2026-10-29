# Resume prompt: 1 MB memory support for the ICON emulator

Paste everything below the line into a new session.

---

Goal: make **icon-emu** emulate an ICON workstation (and, if it falls out, a LEXICON) fitted with the CEMCORP **"1 MEG ICON UPGRADE BOARD"**. With the **Rel 2.11** boot ROM it should pass its self-test without error **50** ("One megabyte upgrade card RAM failure"), network-boot ICON system software 4.10.05, and have the kernel report 1024 KB. No dump of the card's logic exists, so the memory map has to be reconstructed from the ROM and kernel code. Work out what you can; say plainly what is inferred.

## Where things are

- **`~/repos/icon-emu`:** Rust/WASM emulator. Commit straight to `main`, and push only when asked. End commit messages with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
  - `crates/icon-core/src/machine.rs`: the board. `mem_read8`/`mem_write8` hard-code the map: RAM `0..ram.len()`, COM9026 RAM at `D8000–D8FFF` (even bytes), ROM at `F8000–FFFFF`.
    - `Config.ram_kb` is 512 everywhere: `session.rs`, `icon-web/src/lib.rs`, `icon-cli`.
    - An earlier attempt at simply setting 1024 KB failed, because RAM then shadowed the ROM and the network window.
  - `crates/icon-core/src/cpu/i186.rs`: the 80186 core (MAME port). It stores and reads back the chip-select registers UMCS/LMCS/PACS/MMCS/MPCS (`ChipSelects`, `Cpu::chip_selects()`), but doesn't decode memory with them.
  - `crates/icon-core/src/devices/nvram.rs`: NVRAM presets.
    - Workstation word 0 = `EC2E`. Bit 10 is set, which Rel 2.11 needs to find the network RAM.
    - Word 0 bits 0–1 also pick the frame buffer: 3ABA0/5ABA0/7ABA0/1ABA0 for 0/1/2/3 (`Board::frame_base`).
  - Tools:
    - `icon-cli serve` + `icon-ci` (see `docs/HEADLESS.md`).
    - `icon-cli --preset fs --rom R --hd IMG --script FILE --seconds N`, a timed script. It prints each node's CS:IP, NVRAM and boot entry at the end.
    - `icon-cli session`.
  - Debug environment variables: `ICON_NET_LOG=1` (packets), `ICON_DUMP_AT_ENTRY=path` (RAM when the ROM hands over to a boot image), `ICON_WATCH=lo-hi` (writes to a linear range, workstation).
  - Disk images are in `disks/` (gitignored): `lexicon-4.10.01-10mb.img` (base install).
- **`~/repos/icon_icon2_research`:** notes; also commit straight to `main`.
  - **ROMs:**
    - `firmware/ICON1/icon1_rom_combined.bin` is 5053 ISS 2, the old ROM.
    - `firmware/ICON1/icon1_rom_rel2.11_combined.bin` is **Rel 2.11**, SHA-1 `3750cf6f…`.
  - Notes: `ICON_firmware.md` (section "Rel 2.11 boot ROM"), `re_notes/nvram_rtc_disk_install.md` (the NVRAM word 0 table), `ICON_hardware.md` (board photos, 1 MB card), `gaps.md`.
- **YorkU disks:** `/Volumes/Software/ICON_ICON2/From YorkU/UNISYS_ICON/Disk_1/`.
  - **4.10.05 patch disk:** `System 4.10.01/ICON_SERIES_SYSTEM_SW_UPGRADE_DISKETTE_4.10.05_PATCH_RELEASE_NOV_1_1991.img`. Rel 2.11 workstations need its `ws_boot`. To install it on a hard disk, either use the `configure` procedure in `docs/REBUILD.md` §2b, or copy just the boot file: `chattr 1:/boot/ws_boot a=+w ; copy 2:/boot/ws_boot 1:/boot/ws_boot`, with the patch disk in the fileserver.
- **`rb-cli`** (`~/repos/rusty-backup/target/release/rb-cli`) reads QNX 2 volumes, for example to extract kernels for disassembly. Partition 1 of the 10 MB image starts at LBA 29: `dd skip=29 count=20779`.
- **`ndisasm`** is installed. Rel 2.11's code runs at CS=F800, so file offset = IP.

## What is known

**The card (from photos; see `ICON_hardware.md`):**
- It is a **CPU-socket interposer**. The 80186 moves to the card's PLCC socket U34, and the card plugs into main-board U39. It therefore sees all CPU pins, including the chip-select outputs.
- It carries 18 × MB81256 (512 KB + 2, probably parity), 373/245 latches and buffers, '157 address multiplexers, and two delay lines.
- Its decode is in three undumped 20-pin parts: **U30 200-507-000, U31 200-508-000, U33 200-509-000**.
- The main board itself has 512 KB (4 banks × 16 TMS4164).

**NVRAM word 0** (`re_notes/nvram_rtc_disk_install.md`):
- Bit 11 = 0 means a 1 MB board, with bits 0–1 = 1. The suggested 1 MB fileserver value is `9189`.
- The kernel's `nv_config` table maps `0x001` to 1024 KB.
- The kernel sets `machine_test +m` flags 10h–80h (kernel `009C:5857`).
- Bit 10 (Rel 2.11 only): 1 means the network RAM is decoded by the mid-range chip selects.

**Rel 2.11 code to read** (file offsets):
- **`0x0000`, reset:** UMCS=E03D, LMCS=3FF8, PACS=0FBB, MMCS=C1F8, MPCS=90BB.
- **`0x0086`:** if word 0 bit 11 is clear (1 MB), it calls `0x1507` when bit 10 is clear, then `0x1523`.
- **`0x1507`:** copies the 32 KB ROM from F800:0 into RAM at **6000:0**, presumably so it can run from RAM while it remaps memory. Follow where it jumps.
- **`0x1523`–`0x1585`:** the 1 MB card test.
  - `out` to I/O port **CF80**, an address used nowhere else on the main board. It may be a control register on the card; find out what value it writes.
  - Reads NVRAM word 0 and tests bit 10 (0x400).
  - **Bit clear:** sets UMCS=FF38, then tests memory at EF00 (`call 0x797`, bl=0x50 = error 50).
  - **Bit set:** sets UMCS=F83D, MPCS=82BB, MMCS=F5F8, then tests at E700.
- **`0x0A0E`–`0x0A39`:** RAM sizing loop. On failure, if bit 10 is clear it ORs 0xC00 into the word.
- **Network loader `0x3ACD`–`0x3B17`:**
  - Bit 10 set: network RAM segment = `(MMCS & FE00) + 3·((MPCS & 7F00) >> 1)`. That's D800 with the reset values; with MMCS F5F8 / MPCS 82BB it comes out as **F700**.
  - Bit 10 clear: `UMCS & FFC0`.
  - So on a 1 MB board the COM9026 RAM probably moves (to F7000?), and the ROM shrinks from E0000-up to F8000-up.
- The old 5053 ROM sizes memory from NVRAM bits 0–1 at `F88F:090C`. Check whether it supports 1 MB at all.

**The 4.10 kernels** (`/boot/ws_boot`, `/boot/hd_boot`) have their own memory handling for 1 MB. Look for chip-select writes (`FFA0`–`FFA8`) and how they find the frame buffer and the COM9026 RAM. `ICON_firmware.md` / `gaps.md` EXE-3 describe the boot-image format: a 0x40-byte header, a stub, and an RLE body.

## Suggested approach

1. Disassemble the Rel 2.11 1 MB paths above. Write down the chip-select values it programs, and where it expects:
   - RAM: 0–7FFFF on the main board, then the card's 512 KB somewhere above;
   - the ROM;
   - the COM9026 RAM;
   - the frame buffer.
2. Check the 4.10.05 kernels for the same, so the running system agrees with the ROM.
3. Make the board's memory decode follow the 80186 chip selects, so it no longer relies on fixed ranges:
   - **UMCS:** the ROM (upper block);
   - **MMCS/MPCS:** the COM9026 RAM;
   - **the rest:** RAM, up to 1 MB when the card is fitted.

   Keep the 512 KB behaviour byte-identical.
4. Add a 1 MB option: `Config` (or an NVRAM-preset variant), the CLI (`--ram 1024`) and the web page.
5. **Verify:**
   - Rel 2.11 self-test passes with no error 50, and a 1 MB workstation network-boots 4.10.05 to `Login:`.
   - A kernel check (`nv_config`, or `machine_test +m`) reports 1 MB.
   - 512 KB still works with both ROMs: `cargo test --release`, and `scripts/ci-smoke.txt` via `icon-cli serve` + `icon-ci script`.
6. Record findings in `ICON_firmware.md` / `ICON_hardware.md` / `gaps.md`. Mark them inferred until the U30/U31/U33 logic has been read (a dump has been requested from the board's owner).
