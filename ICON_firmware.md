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

### Real NVRAM images

Two images were read from a real 1 MB ICON's NMC9306 with a chip programmer (2025), with a bit-by-bit bench test of each word. The files store each word **low byte first**. Decoded, they confirm the full word map in re_notes/nvram_rtc_disk_install.md §1.3: boot order, memory, options, node and server, boot-file type, monitor and the serial-number packing. As delivered, the board was a factory workstation: node 254, network boot only. Later it was set up as the network fileserver, node 1. On that 1 MB machine, w0 bit 10 clear gives **Diag error 50** ("One megabyte upgrade RAM card failure"). The files are in `firmware/ICON1/NVRAM/`; the findings are in re_notes/nvram_rtc_disk_install.md §1.5 (c). icon-emu reads and writes these files (`icon-cli nvram`, the web page's NVRAM panel).

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
- **Boot disks vs this ROM (resolved, FW-11, corrected 2026-10-28):** the 4.01/4.10 fileserver boot disks set boot start = 3 and count = 10 or 9, and the loader is **laid out for this ROM's 8-sector mapping**: ROM blocks 3–8 are physical sectors 3–8 of head 0, and ROM blocks 9–12 are head 1 sectors 1–4, i.e. linear blocks 11–14 of the 10-sector image. Linear blocks 9–10 are left empty. **So the stock 5053 ISS 2 ROM boots the 800K boot disks unchanged.** This was confirmed in the emulator: it boots QNX 2.05b from the 4.01.00 ICON Fileserver Boot Diskette to `Login:` as node 1.
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

## Rel 2.11 boot ROM (U33 200-504-000 / U36 200-503-000)

Dumped in 2026 (see `firmware/ICON1/README.md`). The byte sums match the labels: LOW 2373, HIGH 048C. It reports itself as "Diagnostics Report Rev 2.11". About 49 % of its bytes match 5053 ISS 2. Its reset vector jumps to F800:0000, where 5053 jumped to F88F:0000. The 8×10 font is unchanged, at file offset 301Ah (2CF4h in 5053).

**New compared with 5053 ISS 2** (from its messages):
- **1 MB upgrade card support:** test 50 "One megabyte upgrade card RAM failure", and memory sizing that reprograms the chip selects.
- **A ROM loader:** 5e "Attempting to load from ROM".
- **Hard disk:** "Hard Disk loading sequence started", plus detailed abort codes f3–ff (read error, invalid boot disk/loader, missing boot file, network failure, fileserver not responding, incomplete boot broadcast, excessive reconfiguration).
- **Network:** test 34 "Network controller RECON interrupt present". "Loading from specified/default fileserver id": it tries the NVRAM boot server, then the default.
- Message 00 is "DON'T PANIC". The signature at the end changed from `MARTHAHANSMS` to `NHBJMBHPHDVP`.

**NVRAM word 0 bit 10** is new in Rel 2.11, and nothing else examined reads it:
- With bit 11 clear (a 1 MB board), it selects which 1 MB memory-test path runs at reset.
- In the network loader: **bit 10 set** means the COM9026 RAM segment is computed from the mid-range chip selects, `(MMCS & FE00) + 3 × ((MPCS & 7F00) >> 1)`. That gives D800 with the reset values MMCS C1F8, MPCS 90BB. **Bit 10 clear** means it takes UMCS & FFC0 (E000 at reset), which isn't where a standard ICON's network RAM is.
- So a standard ICON workstation running Rel 2.11 needs bit 10 set. icon-emu's workstation preset is now EC2E.

**Netboot:** the request (`seg`) and the boot broadcast (`b` blocks) are unchanged. The ROM still loads the image at 0070:0000. One addition: just before jumping to it, Rel 2.11 stores the boot server's node ID at **[ES:BX+48h]**, the boot parameter block, which the image header puts at 0070:0000.
- In the **4.10.01 `ws_boot`**, offset 48h is inside the start-up stub's `mov di,0 / rep movsw`. The write breaks the stub, and the workstation crashes after loading.
- The **4.10.05 patch release** (Nov 1991) changes `ws_boot` at exactly this point: at file offset 86h it inserts `EB 02 90 90` (`jmp +2; nop; nop`), leaving bytes 48h–49h free for the ROM to write.
- With the 4.10.05 `ws_boot`, a Rel 2.11 workstation network-boots 4.10 to `Login:` (confirmed in icon-emu). A Rel 2.11 fileserver boots 4.10.01 from its hard disk unchanged.

### The 1 MB upgrade card under Rel 2.11

Read from the ROM code. The memory map is **inferred**: the card's decode is in its undumped PALs (U30/U31/U33, see ICON_hardware.md), so it is reconstructed from what the ROM programs and tests. icon-emu now emulates it this way (`--ram-kb 1024`), and the result boots.

**When it runs (file `0086`):** after the main self-tests, if NVRAM word 0 bit 11 is clear (a 1 MB board), the ROM calls `1507` when bit 10 is clear, then `1523` in both cases.

**`1507` (bit 10 clear only):** copies the whole 32 KB ROM from F800:0 to RAM at 6000:0. It then returns through a `retf` to **6000:0093**, so from then on the ROM runs from the RAM copy.

