# Resume prompt: a standalone ("solo") ICON in icon-emu

Paste everything below the line into a new session.

---

Goal: make **icon-emu** run a **solo ICON**: one workstation with its own pedestal floppy drive and no fileserver, booting ICON system software from a solo boot diskette. Schools without a LEXICON used this setup. Say plainly what is inferred.

## Where things are

- **`~/repos/icon-emu`:** the Rust/WASM emulator. Commit straight to `main`, push only when asked, end commit messages with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Keep clippy `--all-targets` at zero warnings. Workflows use Node 24.
  - `crates/icon-core/src/machine.rs`: the board, I/O decode (`io_read8_inner` / `io_write8_inner`) and interrupt wiring in `Machine::step`. INT0 is shared by the keyboard (PA7) and the disk (PA5); INT2 by the COM9026 and the speech chip.
  - `crates/icon-core/src/devices/wd1002.rs`: the fileserver's WD1002-05 at **FAA0–FAAE** (even bytes). `Disk::floppy_from_image` knows 640K (80×2×8) and 800K (80×2×10).
  - `crates/icon-core/src/cpu/i186.rs`: the 80186 (a MAME port). It **already has the two DMA channels** (`DmaState`, `update_dma_control`, the DRQ handling around line 370). Check how a device raises DRQ.
  - Workstations are built with `disk_controller: false` (`Config`, `session.rs`, `icon-web`), so they have no disk ports at all.
  - `crates/icon-core/src/devices/nvram.rs`: `presets::workstation()`. Word 0 bit 2 = workstation, bit 3 = 1 means **no** on-board 1797 FDC, bits 12–15 disable loaders (network, floppy, hard disk, ROM).
  - Tools: `icon-cli serve` + `icon-ci` (see `docs/HEADLESS.md`); `icon-cli --preset ws|fs --script FILE`; `icon-cli disk IMAGE …` (files on an image, no machine); `ICON_WATCH`, `ICON_NET_LOG`, `ICON_DUMP_AT_ENTRY`.
  - Disk images are in `disks/` (gitignored). `lexicon-4.10.01-10mb-help.img` is an installed 4.10.01 fileserver with Help.
- **`~/repos/icon_icon2_research`:** notes; also commit straight to `main`. ROMs are in `firmware/ICON1/` (5053 ISS 2, and Rel 2.11).

## What is known

- **The hardware** (`ICON_firmware.md` §3–4, `ICON_hardware.md`): the standalone ICON's pedestal drive is driven by an **on-board WD 1797 FDC**. The ROM's floppy loader drives it through the **same FAA0–FAAE block** the LEXICON uses for its WD1002, with **80186 DMA**.
  - The ROM loader for it is the one selected by word 0 bit 13 = 0 and bit 2 = 1, at `F873:0000` in 5053 ISS 2. Rel 2.11 has its own; find it.
  - Self-test 46–4A checks the 1797: 46 busy-bit timeout, 47 failure to reset, 48 missing or defective drive, 49 interrupt did not occur, 4a interrupt will not clear. Word 0 bit 3 = 1 skips them.
  - Floppy geometry: 80 cylinders × 2 heads × 8 sectors (or 10), sectors numbered from 1; the block mapping is in `ICON_firmware.md` §7.
- **The software** (on the installed disk; read it with `icon-cli disk disks/lexicon-4.10.01-10mb-help.img cat PATH`):
  - **`/config/make_solo_disk`** ("4.01 version of stand alone operating system, November 1989"). Run it as `make_solo_disk N` with a formatted diskette in drive N.
    - It runs `dinit #1 I=/sys/FDos_ldr` (the floppy boot loader), then copies `/boot/ws_fd_boot`, about 30 `/cmds`, `/sys/gtask`, `timer`, `speech` and `sysint`, the `/util` tty tools, the editor, `/config/solo.pass` (as `pass`) and `/config/solo.sys.init` (as `sys.init`).
    - It makes `/user/solo` and `/tmp`, and only works on 640K or 800K diskettes.
  - **`/boot/ws_fd_boot`** (104,037 bytes): the kernel for a workstation with its own floppy.
  - **`/drivers/ws_floppy`**: the workstation 1797 driver. `/drivers/fd` is the fileserver's.
  - `boot_config` maps boot selectors to images, and `set_boot_type` writes word 12.
  - `ICON_disk_layout.md`: on a solo workstation its own floppy is **drive 5** (`/drivers/ws_floppy`), while the LEXICON's floppy is drive 2.
- **Making the diskette needs no new emulation.** Run `make_solo_disk 2` on the emulated fileserver with a blank diskette in its drive: `icon-ci floppy-new`, or `serve` + `run`. Save it with `floppy-save`, then check it with `icon-cli disk IMG check`.

## Suggested approach

1. **Build the solo diskette** headlessly as above, and keep it in `disks/`. Find out whether `make_solo_disk` needs 80-track 640K or accepts 800K.
2. **Read the code that drives the 1797:**
   - the ROM's 1797 loader and self-tests (5053 `F873:0000` and the test code around errors 46–4A; Rel 2.11's equivalents): register use, the command sequence (restore, seek, read sector), how DMA is set up (D0/D1 control, source FAA0?, count), and which interrupt line and PPI bit report completion;
   - `ws_fd_boot`'s `/drivers/ws_floppy` (or the copy built into the kernel): which commands it uses (read / write sector, multi-sector, format track for `fdformat`), and whether it uses DMA or polling.
   - Write it up in `ICON_firmware.md` / `ICON_hardware.md` as it becomes clear.
3. **Emulate a WD 1797** for workstations: Type I (restore, seek, step), Type II (read / write sector), Type III (read address, write track for `fdformat`), Type IV (force interrupt), plus status bits, DRQ to 80186 DMA, and INTRQ to the right interrupt line. Reuse `Disk` for the image. Keep the fileserver's WD1002 path byte-identical.
4. **A solo configuration:** a workstation with the FDC fitted, NVRAM set for it (word 0 bit 3 = 0, the floppy loader enabled, the network loader disabled or simply no fileserver, and word 12 set for `ws_fd_boot` if that's how it's chosen). Expose it through `Config`, the CLI (`--preset solo`?), headless (`icon-cli serve --solo`?) and the web page, where the workstation needs a floppy slot.
5. **Verify:** the ROM self-test passes 46–4A; the solo diskette boots to `Login:`; logging in as `solo` works (password in `solo.pass`); `files 5:/` lists the diskette; a file written to it survives a reboot and passes `chkfsys`; `fdformat` / `dinit` of a fresh diskette works. Also check that 512 KB networked workstations and the fileserver are unchanged: `cargo test --release`, and `scripts/ci-smoke.txt` via `icon-cli serve` + `icon-ci script`.
6. **Docs:** the web page guide (`docs/UI.md`), `docs/ADMIN.md` (making a solo diskette) and `docs/HEADLESS.md`. Record findings in the research notes, and in `gaps.md` (FW-7, the board straps, may also be settled).
