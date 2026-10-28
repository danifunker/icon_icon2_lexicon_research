# ICON Boot ROM ("5053 ISS 2"): First-Pass Analysis

Source: `Firmware/ICON1/` from the York University (YUCoM) material, dumped 2017. Everything here comes from disassembling the ROM with `ndisasm -b16`. **Confirmed** means it is read directly from the code. **Inference** means it is my interpretation of what the code does, and is labelled as such.

## 1. The files

| File | Size | SHA-1 | What it is |
|---|---|---|---|
| `icon1_rom_part1_5053-1_iss2.bin` | 16 KB | `f5bd3ef2…cb` | EPROM **5053-1 ISS 2**, the **even** bytes (low half of the 16-bit bus) |
| `icon1_rom_part2_5053-2_iss2.bin` | 16 KB | `f974dfe6…bc` | EPROM **5053-2 ISS 2**, the **odd** bytes |
| `icon1_rom_combined.bin` | 32 KB | `5d7f7b17…d9` | The two parts **byte-interleaved** (part1[0], part2[0], part1[1], …). This is the image the CPU sees |
| `icon1_keyboard-70109-135d-scn8035a.bin` | 2 KB | `010053fe…7a` | Keyboard microcontroller firmware (Signetics **SCN8035A**, an 8048-family MCU; part 70109-135D). Not analysed yet |

The binaries are now kept in [firmware/ICON1/](firmware/ICON1/).

**The keyboard dump is bad.** All eight 256-byte pages are byte-identical, and in every 8-byte group the bytes at positions 0, 5, 6 and 7 are always `FD`. Only about 128 bytes vary, and they don't decode as sensible MCS-48 code. The SCN8035A is the **ROM-less** member of the 8048 family, so the chip has no internal ROM to read. The real keyboard program must sit on an external EPROM next to it, probably the part labelled 70109-135D. The keyboard needs **re-dumping from that EPROM**, or from the 8035 board with the external bus monitored.

The 16-bit sums of the parts are `0618` and `21E7`. These are the early CEMCORP-era chips seen in Eckert's photo, not the later "Rel. 2.11" pair (checksums 2373 / 048C), which is still undumped. A string `MARTHAHANSMS` sits at offset `7FD4`, just before the reset vector. It is probably a programmer's signature.

## 2. Memory map (confirmed from 80186 chip-select setup at file `0AA5`)

The reset vector (file `7FF0`) is `mov dx,0FFA0h / mov ax,0E03Dh / out dx,ax / jmp F88F:0000`.

| Register | Value | Meaning |
|---|---|---|
| UMCS | `E03D` | Upper ROM window **E0000–FFFFF (128 KB)**. This is the book's "ROM up to 128 KB". These two EPROMs fill only **F8000–FFFFF**; E0000–F7FFF is the empty "FUTURE" EPROM sockets |
| LMCS | `3FF8` | Lower RAM chip select 00000–3FFFF (256 KB) |
| MMCS / MPCS | `C1F8` / `90BB` | Mid-range block at C0000 (128 KB). Peripheral chip selects are in **I/O space** |
| PACS | `0FBB` | Peripheral chip selects start at I/O **F800**, one per 128 bytes |

| Address | Device |
|---|---|
| 00000–7FFFF | DRAM, 4 × 128 KB banks (self-test codes 10, 14–16). NVRAM word 0 bits 0–1 say how many banks to test and clear |
| **D8000–D8FFF** | **COM9026 ARCNET dual-port RAM** (2 KB, on even addresses only). Page 0 at D8000 is transmit; page 1 at D8400 (segment D840) is receive |
| F8000–FFFFF | This ROM. The main code runs with CS=F88F (file offset = IP + 08F0) |
| FFFC0 | **Loader vector table** (§4) |

## 3. I/O map (confirmed unless marked)

| Port(s) | Device | Evidence |
|---|---|---|
| F801 / F803 | **HD46505 / 6845 CRTC** (address / data) | Registers R0–R9 are loaded from NVRAM words 5–9; error 0e |
| F880 / F882 | **SMC COM9026 ARCNET** (status–interrupt mask / command) | After reset it expects `D1h` at RAM[0] (error 31) and status bits `91h` |
| F900–F906 | Four 4-bit registers loaded from NVRAM word 11, one nibble each. **Inference:** colour or palette map |
| F920 | Cleared at reset; used in the keyboard test (codes 17–18) |
| F981 / F983 / F985 / F987 | **8255 PPI "A"** (ports A, B, C, control); mode `98h` |
| FAC1 / FAC3 / FAC5 / FAC7 | **8255 PPI "B"** |
| FA88 | **Beeper / trouble LED.** Pulsed once between test phases. On failure it shifts the error code out bit by bit with long and short timings. This matches Admin Guide Vol. 3: *8 tones per code, high nibble first, long low tone = 0, short high tone = 1*, with the fileserver's yellow LED flashing the same pattern and a single tone meaning "passed" |
| FAA0–FAAE | **Disk task file.** On a LEXICON this is a WD1002-05 (data, error, count, sector, cyl-lo, cyl-hi, SDH, status/command). In the standalone ICON's floppy loader the same block drives a 1797 FDC directly, with 80186 DMA |
| FA01 | **TMS5220 speech synthesizer** data/status. Used only in the speech tests (codes 4c, 4d) and the disk-interrupt test (4a) |
| FAE1 / FAE3 / FAE5 / FAE7 | **Dual serial controller** (8274-class): data A, data B, control A, control B. Used only in the serial tests (codes 38–3f) |

