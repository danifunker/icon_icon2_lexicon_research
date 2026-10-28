# ICON / LEXICON: Findings from Web Sources

A summary in my own words of 11 online sources, archived locally in [web_archive/](web_archive/README.md). Each fact is tagged with its source:

| Tag | Source |
|---|---|
| **[JE1]** / **[JE2]** | Jason Eckert, ICON and LEXICON posts (2022) |
| **[VC]** | vintagecomputer.ca |
| **[AA]** | Anthony Anjo's Burroughs ICON site (2000) |
| **[JF]** | Jacob Filipp (2025) |
| **[OC]** | old-computers.com, including notes from John Bridgman |
| **[PCM]** | pcmuseum.ca comments |
| **[VCF]** | VCFed forum thread |
| **[DY]** | Dean Yergens |
| **[DP]** | Doug Peterson |
| **[ERIC]** | *Closing the Circle* (Ontario MOE report, 1991) |
| **[Book]** | *The Book of ICON* (local) |

Photos referenced are in [web_archive/_hardware_photos/](web_archive/_hardware_photos/).

---

## 1. The network boot question: answered

The book was silent on this. The web sources fill in the gap:

- **Networked ICONs have no local storage.** At every power-on they download QNX from the LEXICON over ARCNET into RAM, then log in to the LEXICON for files and programs [JE1][JE2][JF].
- **Without a LEXICON, an ICON just searches for the server and then sits silent** [VC]. In other words, the ROM contains only the boot and network loader, not a usable standalone OS.
- **The LEXICON keeps the OS image to send to stations on a RAM disk, drive `8`** [JE1]. Drives are `1` = hard disk, `2` = floppy, `8` = RAM disk.
- **Node numbers:** the LEXICON is node **1**, ICONs are nodes **16–47** (up to 32 per LEXICON). A second LEXICON would be node 2, with its ICONs at 48–79 [JE2]. The login banner shows the station's node, e.g. "node 18" (`eckert_QNX_login_node18.png`).
- **Where the node number lives:** Burroughs set it at order time and stored it **in the system EPROMs** [JE2]. The book's spec lists a 32-byte EEPROM [Book p158]. Both could be true: the EPROMs may hold defaults and the EEPROM a per-unit ID. This is **unresolved**.
- **The LEXICON is the same board with a monitor and keyboard attached.** Its video and keyboard ports are used only to watch or interact **during boot, for troubleshooting**. After boot its video turns off and you administer it from an ICON [JE2].
- **Booting a standalone ICON from floppy:** Anjo says the standalone (floppy) ICON could run MS-DOS 3.2 [AA]. VC guesses it needs the WD controller board and different jumpers [VC]. *(Correction, 2026-10-28: DOS ran under QDOS on networked ICONs. The ROM does have a standalone floppy loader using an on-board 1797 FDC, but it boots QNX. See §7 and [ICON_firmware.md](ICON_firmware.md).)*
- **Rebooting:** RESET + CTRL + SHIFT together reboots an ICON [AA].

### Boot firmware seen in photos

| Board | EPROM labels | Chip | Source photo |
|---|---|---|---|
| ICON (Eckert, CEMCORP-era board, "COPYRIGHT 1983 CEMCORP", Microtel) | **5053-1 ISS 2** and **5053-2 ISS 2** | AM27128ADC (16 KB each → **32 KB**) | `eckert_ICON_motherboard.jpg` (item 5) |
| LEXICON (Eckert) | **LOGIC 200-50x-000, U36 HIGH / U33 LOW, Rel. 2.11** | — | `eckert_LEXICON_motherboard.jpg` (item 4) |
| ICON with 1 MB board (vintagecomputer.ca) | **LOGIC 200-504-000, U33, CHKSUM 2373, Rel. 2.11, LOW**<br>**LOGIC 200-503-000, U36, CHKSUM 048C, Rel. 2.11, HIGH** | 28-pin EPROMs | `vintagecomputer_ICON-formware.jpg` |
| "1 MEG ICON UPGRADE BOARD" (on the LEXICON) | small parts labelled **200-507-000, 200-508-000, 200-509-000, Rev 1.0** (probably PALs/PROMs for memory decoding, not code) | — | `eckert_LEXICON_motherboard.jpg` (item 8) |

