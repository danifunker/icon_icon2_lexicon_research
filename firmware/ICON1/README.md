# ICON1 firmware dumps

Source: York University Computer Museum (YUCoM) material, `UNISYS_ICON/Firmware/ICON1/`, dumped September 2017. Analysis: [../../ICON_firmware.md](../../ICON_firmware.md).

| File | SHA-1 | Notes |
|---|---|---|
| `icon1_rom_part1_5053-1_iss2.bin` | `f5bd3ef2fdca4571371fcca5ac1fd2bd6a9fc8cb` | EPROM 5053-1 ISS 2, even bytes |
| `icon1_rom_part2_5053-2_iss2.bin` | `f974dfe6b7a83c1d4811ade6ec534fea30d324d6` | EPROM 5053-2 ISS 2, odd bytes |
| `icon1_rom_combined.bin` | `5d7f7b173f599edf1f0e8655852c3adafa61fd89` | The two interleaved; maps to F8000–FFFFF |
| `icon1_rom_rel2.11_U33_200-504-000_low.bin` | `48a3e327edad9d73bd0cd274a051b01f0178b499` | **Rel 2.11** LOW, U33 "LOGIC 200-504-000, CHK-SUM 2373" (byte sum = 2373 ✓). Even bytes |
| `icon1_rom_rel2.11_U36_200-503-000_high.bin` | `6402e54164a19062cff97a51924ec8a816f4f782` | **Rel 2.11** HIGH, U36 "LOGIC 200-503-000, CHK-SUM 048C" (byte sum = 048C ✓). Odd bytes |
| `icon1_rom_rel2.11_combined.bin` | `3750cf6f203fe1a167b853e36d30114701b039eb` | Rel 2.11, the two interleaved (LOW even, HIGH odd); maps to F8000–FFFFF |
| `icon1_keyboard-70109-135d-scn8035a.bin` | `010053fe147e05770847c8be1b91cc1b1b4e5747` | **Bad dump.** The 8035 is ROM-less; re-dump the external EPROM |

The Rel 2.11 pair was dumped by a third party from a CEMCORP ICON main board (Microtel 350710-002) with a 1 MB upgrade card, and supplied in 2026.

These are copyrighted CEMCORP/Burroughs/Unisys firmware, kept here for preservation research.
