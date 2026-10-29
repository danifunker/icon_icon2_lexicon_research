# ICON Series Hardware: Models, Requirements and Configurations

This document gathers every hardware fact we have in one place. Sources are tagged:

| Tag | Source |
|---|---|
| **[Disk]** | The system software itself (ISSSW 4.01–4.10.05 diskettes from the YorkU `Disk_1` set, read with `rb-cli`): `/config/sys.init`, `/config/boot_config`, `/sys/machine_test` and the help files |
| **[QDOS-RN]** | *QDOS 4.01.02 release notes* (on the QDOS diskette, May 1990) |
| **[ROM]** | Boot ROM disassembly ([ICON_firmware.md](ICON_firmware.md)) |
| **[Man]** | Unisys manuals ([manual_notes/](manual_notes/)) |
| **[Web]** | [ICON_web_research.md](ICON_web_research.md) |

Open hardware questions are tracked in [gaps.md](gaps.md) (FW-* and HW-*).

## 1. The machine family

The OS identifies machines in two ways:
- **`/sys/machine_test +t`** returns 0 = ICON, 1 = ICON2, 2 = ICONNET 286/386 (a PC with an ARCNET "Netcard"). `+c` gives the CPU type and `+b` the base machine type [Disk].
- **The boot server** chooses a boot image by the *selector* the station's ROM sends in its network boot request (NVRAM word 12; see ICON_firmware.md §6). The table is `/config/boot_config` [Disk].

| Machine | Role | Boot selector → boot file (4.10.01) | CPU | RAM | Notes |
|---|---|---|---|---|---|
| **ICON** (CEMCORP/Burroughs/Unisys, 1984–) | Workstation (networked or solo) | 1 → `ws_boot`; 2 → `ws_fsys_boot` (full file system for a local floppy or RAM disk) | Intel 80186, about 7.16 MHz [ROM][Web] | **384 KB, 512 KB or 1 MB** (the 1 MB upgrade board) [QDOS-RN] | TMS5220 speech, trackball, built-in keyboard, separate monitor. ROM 5053 ISS 2 |
| **ICON fileserver** ("LEXICON") | Network, cluster or storage fileserver | 0 → `hd_boot` (or `fd_boot` from the boot floppy) | 80186 (same board as the ICON) | 512 KB to 1 MB; the cache grows if RAM is 1 MB or more [Disk] | WD1002-05 controller, 10 or 64 MB ST-506 disk, 800K floppy, battery RTC, no speech chip |
| **ICON2** | Workstation, solo ICON2, or ICON2 with a local hard disk | 16 / 18 → `ws2_high_boot`; 20 → `ws2_fsys_boot` (also `ws2_hd_boot`, `ws2_emul_nologin`) | **unknown** (HW-1) | **1 MB** (512 KB models also existed) [Man] | Its own boot ROM (codes 40/a1/fa–fc). Built-in colour monitor, separate PC-style keyboard with trackball, YM3014 DAC (speech is emulated), 16-of-4096 colours, optional **I/O Module with SCSI**, optional **internal hard disk** (mounted as drive 8), "external card" option, ICON emulator |
| **ICON3** | Workstation | 3 → `ws3_boot` | **adjustable speed** (`set_cpu_speed`: full, 1/2, 1/3 or 1/4) [Disk] | ? | Supported from 4.10 ("QNX 4.10 SYS.INIT with ICON_LOOK and ICON3 SUPPORT"). Needs a proprietary "octopus" cable for keyboard, trackball and ARCNET [Web]. Details unknown (HW-2) |
| **ICONNET 286 / 386 workstation** (Netcard PC, e.g. **Unisys PW2 Series 300**) | Workstation | 22 → `ws_atp_boot` | 80286 or 80386 (`machine_test +c` = 2 / 3) | e.g. 1.6 MB | A PC with EGA and an ARCNET Netcard. Can use an ICON2 keyboard and trackball (`TBALL.COM`) [QDOS-RN] |
| **ICONNET 286 / 386 fileserver** (e.g. **PW2 300** with a 32 MB disk; **PW2 800** with one or two 64 MB disks) | Cluster or storage fileserver | 23 → `hd_atp_boot` (`fd_atp_boot` from floppy); 24 → `hd_atp_scsi_boot` | 286 / 386 | The 386 cache scales with RAM; with **≥ 3712 KB** the boot images are staged on a 800 KB RAM disk (drive 8) [Disk] | Runs QNX **3.06** for the PC boot images (4.10.05 `boot_atp_message`); ICON images say QNX 2.05b. Optional second floppy (drive 7). SCSI or "AT" hard disk |
| **Network Expander** | ARCNET repeater | — | — | — | 2 ICONET ports and 2 ARCNET ports, "Isolate" keyswitch, 117 V 0.25 A [Man] |