What this tells us:

- **Firmware is split into HIGH and LOW halves** (U36 and U33). This matches the 80186's 16-bit bus, with even and odd bytes in separate chips.
- **At least two firmware generations exist:** the early "5053-x ISS 2" and the later "Rel. 2.11". The later one appears on both a LEXICON and an upgraded ICON, so **the same ROM probably boots both roles**. Jumpers or the attached disk controller would then decide whether a board acts as a server or a station [VC].
- **Sizes disagree.** Anjo says two 32 KB ROMs (64 KB) [AA]. The early board has 2 × 27128 (32 KB). The book says "up to 128 KB" [Book]. The board has unpopulated sockets marked "FUTURE" next to the EPROMs (A28–A31), which leaves room for that.
- **The checksums give you something to verify a dump against** (2373 / 048C for Rel. 2.11).

## 2. Hardware details

### ICON workstation board [JE1][AA][VC]

| Part | Chip |
|---|---|
| CPU | Intel R80186, about 7.16 MHz, under a metal retaining plate |
| RAM | 512 KB of TMS4164 (64K×1). Anjo: 48 soldered chips plus a socketed bank. VC repaired a unit with one bad 4164 |
| I/O | 2 × AMD P8255A-5 PPI |
| Network | **SMC COM9026** ARCNET controller, 2.5 Mbit/s |
| Video | Hitachi HD46505SP CRTC. Monitor is CGA-compatible (VC uses a Magnavox RGB 80) |
| Speech | TI **TMS5220CNL** |
| Firmware | 2 EPROMs (see above) |
| Unused on a network ICON | Parallel, serial and disk controller headers on the board. Rear blanking plates cover RS-232 and parallel. Side plate hides an expansion edge connector [AA] |
| Rear panel | 2 × BNC (ICONET in/out), video out (two connectors, only one works), external keyboard (disables the built-in one), 2 × AC outlets (one switched), volume control and ¼" headphone jack at the front [AA][JE1] |
| Model numbers (Burroughs) | Microtel **BB-5001ADAE ISS:3**; Burroughs **CA8215-TPO**; "Made in Canada" [AA] |
| Physical | 65 lb with monitor; 23" high [AA] |

### LEXICON server [JE2][AA][VC][OC]

- **Same motherboard** as the ICON, with these differences: no speech chip; a cheaper **Fairchild F6845AP** CRTC; an **Intel D8274** serial controller for modems; a lithium battery for the real-time clock; and the parallel, serial and disk headers wired out to ribbon cables.
- **Memory:** the **1 MEG ICON UPGRADE BOARD** (© 1988) carries the CPU and 512 KB of Fujitsu MB81256 and plugs in around the CPU socket. QNX alone uses about 417 KB on the LEXICON [JE2].
- **Disks:** **Western Digital WD1002-05** MFM controller [VC]. The original drive was 10 MB [Book][DP][OC]. Later drives were 64 MB formatted / 70 MB unformatted; Eckert's unit has a **Micropolis 1335** (71 MB) [JE2][OC]. The floppy is a 5.25" DS/DD Panasonic. *(old-computers.com's claim of "8-inch floppies" looks wrong; everyone else says 5.25".)*
- **Ports:** 2 × BNC, parallel, serial 1 (serial printer), serial 2 (HDLC, SDLC, X.25 or Telidon modem, up to 9600 baud), and video plus keyboard for boot-time troubleshooting [JE2].
- **Stations per server:** 32 according to [JE2]; "sixteen nodes" according to [AA]; 12–16 per the original Ministry spec [ERIC]. The protocol limit is probably 32, with 16 typical in classrooms.

### ICONET [JE2][Book]