Ports were labelled by which self-test error codes reference them (each port was matched to the `mov bl,<code>` that precedes it in the disassembly). The baud-rate and trackball counter tests (2b–30) don't touch a dedicated port block, so those counters are probably read through PPI B and the 80186 timers.

PPI A bit assignments seen so far:
- Port A bits 0–4 hold board straps. Values `00`, `1D`, `1E` or `1F` make the ROM also test PPI B. **Inference:** these are LEXICON / standalone-type boards.
- Port A bit 7 is keyboard data or interrupt.
- Port B bit 1 is the ARCNET reset. Port B bit 7 is the EEPROM chip select.
- Port C bit 0 is EEPROM data in and bit 1 is EEPROM clock. Port C bit 5 is the ARCNET interrupt, bit 6 is EEPROM data out, and bit 7 is a sync input (error 13).

## 4. Boot sequence (confirmed)

1. CPU and flag self-test, chip selects, PPIs, CRTC, NVRAM, DRAM banks, keyboard, interrupt controller, timers, ARCNET, serial and disk controllers. Failures print `NN : message` from the table below, under the banner `>>> Diagnostics Report Rev. 1.1 <<<`. The screen font is 8×10 and is at file offset `2CF4`.
2. The ROM reads **NVRAM word 0** (the boot configuration word) and calls the loaders through the far-pointer table at **FFFC0**, in this order. A clear bit enables a loader:

| Order | Enable bit (word 0) | Table slot → entry | Loader | Failure code |
|---|---|---|---|---|
| 1 | bit 13 = 0, bit 2 = 0 | +04 → `F817:0000` | **Floppy via WD1002-05** (drive-select 3 = its WD2797) | 56 |
| 1 | bit 13 = 0, bit 2 = 1 | +0C → `F873:0000` | **Floppy via on-board 1797 FDC** (standalone ICON pedestal drive) | 56 |
| 2 | bit 12 = 0 | +08 → `F82D:0000` | **Network (ARCNET) loader** | 57 |
| 3 | bit 14 = 0 | +00 → `F800:0000` | **Hard disk via WD1002-05** | 58 |
| 4 | bit 15 = 0 | +10 → `0000:0000` (empty) | **ROM loader.** The slot is empty in this ROM. It was presumably meant for an OS in the "FUTURE" sockets | 59 |

If the top nibble of word 0 is `F` (all loaders disabled), the ROM reports `55 : Diagnostics report complete, no loader specified`. If every enabled loader returns, it reports `5a : Sorry, no specified loader option has succeeded`. A loader that succeeds never returns.

The ROM itself boots in the order **floppy → network → hard disk**. The 4.x installation guides describe the same order from the user's side, with the floppy taking priority when its drive lever is closed (see the manual notes).

## 5. NVRAM: the 32-byte EEPROM (answers open question 1)

- **Part:** a Microwire serial EEPROM, bit-banged on PPI A. It uses the 93x6 command set (start bit, opcode `10` read / `11` erase / `01` write / `0011` write-enable) and holds **16 words × 16 bits = 32 bytes**, which matches the book's "32-byte EEPROM". It is an NMC9306 or equivalent.
- **Errors:** 11 means missing (every word reads `FFFF`), 12 means the read/write check failed, and 13 means "misprogrammed (no sync)".

| Word | Use (confirmed reader) | Meaning |
|---|---|---|
| 0 | Boot configuration | bits 0–1 = number of extra 128 KB DRAM banks · bit 2 = floppy loader type · bit 4 = ? (tested before the keyboard/PPI setup) · bit 7 = keyboard option · **bits 12–15 = loader disables** (§4) |
| 5–9 | Video timing | Ten bytes copied into CRTC registers R0–R9 |
| **10** | **Network identity** | **High byte = this station's ARCNET node ID.** It goes out on PPI B port A while the COM9026 is held in reset, so the chip latches it as its ID. **Low byte = the boot server's node.** The network loader sends its request there and accepts replies only from it |
| 11 | Four nibbles | Written to F900–F906 (palette?) |
| 12 | Network loader | Low nibble = **boot image selector**, sent in the request and checked in the replies |
| 1–4, 13–15 | Not read by the boot path seen so far | |