*Model-to-CPU pairing for PW2 300 / 800 is an inference from the machine_test codes and the QDOS notes; check it.*

## 2. Minimum requirements by task

| Task | Requirement | Source |
|---|---|---|
| Boot a networked ICON | ICON ROM, NVRAM holding a valid node ID and boot-server node (`attach_net`), coax link to a cluster fileserver | ROM, Man |
| Solo ICON (no network) | ICON with the pedestal floppy (on-board 1797 FDC) and a boot floppy made by `make_solo_disk`. The disk must be **80-track**. NVRAM word 0 bit 2 selects the on-board FDC | Disk (help), ROM |
| Local floppy or RAM disk on a workstation | `set_boot_type +fsys` (larger boot image). Has no effect on 1 MB workstations, which always have it | Disk (help) |
| Fileserver | ICON fileserver hardware (10 MB or 64 MB) or an ICONNET 286/386 PC. A 10 MB "old format" disk must be reinitialized for 4.x | Man |
| 4.x system software | About 5 × 800K diskettes. A 64 MB disk is split into two 32 MB partitions | Disk, Man |
| QDOS 4.01.02 | **SSW 4.01.00 or later** on the fileserver, and **2000 free blocks** | QDOS-RN |
| QDOS memory available to DOS | 384 KB ICON → up to 128 KB · 512 KB ICON → 256 KB · 1 MB ICON → 550 KB · ICON2 → 610 KB · 1.6 MB workstation → 636 KB. Add 20 KB with `-p` (no spooler) | QDOS-RN |
| QDOS `install_dos_pgm` "Other" option | A 1 MB workstation | QDOS-RN |
| QDOS colour and graphics | ICON2 (CGA emulation via XTM, trackball as mouse) or a PW2 300. The ICON is monochrome only | Man, QDOS-RN |
| Floating point | Optional **8087** coprocessor. `sys.init` mounts `float8087.slib` instead of `float.slib` when `machine_test +8` finds one | Disk |
| ICON2 hard disk | `machine_test +H1`, mounted as drive 8 with `/drivers/ws2_hd` | Disk |
| SCSI | ICON2 I/O Module, or a PC fileserver (`scsi_admin`, `scsi_config`, devices 0–7) | Disk |

## 3. Storage devices and drivers

| Drive | Device | Driver | Machines |
|---|---|---|---|
| 1 | Fileserver hard disk (partition 1) | `/drivers/hd` (ICON FS), `hd_atp` (Netcard FS), `hd_atp_scsi` | FS |
| 2 | Fileserver floppy | `/drivers/fd` (ICON FS), `fd_atp` (Netcard FS) | FS |
| 3 | Hard disk partition 2 (64 MB disks) | `hd p=2` | FS |
| 5 | Workstation floppy | `ws_floppy` (ICON), `ws2_fd` (ICON2), `fd_atp` (PC) | WS |
| 7 | Second floppy | `d=2 p=2` | 386 FS |
| 8 | RAM disk (`ramdisk`), or the ICON2 local hard disk (`ws2_hd`) | | any / ICON2 |

`machine_test` probes for these options: `+F1`/`+F2` (floppies), `+H1`/`+H2` (ST-506 hard disks), `+S1`/`+S2` (SCSI hard disks), `+m` (memory, in KB, as hex), `+k` (keyboard type), `+e` (external card on an ICON2), `+n` (booted from the network), `+s` (solo ICON2) [Disk].

Media:
- QNX floppies are 5¼" DS, 80 track, 8 or **10** sectors per track (640 / 800 KB). All release disks are 800K.
- QDOS reads DOS 5¼" HD and DD floppies, and 3½" 720K.
- ICON drives have **half-width heads**, so DOS disks they write aren't reliable in PC drives [QDOS-RN].
- Fileserver disks: **10 MB** (ST-412 class, 306 × 4 × 17, needs `park`) or **64 MB** (e.g. Micropolis 1335, two partitions). On PC fileservers: 32 MB or 64 MB (PW2 300 / 800).