- ARCNET at 2.5 Mbit/s over **RG-62/U coax** with BNC connectors, in a bus or daisy-chain layout, terminated at both ends.
- ARCNET calls a network a "cluster", and the login banner shows the cluster number.

## 3. QNX on the ICON: additions to [ICON_commands.md](ICON_commands.md)

- **Path syntax confirmed:** `[node]drive:/path`, e.g. `[1]1:/config/print`. Both the node and the drive default to 1, which is why the book's `[1] fdformat 2` targets the LEXICON [JE1][JE2].
- **Filenames:** up to 16 characters, case-sensitive.
- **Epoch:** 1 Jan **1980**.
- **Variables:** the shell (`/cmds/sh`) uses `#` for variables: `#1`, `#2` for positional arguments and `#$` for the process ID [JE1].
- **More directories:** `/drivers` (e.g. `hd`, `fd`). `/cmds` and `/util` are on the default search path [JE1].
- **Commands not in the book:**

| Command | What it does |
|---|---|
| `help <cmd>` | Help pages, similar to man pages |
| `<cmd> ?` | Short usage summary |
| `task` | Process list, showing "father", "son" and "brother" relationships |
| `apb` | "All points bulletin": broadcast a message to every ICON on the LEXICON |
| `say` | Speak text through the TMS5220 |
| `new_password` | Change a user's password |
| `pwd` | Print working directory, including node and drive, e.g. `[1]1:/…` |
| `query [1]1` | Free space on the LEXICON hard disk |
| `2:/install` | Standard installer on software floppies; `print 2:/READ_ME` first |

- **Default accounts** in `/config/pass`, which stores passwords in plain text [JE1]: `root`, `supervisor` (home `/util`), `teacher`, and `icon` (no password). The default passwords are listed in [JE1].
- **A known security hole:** a former student describes getting root through a flaw in QNX's `exec()`, which let a program name a parent process to inherit privileges from [PCM]. *(This is an anecdote and is unverified.)*

## 4. Versions and timeline

| Year | Event | Source |
|---|---|---|
| 1982 | Ministry call for an educational microcomputer; GEMS / OAEM specification | [ERIC][JF] |
| Nov 1983 | CEMCORP prototypes delivered; field tests in 22 schools | [ERIC][JF] |
| 1984–85 | Burroughs joins CEMCORP; Microtel builds the hardware; Quantum writes QNX. 10,000 ICONs installed by the end of 1985 | [ERIC][AA][VC] |
| 1985 | Book of ICON written against QNX 1.13. The command to format floppies has already changed twice | [Book] |
| 1986 | Burroughs and Sperry merge to form **Unisys**. IBM EDNET also approved for GEMS funding | [ERIC] |
| 1987 | Production moved to Asia (Lucky Goldstar, Korea, per JF); 17,000 GEMS workstations | [JF][ERIC] |
| 1988 | **ICON System 3.00.04**, based on **QNX 2.05b**; 1 MB upgrade board | [JF][JE2] |
| 1989–90 | Other GEMS platforms approved (Commodore, Apple, Tandy, Atari, Acorn). **EASI** portability layer contracted (1990) | [ERIC] |
| — | **ICON II** (lighter: just a monitor and a keyboard/trackball unit), **ICON III** (desktop, networked or standalone), then 486 and Pentium Windows PCs under the ICON name. Unisys's QNX fork is called **"Iconix"** | [AA] |
| 1993 | **ICON System 5.00.02**, based on **QNX 3.15g**, running on a Unisys PW2 Advantage 4336DX server | [JF] |
| 1994 | Government support ends; hardware ordered scrapped | [JE1] |
| 1995 | Iconix development stops (version 5) | [AA] |

Other known system releases: **2.25 Rev 3** (in the VCFed lot), **3.00.00** and **4.00.00** (imaged by a collector) [VCF][DY]. A DOS emulator called **QDOS** let later systems run MS-DOS programs such as MS Works [DY][DP]. The GUI layers were **Ambience**, **ICONLook**, and the **House** file manager [OC][Book].