**The node ID therefore lives in the EEPROM, not in the EPROMs.** This settles the [JE2]-versus-book disagreement in favour of the book. It also explains the installation guides' behaviour: new stations come up as node 254, and `attach_net n=… c=…` sets the node and boot server permanently, because it writes NVRAM word 10.

## 6. The network boot protocol (confirmed structure; field meanings partly inferred)

The loader is compiled C. It runs as follows.

1. It reads NVRAM words 10 and 12, resets the COM9026 and waits for the reset flags. It enables receive into page 1, and sends a request from page 0 to the boot server node (command `03`).
2. **The request** is a short ARCNET packet, 10 data bytes long (count byte `F6`):

   ```
   FF FF 00 03  <attempt#>  00  's' 'e' 'g'  <image selector>
   ```

   The attempt number starts at 5 and goes up by one per retry.
3. **The replies** are accepted only when the source ID equals the server node. Each reply's data has these fields:

   ```
   +0  FF FF          (checked)
   +2  5 bytes        (skipped)
   +7  'b'            (0x62, block message)
   +8  1 byte         (skipped)
   +9  image selector (must match)
   +10 u16 block#     u16 last-block flag   u16 load segment   u16 load offset   u16 byte count
   +20 u16 arg0       u16 arg1              u16 entry IP       u16 entry CS
   +28 2 bytes        (skipped)
   +30 data …         (byte count bytes, copied to load segment:offset)
   ```

4. The loader marks each block number in a bitmap. When the last-block flag has arrived and every bit up to that block is set, it pushes arg0 and arg1 and **far-jumps to entry CS:IP**.
5. **Timeout and retries:** 80186 timer 1 counts to `125C` per try, and there are 3 tries. After that the loader returns and the ROM reports code 57.

The 4-byte gap after `'seg'` and the server's reply format look like a QNX 2 "segment load" message. **Inference:** the LEXICON's drive-8 RAM disk image is served block by block this way.

### 6b. Server side (from `/sys/bootrec` and `/sys/bootsnd` on the 4.x disks)

- `bootrec` (2 KB) runs on every cluster fileserver. It listens for `'seg'` requests and starts `bootsnd`; with `+logging` it logs to `1:/tmp/bootlog`.
- `bootsnd` (4 KB) looks up the request's **type** (the image selector) in `1:/config/boot_config`, then reports `Broadcasting %s (type %u)` and `sending download packet (%u) for type %u`. **The image is broadcast**, so every station waiting for the same type takes the blocks from one stream. That is why the ROM keeps a bitmap of received blocks and accepts them in any order.
- `boot_config` maps selector → file. In 4.10.01: 0 `hd_boot`/`fd_boot` (fileserver), 1 `ws_boot`, 2 `ws_fsys_boot`, 3 `ws3_boot`, 16/18 `ws2_high_boot`, 20 `ws2_fsys_boot`, 22 `ws_atp_boot`, 23 `hd_atp_boot`/`fd_atp_boot`, 24 `hd_atp_scsi_boot`. See ICON_hardware.md §1.
- After boot, everything is **QNX 2 native networking**: message passing between nodes over ARCNET (remote disks via `remdisk`, `[node]` paths, the `poll`/`alive` node poller, the `def_server` name server). It is proprietary and undocumented. An emulator doesn't need to understand it if the COM9026 is emulated at register level, because the guest OS implements the protocol itself. **No TCP/IP, RPL or standard ARCNET boot protocol is involved.**

## 7. Disk loaders: geometry for the filesystem reader (confirmed)

All three disk loaders work the same way. They read **one 512-byte boot sector** to `3000:0000`, and take the **start block** (1-based, word at +2) and **sector count** (word at +4) from it. They then read that many **contiguous** sectors to `3000:0000` onward and `call far 3000:0000`.

| Loader | First sector read | Geometry hard-wired in ROM | Max start block | Block → CHS |
|---|---|---|---|---|
| Hard disk (WD1002, SDH `A0`) | C0 H0 S0 | **17 sectors × 4 heads = 68 per cylinder**, sectors numbered from 0 | `5148h` = **20808** (306 cyl × 4 × 17 = ST-412-class 10 MB) | cyl = (b−1)/68 · head = ((b−1) mod 68)/17 · sec = (b−1) mod 17 |
| Floppy (WD1002 drive 3, SDH `38`) or on-board 1797 | C0 H0 **S1** | **8 sectors × 2 heads**, sectors numbered from 1, 80 cylinders | `500h` = **1280** blocks = 640 KB | cyl = (b−1)/16 · head = ((b−1) mod 16)/8 · sec = (b−1) mod 8 + 1 |

