# ICON1 firmware dumps

Source: York University Computer Museum (YUCoM) material, `UNISYS_ICON/Firmware/ICON1/`, dumped September 2017. Analysis: [../../ICON_firmware.md](../../ICON_firmware.md).

| File | SHA-1 | Notes |
|---|---|---|
| `icon1_rom_part1_5053-1_iss2.bin` | `f5bd3ef2fdca4571371fcca5ac1fd2bd6a9fc8cb` | EPROM 5053-1 ISS 2, even bytes |
| `icon1_rom_part2_5053-2_iss2.bin` | `f974dfe6b7a83c1d4811ade6ec534fea30d324d6` | EPROM 5053-2 ISS 2, odd bytes |
| `icon1_rom_combined.bin` | `5d7f7b173f599edf1f0e8655852c3adafa61fd89` | The two interleaved; maps to F8000–FFFFF |
| `icon1_keyboard-70109-135d-scn8035a.bin` | `010053fe147e05770847c8be1b91cc1b1b4e5747` | **Bad dump.** The 8035 is ROM-less; re-dump the external EPROM |

These are copyrighted CEMCORP/Burroughs/Unisys firmware, kept here for preservation research.