Design history: per John Bridgman [OC], the ICON was first conceived as a teacher-authoring and hypertext system built on **NAPLPS/Telidon** graphics, but that part was cancelled before launch. The Ministry had asked for a 5 MB disk; the designers insisted on 10 MB.

## 5. Preservation leads

- **Disk images:** collectors have imaged System 3.00.00 and 4.00.00 disks with a Kryoflux, and pcmuseum.ca also holds software [DY]. A VCFed lot (2019, Toronto) included System 2.25 Rev 3 and a long software list, but there's no evidence it was ever imaged [VCF].
- **Videos:** Eckert has posted videos of a network boot and of QNX in use on YouTube: `pn0SIteAe1k` and `kr8ts6zV2DU` [JE1].
- **Manuals known to exist:** *ICON User's Guide*, *QNX for the ICON* (CEMCORP), *Burroughs ICON Computer Dictionary*, and the Unisys *Administration Programs* box [AA]. The **CEMCORP ICON Hardware Manual** was being sought as of 2000 and would explain the boot hardware [AA].
- **Archives:** the Archives of Ontario holds material and floppies; search for Ambience, Cemcorp, Unisys, Burroughs or Meridian [JF].
- **Emulation:** there is no known ICON emulator; several commenters want one [VCF][DY]. The ROM checksums in §1 and the chip list in §2 are the starting points.

## 6. Second pass: further-reading sources (added 2026-10-14)

More tags: **[GB]** Giant Bomb thread · **[JD]** Julian Dunn (2006) · **[WP]** Wikipedia · **[PGC]** Post Game Content · **[PR]** Pop Rewind · **[OCS]** old-computers.com user stories · **[W88]** Willing, *Journal of Reading* (1988). All are archived in [web_archive/](web_archive/README.md). The full list of software titles is in [ICON_software_catalog.md](ICON_software_catalog.md).

### Emulation and preservation (the most important new lead)

- **ROMs already dumped and disassembled:** a Toronto collector has a working LEXICON plus an ICON 1 and ICON 2 [GB]. A commenter writing as "edgecase" says they **dumped the ROMs, disassembled the firmware and built a memory map**.
- **MAME is mostly there:** per the same commenter, MAME already emulates most of the chips. Only the **trackball interface** and the **ARCNET chip (COM9026)** are missing.
- **Software recovered from the Archives of Ontario:** the collector has been imaging floppies there and has recovered **QNX 2, Turing, Pascal and C**, but no games yet [GB].
- **More material:** a Toronto university museum reportedly holds donated ICON material [GB].
- **QNX has nothing left:** a BlackBerry employee asked around inside QNX and found no source, specs or hardware [GB].
- **Related project:** virtuallyfun.com has a write-up on reverse-engineering QNX 1.2 to boot from a hard disk on PCs (Feb 2021). It is relevant because ICON QNX is from the same lineage [GB].

**Status of the ROM dumps (checked 2026-10-14):**
- **Not public.** The claim comes from a single Giant Bomb comment by "edgecase", from around late 2019 (a reply in the thread is dated Dec 10, 2019).
- **Not in MAME.** MAME's current source tree has **no ICON driver**; the only Burroughs and Unisys entries are unrelated terminals. So the dumps and memory map were never merged into MAME, and as far as we can find they've never been posted anywhere.

**Who to contact:**
- **u/archeobits on Reddit** (r/retrobattlestations, 2018). They own a working LEXICON (System 3.00.04), an ICON 1, an ICON 2, an ICON 3 and a PW2 Advantage server running System 5.00.02. They say they've imaged every ICON floppy and server hard drive they could get, they're asking for more material, and they know of only four ICON owners worldwide. Their photos are the ones Jacob Filipp used.
- **"edgecase" on Giant Bomb** (the ROM dumper), who may be archeobits or working with them.
- **The collector who left Kryoflux comments on Dean Yergens' blog** (System 3.00.00 and 4.00.00 images) is probably the same person. All three describe a Toronto collector with a Kryoflux.

