# ICON1 NVRAM images

Read in 2025 from the NMC9306 serial EEPROM (16 words × 16 bits) of a real ICON with the CEMCORP 1 MB card, using an EPROM programmer. Each file is 32 bytes, with every word stored **low byte first**: the bytes `7D 06` are word 0 = `067D`. Decode or edit them with `icon-cli nvram FILE` (icon-emu), or load them in the web page's NVRAM panel. The word map is in `re_notes/nvram_rtc_disk_install.md` §1.3. These images and the bench findings are in §1.5 (c).

| File | SHA-1 | Notes |
|---|---|---|
| `ICON_NVRAM_9306-org.bin` | `854deed0f1a73849bf9f169ead6ec130ca5f66e2` | As delivered: a factory workstation. Node 254, server 1, network boot only, ws_boot. The hardware serial equals the software serial, 300-850803-188. Words `E67D 0358 0CBC 0358 0CBC 2838 442D 0041 3C3C 0300 FE01 F0F0 0001 EBFA FFFF FFFF` |
| `ICON_NVRAM_9306_fileserver_nofddboot.bin` | `3ab1f62b3cbed3ba38cf03588ac3ca97ecb693a1` | The same board as the network fileserver: node 1, server 1, 1 MB, Roland CC-141, hardware serial 707-870712-867. Word 0 bit 2 is set, so the floppy is skipped and it boots network → hard disk. Words `067D 7777 3363 0358 0CBC 2838 442D 0441 3C3C 0300 0101 F0F0 0101 EBFA FFFF FFFF`. Boots icon-emu to `Login:` (Rel 2.11 ROM, 1 MB) |
| `NVRAM_bits.txt` | `ab1fa32d79fd1690ebe354370b9830b97b6e92fb` | Bench log: each bit flipped on the real machine and what happened, plus Jeremy's notes on the chip. Its register values are written as the bytes appear in the file (low byte first): "7D06" is word 0 = 067D, and its "0x20" bit is word 0 bit 13 |
| `nv_config` | `3159d4bc5290bcadcf1f2843147058a996cf49da` | `/cmds/nv_config` from the machine, a QNX 2 load file. It prints the NVRAM settings through `int 5Ch` |