**`1523`, the card test:**
1. `out CF80h, ax`, where AX is NVRAM word 0. Nothing on the main board decodes CF80, so this is probably a register on the card. What the card does with the value is unknown.
2. It re-reads word 0 and tests bit 10.
   - **Bit 10 clear:** UMCS = **FF38**, so the upper chip select covers only FF000–FFFFF (4 KB). It tests 64 KB at **EF00:0**, i.e. EF000–FEFFF. RAM therefore reaches FEFFF, and the ROM can no longer be fetched below FF000, which is why it moved itself into RAM first.
   - **Bit 10 set:** UMCS = **F83D** (the ROM at F8000–FFFFF, 32 KB), MPCS = **82BB** (mid-range block 16 KB, so each MCS is 4 KB), MMCS = **F5F8** (base F4000). MCS0–3 are then F4000, F5000, F6000 and **F7000**. It tests 64 KB at **E700:0** (E7000–F6FFF).
3. In both cases it then tests 64 KB at each of 8000, 9000, … E000 (80000–EFFFF). A failure anywhere is error **50**, "One megabyte upgrade card RAM failure".

**Where things end up with bit 10 set (the layout icon-emu emulates):**

| Range | What |
|---|---|
| 00000–7FFFF | Main-board RAM (512 KB) |
| 80000–F6FFF | Card RAM. It is tested right through F4000–F6FFF, so MCS0–2 select nothing on the bus there. |
| F7000–F7FFF | MCS3: COM9026 buffer RAM (even bytes). The network loader computes F700 from MMCS/MPCS. |
| F8000–FFFFF | UCS: the boot ROM |

The main-board RAM sizing at `09EA` treats a 1 MB board as 3 extra banks (it tests 20000–7FFFF). The frame buffer follows word 0 bits 0–1 as usual: 1 gives 5ABA0.

**Bit 10 clear:** the network loader takes `UMCS & FFC0`, which gives **FF00**. So the COM9026 RAM would sit in the 4 KB UCS window at FF000, where the ROM's reset vector also lives. That only works if the card or the CF80 write changes what UCS selects, and the ROM can't be in that window while the network RAM is. This mode is **not emulated**, and is unexplained until the PALs are read.

**The 4.10 kernels** (disassembled from RAM dumps of running 1 MB nodes: the fileserver's 4.10.01 `hd_boot` and the workstation's 4.10.05 `ws_boot`). They match the map above.
- **They don't probe memory; they trust NVRAM.** At kernel `009C:505C` (`hd_boot`; `009C:4D19` in `ws_boot`) the kernel reads word 0 through `int 5Ch` fn 2Dh and switches on `word0 & 0803h` (table at kernel DS:00E6):

  | word0 & 803 | Top of memory | Display segment (sys+62h) | `machine_test +m` flag |
  |---|---|---|---|
  | 803 (128 KB) | 1AB0 | 1ABA | 10 |
  | 800 (256 KB) | 3AB0 | 3ABA | 20 |
  | 801 (384 KB) | 5AB0 | 5ABA | 30 |
  | 802 (512 KB) | 7AB0 | 7ABA | 40 |
  | **001 (1 MB card)** | **F700** | **5ABA** | **80** |

  (The `machine_test` flags are set at `009C:5857` (`hd_boot`) or `009C:5514` (`ws_boot`) into word [067F].)
- On a 512 KB node, memory runs from the end of the kernel to 7AB00, and the display pages sit above it. On a **1 MB node** the kernel builds two free blocks (`009C:5104`): from its end up to **5AB00**, and from **5F6A0** up to **F7000**. The hole between them is the 19 KB display page at 5ABA0. So the kernel expects contiguous RAM up to F6FFF and nothing of its own at F7000 and above: exactly the bit-10-set layout. 988 KB − 19 KB = **969 KB**, which is what `net` reports (490 KB at 512 KB).
- **The network driver** (a task loaded at CS 1000 on a workstation, linear 1972B; 1CFF:972B on the fileserver) computes the COM9026 RAM segment from MMCS/MPCS with the ROM's formula, `(MMCS & FE00) + 3·((MPCS & 7F00) >> 1)`. It stores it at [0AAD] and doesn't test word 0 bit 10. So the kernels find F7000 because Rel 2.11 has reprogrammed the chip selects. They never write the chip selects or port CF80.
- With these kernels, the **bit-10-clear** layout can't work. Rel 2.11 leaves MMCS/MPCS at their reset values there, so the driver would use D8000, while the kernel hands out D8000 as free RAM. Bit-10-clear 1 MB boards probably don't exist, or ran other software (INFERRED).

**The old 5053 ISS 2 ROM can't run a 1 MB node.** It doesn't know the card: it sizes word 0 bits 0–1 = 1 as main-board banks, never writes CF80 and leaves MMCS at C000. The 4.10 kernel would then take RAM up to F7000, overlapping the network RAM at D8000 and the ROM area. In icon-emu the image loads (entry 0070:0000), but the workstation hangs on a blank console before `Login:`.