**Next step:** contact archeobits / edgecase. Their ROM dumps, memory map and disk images would answer the open questions below and save months of work.

**Other bits from the Reddit threads:**
- **ICON 3 cable:** the ICON 3 needs a proprietary "octopus" cable carrying the keyboard/trackball and ARCNET in/out. archeobits lacks it, so their ICON 3 doesn't work.
- **86Box suggestion:** a commenter suggests 86Box, since it emulates the NEC V20. This is unlikely to be enough on its own: the ICON needs 80186 on-chip peripherals plus custom video, speech and ARCNET hardware. MAME is the better fit.
- **QPaint:** a paint program that saved pictures as a C `.h` file you could compile into your program.
- **DOS games on later ICONs:** later DOS-capable ICONs ran games like DOOM and Scorched Earth.
- **Login-screen trojans** were a common student prank. Their ROM dumps and memory map would answer the open questions below and save months of work.

### Boot and system behaviour

- **Boot path confirmed:** the ICONs netboot from the LEXICON; Julian Dunn says the 80186 terminals boot off the central server [JD]. Wikipedia says the basic parts of QNX, including network file sharing, are **embedded in ROM** [WP]. That fits a ROM loader that pulls the rest from drive 8.
- **The LEXICON starts as superuser:** a former student says the LEXICON **boots in superuser mode** and runs a **boot file**, which they edited to copy the password file into their own directory [JD comments]. So there is a server-side start-up script, probably in `/boot` or `/config`.
- **Startup sounds:** the TMS5220 says "HELLO" at power-on [PGC]. Wikipedia describes it as a garbled word that sounds like "dhtick" [WP].
- **Local console on the LEXICON:** students plugged an EGA monitor into it [JD]. Eckert says the console only works during boot [JE2]. These may be different system versions.
- **A sign of a bus network:** unplugging any coax segment stopped every machine in the school [PR comments].
- **Speed:** with 20 stations on one LEXICON, students sometimes waited minutes for a response [OCS].

### More QNX behaviour and commands

- **Break keys:** `CTRL-RUBOUT` broke out of the window manager to a shell [JD comments]. After a superuser logged out, `CTRL-C` or `CTRL-RUBOUT` could drop you back to their `$` prompt [JD comments].
- **More commands:** `passcheck <user>` displayed a user's password [JD comments]; `chat` sent system-wide messages [WP]; `apb` as above.
- **Terminals as files:** each terminal was a tty you could redirect to or from [JD comments].
- **Root exploit:** QNX's `exec()` could clone an arbitrary PID and inherit its rights, which gave a root shell. This is the same anecdote as in §3, here from the person who found it [JD].
- **Documentation:** schools received printed UNIX-style man pages in binders [JD].
- **Default admin accounts** match [JE1]. The passwords are listed in [JE1] and [JD comments].
- **User ID 255,255** was a superuser [JD comments], which matches the book's "255 = administrator".

### Graphics hardware, from a developer

- **Original ICON:** 640×240 at 1 bit, or 320×240 at 2 bits, with colours chosen from a palette of 16 [OCS, Mark Stock].
- **A later model** added a 16-colour mode with a palette of 4096 [OCS].
- **The graphics were NAPLPS/Telidon-based** [WP]. Applications drew through the **Cemcorp Window Manager**, which made operations like flood fill slow [OCS].

### Models

- **Original ICON:** a short-lived dark-brown Microtel case, then beige [WP].
- **ICON 2:** detached keyboard with the trackball, more RAM, and room for an internal hard disk [WP].
- **Series III:** 386 CPU [WP].
- **ICON PC:** ran DOS and Windows 3.1, and one was fitted with 8 MB of RAM to act as a print server [PR comments].
- **"ICON-on-a-card":** a card that turned a PC into an ICON [WP].
- **Helicon Systems** made the Ambience GUI prototype and a MIDI interface for the original ICON [WP].

### Software developers identified