## 4. Network

- **ICONET** is ARCNET at 2.5 Mbit/s over RG-62/U coax with BNC connectors, daisy-chained and terminated at both ends. The controller is an SMC **COM9026** with 2 KB of dual-port RAM at D8000 [ROM].
- **Limits:**
  - 17 nodes and 61 m per segment;
  - 16 workstations per fileserver without an expander, and a software limit of **32** booting from one fileserver;
  - **128** nodes per network;
  - node IDs: workstations 16–253, fileservers 1–245, factory defaults 254 (WS) and 245 (FS) [Man].
- **Netcard PCs** use an ARCNET card and boot through a PC BIOS. The ICON2 keyboard works on some of them.

## 5. Peripherals

| Kind | Supported | Source |
|---|---|---|
| Printers (`/config/print/printer_types`, 4.10) | Epson FX-80 · Panasonic KX-P1091 · Unisys AP1314/AP1354 · AP1327/AP1329 · AP9206 (parallel) · Unisys Model 37 (parallel and serial) | Disk |
| Printer ports | Each fileserver supports one parallel (Centronics) and one serial (RS-232) printer. Workstations need the "printer port option" | Man |
| Serial | `$slpr` RS-232-1 (ICON), `$sec` RS-422 (ICON2), `$mdm` RS-232-2 (sync/async, up to 9600). 8274-class controller on the board | Man, ROM |
| ICON monitors (`/config/monitor`) | Roland MB-121A mono, Roland CC-141 colour, KIMTRON/ICON colour, amber. CGA-type timing (CRTC values from NVRAM words 5–9) | Man, ROM |
| Display modes | ICON: 640×240×1, 320×240×2 (4 of 16 colours). ICON2 adds ×2 and ×4 bit-planes with **16 of 4096** colours. 80×24 text | Man |
| Keyboards | ICON: built-in, CUR LOCK, Action keys, RESET; on its own 8035 MCU. ICON2: separate PC-style keyboard with trackball and DIP switches, fused +5 V feed | Man, ROM |
| Pointing | Trackball (ICON quadrature counters; on the ICON2 it works as a Microsoft-mouse emulation under QDOS) | ROM, QDOS-RN |
| Sound and speech | ICON: TI TMS5220 LPC, a **TMS5220CNL (5220C revision)** per a board photo, with no speech ROM (TMS6100). ICON2: Yamaha YM3014 DAC with a software TMS5220 emulator, plus a sound administrator. Headphone jack and volume control | Man, photo |
| Telidon / NAPLPS | Software decoders (`gtask`, `naplps.sys`); `$tel` device for a Telidon modem | Man |
| Clock | Battery RTC on fileservers (`read_rtc`, `set_rtc`). Workstations take the time from their server | Man |
| NVRAM | 32-byte serial EEPROM (93x6) on every unit: node ID, boot server, boot options, video timing, serial number (`get_serial_no`, `nv_config`) | ROM, Disk |

## 6. Physical and power [Man]

| Unit | H × W × D | Weight | Power |
|---|---|---|---|
| ICON + amber monitor | 53 × 53 × 56 cm | 19.5 kg | 120 V, 3 A max |
| ICON + colour monitor | 58 × 53 × 63.5 cm | 25.5 kg | 120 V, 3 A max |
| ICON2 | 42 × 39.5 × 39.4 cm | 16 kg | 120 V, 2.5 A max |
| ICON2 keyboard | 3 × 56 × 28 cm | 3 kg | from the ICON2 (fused) |
| Fileserver, 10 / 64 MB | 21 × 38 × 54 cm | 18 / 19 kg | 120 V, 3 A max |
| Network Expander | 16 × 25 × 5 cm | 2.5 kg | 117 V, 0.25 A |

Operating environment: 13–35 °C, 10–80 % relative humidity, altitude up to 2000 m.

## 7. Software versions tied to hardware [Disk]