What this means for a filesystem reader:
- **QNX block *n* (1-based) is at byte offset (n−1) × 512** in a raw floppy image stored in cylinder, head, sector order. Block 1 is the boot sector.
- The ROM can boot only the 8-sector format. The Admin Guides also document a **10-sector/track 800 KB** format, so a reader should detect the format from image size (655360 vs 819200 bytes).
- **Boot disks vs this ROM (resolved, FW-11):** the 4.01 and 4.10 fileserver boot disks have boot start = 3 and count = 10 or 9, so they load **blocks 3–12 or 3–11**. On a 10-sector disk this ROM's 8-sector mapping reads block 9 onward from the wrong sectors. **This ROM cannot boot the 800K fileserver boot disks.** The fileserver needs the later Rel. 2.11 ROM (or an emulator with a patched or high-level loader).
- **Real-world note (2026-10-28):** all 34 YorkU QNX disks are **800K / 10 spt**, including the fileserver boot disks. This ROM's loaders assume 8 spt, so it could read only blocks 1–8 of them correctly. See gaps.md FW-11.
- The **ICON floppy format** is 80 tracks, DS, **8 × 512-byte sectors**, 640 KB. That gives **1280 blocks**, and the book's "1276 free blocks" after `dinit` fits that.
- **20808 blocks** is exactly the 4.x installation guides' "old format" 10 MB fileserver disk, so that number is confirmed. Larger disks (the 64 MB Micropolis) can't be booted by this ROM's hard-disk loader. That would explain the later Rel. 2.11 ROMs.

## 8. Self-test and error code table (verbatim from ROM, file `2013`)

```
09 Parallel interface A failure in port B      0a  … A port C
0b Parallel interface B failure in port A      0c  … B port B     0d … B port C
0e Crt controller r/w register test failure    0f  No sync transistions detected (GA13)
10 Dynamic RAM 128k bank 0 failure             14/15/16  … bank 1/2/3 failure
11 Missing or failed NVRAM                     12  NVRAM r/w check failure
13 Failed or missprogrammed NVRAM (no sync)
17 Keyboard - no interrupt detected            18  Keyboard interrupt will not clear
19 Keyboard self test failure (invalid keycode)
1a Keyboard interrupt present on PPI A (pa7)   1b  Disk interrupt present on PPI A (pa5)
1c Network interrupt present on PPI A (pc5)    1d  Speech synthesizer interrupt present on PPI A (pc4)
1e Line printer interrupt present on PPI B (pc0)
20 Internal interrupt controller r/w register failure
21 Cpu INT0 line will not clear (possible keyboard error)   22/23/24  INT1/2/3 will not clear
25/26/27 Internal timer 0/1/2 counter error    28/29/2a  timer 0/1/2 interrupt present
2b/2c Baud rate counter A/B failure            2d/2e Trackball X counter A/B   2f/30 Trackball Y counter A/B
31 Network controller did not reset properly   32  Network dual port RAM failure
33 Network contoller interrupt malfunction
38/39 Serial port A reset malfunction / malfunction        3a/3b  Serial port B …
3c/3d Serial loopback A->B Tx/Rx error / interrupt failure  3e/3f  B->A …
40 WD1002 controller busy bit timeout          41–45  WD1002 error in WD2797 / WD1010 / sector buffer / WD1014 or bus / WD1015
46 FDC1797 busy bit timeout   47 failure to reset   48 missing or defective drive   49 interrupt did not occur   4a interrupt will not clear
4b Speech synthesizer failed to reset properly   4c interrupt did not occur   4d/4e speak external start/stop malfunction
55 Diagnostics report complete, no loader specified
56 Floppy disk loader has returned with an error      57  Network loader has returned with an error
58 Hard disk loader has returned with an error        59  ROM loader has returned with an error, cannot boot
5a Sorry, no specified loader option has succeeded
```

The test list confirms the hardware: two 8255s, a 6845 CRTC, a COM9026, a TMS5220 speech chip, a trackball (X/Y counters), two serial ports, a parallel printer port, a WD1002-05 (with WD2797, WD1010, WD1014 and WD1015) and a 1797 FDC.

## 9. Next steps

- Dump the **Rel. 2.11** pair (U33 LOW chk 2373, U36 HIGH chk 048C) and diff the loader table and hard-disk geometry against this one.
- Write an **NVRAM image** spec (a 16-word table) so an emulator such as MAME can boot this ROM into the network loader.
- Re-dump the keyboard's **external** program EPROM (see §1), then disassemble it for the scancode table.
- Work out the server side of `'seg'` / `'b'` by matching it against the netboot code on a 4.x fileserver boot diskette (whoever is extracting Disk_1 should look for it).