- **Logicus Inc.** (Ontario, 1986–89): Number Place, a paint program (likely Spectricon), Perfect Copy, and Shape Maker. It used C and the Cemcorp Window Manager [OCS].
- **Interactive Image Technologies, Toronto:** the Bartlett Saga, for the Ministry. Part I, *Refugees in the Wilderness*, was field-tested 1986–87 [W88].

## 6b. People to contact (public channels only)

Ranked by how likely each is to lead to the ROM dumps and disk images. Only public handles and channels these people posted themselves are listed here.

| # | Who | Why | How to reach |
|---|---|---|---|
| 1 | **u/archeobits** (Reddit) | The Toronto collector: working LEXICON, ICON 1/2/3, PW2 server; says they imaged every floppy and hard drive they found; asked people to contact them (2018) | Reddit message to u/archeobits, or a reply on the r/retrobattlestations post. **Their website, https://mirrors.archeobits.com/, has the same logo as the Reddit avatar.** As of 2026-10-14 it hosts only BBS and Commodore mirrors: no ICON material is public and no contact details are listed. Its robots.txt blocks bulk downloads of archive and disk files, so don't mirror it |
| 2 | **"Ryan"** (comment on Dean Yergens' blog, 24 Jun 2017) | Toronto; owns CEMCORP ICON, Unisys ICON, ICON II and a LEXICON; Kryoflux-imaged System 3.00.00 and 4.00.00. Very likely the same person as #1 | Reply to his comment on yergens.net, or ask Dean Yergens to pass a message on. Commenters' emails aren't public, but the blog owner can see them |
| 3 | **edgecase** (Giant Bomb) | Made the ROM dumps, disassembly and memory map (c. 2019); worked with the Toronto collector | Giant Bomb's forum has been replaced, so old private messages may not work. Retry the archived profile `giantbomb.com/profile/edgecase/` once the Wayback Machine is back up. **Contacting #1 is probably the fastest route to edgecase** |
| 4 | **Dean Yergens** (yergens.net) | Holds a dated set of 4.01–4.10 system disks, *including the fileserver boot diskettes*, plus QDOS and MS-DOS disks and Unisys manuals | Through his blog |
| 5 | **neozeed** (Jason Stevens, virtuallyfun.com) | Replied to edgecase on Giant Bomb; reverse-engineered QNX 1.2 booting; well connected in emulation circles | Comments on virtuallyfun.com |
| 6 | **snuci** (VCFed, Richmond Hill ON) | Kryoflux owner who chased the 2019 VCFed lot and pointed to his website in his post | VCFed private message |
| 6½ | **York University Computer Museum (YUCoM)** | Holds an ICON document archive plus an ICON, LEXICON and ICON 2; runs emulation projects; see §6c | museum.eecs.yorku.ca, Contact Us page |
| 7 | **Personal Computer Museum, Brantford** (pcmuseum.ca) | Had a working ICON on display; in contact with archeobits | Museum contact form |
| 8 | **Jason Eckert** (jasoneckert.github.io) | Has a working ICON + LEXICON booting from disk; may know the others | Contact details on his blog |
| — | Others wanting an emulator | Mike Ladouceur and Jason Ewert (Yergens comments); u/magicmijk (Reddit) | Possible collaborators, not sources |

**Suggested message:** you're building a documented ICON archive (book transcription, command reference, software catalog, hardware photos); you'd like to know whether the ROM dumps, memory map and disk images can be preserved (e.g. on archive.org); and you can offer something in return, such as your organized notes, Book of ICON transcription, or help sourcing archeobits' missing ICON 3 octopus cable.

## 6c. York University Computer Museum (YUCoM)

Source: [museum.eecs.yorku.ca/items/show/350](https://museum.eecs.yorku.ca/items/show/350), archived in `web_archive/yorku_museum_350/`. This is very likely the "university in Toronto" museum the Giant Bomb commenter said held donated ICON material [GB].

**What the museum holds:**
- **An "ICON archive":** described as an extensive collection of documents on the introduction and development of the ICON program.
- **CEMCorp ICON workstation,** s/n 11003684, on display.
- **LEXICON file server,** s/n **T-841201-219**, made April 1985. Its hard drive is a Micropolis-type ST-506 of 85 MB; the museum page spells it "Metropolis".
- **Unisys ICON 2,** s/n 345953715, made Nov 1987.
- **Unisys Model CA-3321 computer,** and a VGA 200 monitor from Feb 1991.

**New or confirming details:**
- **ROM:** 128 KB per the museum's spec, which agrees with the book's "up to 128 KB".
- **Keyboard:** 96 keys plus ACTION, HELP, PAUSE and **REBOOT** keys.
- **Display:** 12" mono at 640×240, or 13" colour at 320×240. The CRTC is listed as **HD46505 SP-1**.
- **Serial number format:** the LEXICON's s/n T-841201-219 has the same pattern as the sticker on Eckert's LEXICON board, "T-840929-261". So that sticker is probably a **unit serial with a build date** (29 Sep 1984).
- **Software program:** 500+ proposals were submitted and 57 lessonware projects funded (the $5M Exemplary Lessonware Project). OESS managed them and **TVOntario distributed** them; fewer than 100 titles were available by the end of 1987.
- **Software sources:** C and Logo came from Unisys; APL, BASIC, COBOL, Fortran and Pascal from Watcom; ALICE Pascal from Looking Glass Software, Toronto.
- **Software list:** the 1987 list (54 titles with authors and publishers) is merged into [ICON_software_catalog.md](ICON_software_catalog.md).

**Why they matter as a contact:** YUCoM runs hardware and software recovery and emulation projects of its own (an MCM/70 emulator, the NABU Network reconstruction, AES-90 software recovery). It is the best-placed institution to host ICON ROM and disk preservation, or to put you in touch with the collectors. Contact is through the "Contact Us" page on the museum site.

## 7. Open questions for this project

1. Where is the node ID actually stored: the EPROMs [JE2], the 32-byte EEPROM [Book], or jumpers?
2. What is the protocol between the ROM and the LEXICON when loading the RAM-disk image (drive 8)? A dump of the "Rel. 2.11" EPROMs is the most direct route.
3. Does the same Rel. 2.11 ROM decide between server and station roles from jumpers, or from whether a disk controller is present?
4. What is in `/boot` on a 3.x or 4.x system disk image?

**Status (2026-10-28), from the YorkU ROM dump and manuals.** See [ICON_firmware.md](ICON_firmware.md) and [manual_notes/](manual_notes/).

1. **Answered:** the node ID is stored in **NVRAM**, the 16×16-bit serial EEPROM, in word 10's high byte. [JE2] is wrong on this point.
2. **Mostly answered:** the protocol is an ARCNET `'seg'` request followed by `'b'` block replies (ICON_firmware §6). The server side, `/sys/bootsnd`/`bootrec`, still needs to be read from a disk image.
3. **Answered:** the same ROM has floppy, network and hard-disk loaders, chosen by NVRAM word 0 and by what succeeds. The role comes from configuration (`configure`, `attach_net`), not jumpers. The "Rel. 2.11" ROM is still undumped.
4. **Partly answered:** `/boot` holds `ws_boot`, `hd_boot` and `fs_net_boot` (SSOG p18).

**Further corrections from the manuals:**
- **§1, MS-DOS on an ICON:** MS-DOS 3.2 ran under **QDOS**, a Quantum BIOS emulator that needs ISSSW 4.00 or later, on a networked ICON. It did not run as a standalone floppy boot.
- **§2, stations per server:** 17 nodes per coax segment and 16 workstations per fileserver without an expander. The software limit is **32** per fileserver, and a network holds at most **128** nodes.
- **§2, fileserver disks:** they come in **10 MB or 64 MB** (the 64 MB ones as two 32 MB partitions, drives 1: and 3:). Fileservers take **Network** (node 1), **cluster**, **storage** and **stand-by** roles.
- **Default accounts in 3.x:** `root`/`superuser` and `icon`.