| Release | Kernel (boot message) | Hardware added |
|---|---|---|
| 3.00 (1987) | QNX 2.0x | ICON, ICON2, 10 / 64 MB fileservers |
| 4.00 / 4.01 (1989–90) | QNX 2.05b | Netcard (PW2 300 / 800) workstations and fileservers |
| 4.10.01 (Sep 1991) | QNX 2.05b (ICON images) | **ICON3**, ICON2 hard disk and SCSI boot images, ICON_LOOK GUI |
| 4.10.05 (Oct 1991) | ICON images 2.05b; **PC (ATP) images QNX 3.06** | Updated boot images |

## Speech chip interface (from the ROM self-test and 4.10.01, confirmed in icon-emu)

- **Port FA01:** reads return the TMS5220 status (TS, BL, BE in bits 7..5) and acknowledge the interrupt. Writes are commands, or speech data after Speak External (0x60). Reset is sixteen 0xFF bytes (0xFF decodes as the Reset command).
- **Interrupt:** on PPI A port C bit 4, **active high** to the PPI. It shares INT2 with the COM9026 (PC5) and the printer.
- **Self-test** (ROM F88F:1B94): reset, then check TS = 0 (error 4B). Speak External plus a 13-byte LPC sample, then expect the PC4 interrupt (4C). Wait for status & E0 = 60, i.e. stopped with the buffer empty (4D). Reset again and check TS = 0 (4E). The sample decodes to two voiced frames and a stop frame.
- **Software:** `/sys/speech` (the Speech Administrator, started by `sys.init` on ICON workstations) and `say FILE`.
- **Phrase files** are a series of 12-byte frames: energy, pitch, k1–k10 as parameter indices. `decode_phrase`/`encode_phrase` convert them to and from text, one frame per line. `convert_phrase` converts old TMS5220 "codepack" bitstreams.
- No phrase files exist in the YorkU disk set.
- **Clock:** an RC oscillator on the board, value unknown. 640 kHz (8 kHz output) is assumed.

## Board photo (third party, ICON workstation main board, Microtel 350710-002, © 1984 CEMCORP)

- **Boot ROMs:** U33 "LOGIC 200-504-000, CHK-SUM 2373, Rel 2.11 LOW" and U36 "200-503-000, CHK-SUM 048C, Rel 2.11 HIGH". These are the FW-2 pair, since dumped (`firmware/ICON1/icon1_rom_rel2.11_*`). The HIGH checksum is 048C; I first misread it as 04BC from the photo.
- **"1 MEG ICON UPGRADE BOARD"** daughter card (CEMCORP 1988, artwork C051-ART-002 issue 4, serial 88102731). From the later photos of the card on its own, its underside, and the bare main board:
  - **It plugs into the CPU socket.** The 80186 sits in a PLCC socket at main-board **U39**. On the card, **U34** is a PLCC socket that takes the CPU, and the pins on the card's underside plug into U39. So the card is interposed on the whole CPU bus, including the 80186 chip-select outputs, and its decode can use them.
  - **Memory:** 18 × MB81256-12 (256K×1), i.e. 512 KB plus 2 more, probably parity. Together with the main board's 512 KB this makes 1 MB.
  - **Glue logic:** 74ALS373 ×4 and 74LS373 ×2 (address latches), 74ALS245 ×2 (data buffers), 74LS157 ×2 (DRAM address multiplexers), 74S169, 74S74, 74S30, 74S244, and two "1166R-001 -330" delay lines (DRAM timing).
  - **Programmed parts** (20-pin, probably PALs), still undumped: **U30 "200-507-000 REV 1.0"**, **U31 "200-508-000 REV 1.0"**, **U33 "200-509-000 REV 1.0"**. U32 is a plain DM74S244N; an earlier note wrongly guessed it was the third labelled part.
- **Also visible:**
  - the 80186 in a PLCC socket at U39 (under the 1 MB card when it's fitted);
  - main RAM: 4 rows of 16 TMS4164 = 512 KB;
  - TMS5220CNL date code DCU8514 (1985), Philippines;
  - COM9026 (SMC 8450) with a 20 MHz crystal;
  - Hitachi HD46505SP (the 6845 video chip);
  - two AMD P8255A-5;
  - COP452-class counters (the trackball);
  - 4 rows of TMS4164 (512 KB);
  - empty "FUTURE" ROM sockets;
  - a "FILESERVER" option area on the silkscreen.
- **Not visible:** no WD1002 or 1797, so a fileserver's disk controller is a separate card.
