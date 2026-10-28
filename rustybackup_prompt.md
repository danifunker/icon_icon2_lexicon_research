Please give me a status report on RustyBackup's QNX 2 filesystem support, for Unisys/CEMCORP ICON and LEXICON disk images (QNX 2.x, 1985–1991). Don't build anything new. Just tell me what exists today.

For each item, answer **Yes**, **Partial** or **No**, with one line on how it's done (file or function name):

1. Detects QNX 2 images and picks the geometry from the image size (655,360 bytes = 80×2×8×512, the standard 640K; 819,200 bytes = 800K).
2. Reads the root directory entry from block 1 (byte offset 68), using 512-byte, 1-based blocks.
3. Follows extent chains (a 16-byte header of prev/next/size/bound at the start of each extent) and walks subdirectories.
4. Extracts files at the correct size. Does it treat `size_xtnt` as blocks or as bytes?
5. Parses the 48-byte directory entry: name (max 16 chars), owner as group.member, attribute/permission bits, and dates (year−1980 in 4 bits plus a PM bit).
6. Lists or recovers deleted entries (status byte bit 0x40 clear).
7. Optionally converts text line endings from RS (0x1E) to LF.
8. Uses the `/bitmap` (block 2) only as a cross-check, and reports orphaned blocks.
9. Detects non-QNX disks and hands them off: FAT12 MS-DOS disks, QDOS `.pf`/`.ph` disk-image files stored inside QNX, and `arch` archive floppies.
10. Handles images with bad sectors without aborting.
11. Supports hard-disk images: 10 MB single volume, or 64 MB with a partition record, a bad-block table and two 32 MB partitions.

Then briefly:
- Which ICON images has it been tested on, and what were the results?
- Did the real images contradict any of the assumptions above?
- What's next on its to-do list?
