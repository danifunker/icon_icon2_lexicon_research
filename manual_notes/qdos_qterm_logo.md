# Notes: QDOS, Qterm and Logo manuals (plus TMS4164 datasheet)

Sources read (source PDFs: YorkU `UNISYS_ICON/Manuals/`). "pN" means PDF page N, with the printed page in brackets where it is visible.

| Short name | Title | Release / part no. |
|---|---|---|
| **QDOS-Impl** | UNISYS ICON Series *QDOS Implementation Guide* | ISSSW 4.00, Apr 1989, Priced Item **1216157** (p3) |
| **QDOS-Ops** | UNISYS ICON Series *QDOS Operations and Reference Manual* | ISSSW 4.00, Jan 1989, Priced Item **1215944** (p3); documents **QDOS 4.01** (p5) |
| **Qterm-Ref** | Quantum *Qterm Reference Manual*, First Edition | Nov 1987, Quantum Software Systems Ltd. "For QNX Version 1.2 / 2.0 Release 5B or later" (p3–4) |
| **Qterm-Inst** | *QTERM Communication Package Installation Instructions* (ICONIX one-pager) | undated, ICON System 4.x era |
| **Logo-LG** | UNISYS ICON Series *Logo Learning Guide* | ISSSW 3.00 SA, July 1987, "Logo 2.00" (p1, p3) |
| datasheet_TMS4164.txt | TI TMS4164 | **No content.** The text is only the repeated watermark "This Material Copyrighted By Its Respective Manufacturer" (19 lines). Nothing to extract. |

Overall, these manuals tell us **nothing about QNX on-disk structures** (bitmap, inodes, directory entries or extents). They do give firm facts on drive-number allocation, on the DOS "pseudo-disk" container (a plain image held in a QNX file), on directory paths and config files, on workstation hardware and RAM, and on product names.

---

## 1. Filesystem-reader relevant facts

### 1.1 QNX drive-number allocation (ISSSW 4.00)

QDOS-Impl p50 [5-6], Figure 5-2 "QNX Drive Allocations". The OCR clipped the left edge, so the labels below are reconstructed:

```
1   1st Partition of Hard Disk
2   Fileserver Diskette Drive
3   2nd Partition of Hard Disk
4   (blank)
5   Workstation Diskette Drive
6-9 (blank)
A-F (allocated to DFS)
```
Quoted text: *"QNX has 15 drives (numbered hexadecimally 1-F)… The last six drives (A-F) are allocated to DFS."* The `mount` command shows the drive assignments.

Further points:
- QDOS-Impl p19–20 [1-7, 1-8]: `a=[4]1:/dos/MSDOS.pf` means *"the first partition (i.e. drive 1) of fileserver number 4"*. *"QNX drive 2 (the fileserver's diskette drive)"* and *"QNX drive 5 (the workstation diskette drive)"*. `b=[4]2` is the diskette drive of node 4, and `b=5` is your own workstation's diskette drive.
- QDOS-Ops p70 [5-20]: *"qnx_drive — The QNX drive number of the MS-DOS drive (1-8). This may include a node number."* The valid physical range is therefore 1–8, which fits drive 8 = RAM disk in our docs.
- Example 5 of `dfs start` (QDOS-Ops p57 [5-7]): `dfs start 16 b=2 c=3 +t` maps DFS B: to QNX drive 2 and C: to **QNX drive 3**. DFS treats drive 3 as a possible FAT volume, presumably a real DOS partition or a second hard disk.
- **Implications for the reader:** a fileserver hard disk can carry **two partitions, exposed as drives 1 and 3**. Drive 3 is not a second physical disk. Drive 4 is unassigned, and drives 6–9 are free (8 = RAM disk from other sources). The repo docs currently list only 1, 2 and 8, so 3 and 5 should be added.

### 1.2 DOS "pseudo-disk" container format

The repo calls this a "DOS virtual disk".

- A pseudo-disk is *"an ordinary QNX file"* that *"contain[s] an image of the MS-DOS file system"*. *"From MS-DOS, the contents of a pseudo-disk are exactly the same as that of an ordinary MS-DOS format diskette or hard disk"* (QDOS-Ops p22–23 [1-8, 1-9]).
- `make_dos_disk` *"creates a file which is an image of a MS-DOS diskette or hard disk **partition**"* (QDOS-Ops p93 [C-11]). For a pseudo-hard-disk, it *"creates a QNX pseudo-disk and provides it with a MS-DOS root directory"* (p25 [1-11]). The image is therefore almost certainly a **raw FAT volume starting with the boot sector/BPB at offset 0, with no MBR**. That is an inference, but the word "partition" points to it. No QNX-side header is documented.
- DFS error *"Unable to read block 1 of pseudo-disk xxxx Error 0000 — Cause: You are trying to access a file that is not a pseudo-disk"* (QDOS-Ops p78 [A-4]). DFS validates block 1 (QNX blocks are numbered from 1), which is the first 512 bytes of the file, i.e. the DOS boot sector/BPB.
- **Sizes.** Pseudo-diskettes must be **160K, 180K, 320K or 360K** (`+f` flag; default 360K). Pseudo-hard-disks range from **32 KB ("the MS-DOS minimum") to just under 32 MB ("the QNX and MS-DOS maximum")** (QDOS-Ops p23–25, p93; QDOS-Impl glossary p61). The 32 MB ceiling is both the FAT16 (pre-DOS 4) limit and, per this manual, the largest QNX file or disk size. That is a useful sanity bound for QNX2 block addressing.
- **Naming:** `.pf` is a pseudo-diskette and `.ph` a pseudo-hard-disk, by convention only (QDOS-Impl p21 [1-9]; QDOS-Ops p24 [1-10]). A floppy image can only be mounted as A:/B: and a hard image only as C:/D:, so a reader can use size or BPB to classify them. The `install_dos_pgm` limit for a pseudo-disk name is a "valid QNX filename up to 13 characters", and a start-up command name may be up to **16 characters** (QDOS-Impl p40 [4-4]). This is consistent with the QNX2 16-char filename limit.
- **Non-standard FAT geometry.** DFS drive_spec can override the BPB for odd media (QDOS-Ops p69–70 [5-19, 5-20]; p89 [C-7]):
  ```
  dos_drive=qnx_drive[(rwm)][,cluster_size,no_fat,no_dir,fat_size,fat_offset]
  cluster_size  The number of sectors in a cluster.
  no_fat        The number of File Allocation Tables on the disk.
  no_dir        The number of directory entries in the root directory.
  fat_size      The number of sectors in the FAT.
  fat_offset    The sector number of the first sector of the FAT.
  ```
  *"If any of the special specifications is required, all items must be supplied."*
- **Well-known files:** `/dos/MSDOS.pf` (bootable MS-DOS 3.2 pseudo-diskette), `/dos/work.ph` (master student work disk, kept empty, 32–1000 KB per install_dos_pgm p31 [3-5]), and `<user dir>/work.ph`. Examples in the manuals: `/dos/WORDSTAR.ph` (800K), `/dos/Bedford.ph`, `/dos/Dpaint.ph`, `/dos/application.work.ph`. The autoexec.bat shipped on MSDOS.pf is (QDOS-Ops p49 [4-3]):
  ```
  ECHO OFF
  PATH A:\
  PROMPT $P$G
  IF NOT EXIST C:GO.BAT GOTO EO
  C:
  GO
  :EO
  ```
  Each application pseudo-disk has a `C:GO.BAT` that ends with `QNX`, which exits back to the QNX shell.
- **Integrity:** *"QNX does not allow two users to open the same file or pseudo-disk for writing at the same time"* (QDOS-Ops p23). A pseudo-disk left "busy after a system failure" must be unbusied by killing virtual circuits (p77 [A-3]).

### 1.3 Text-file convention (affects any QNX file extractor)

QDOS-Impl p48 [5-4] and QDOS-Ops p52 [5-2] both say: *"in QNX each line is terminated by a record separator (RS)"*, whereas MS-DOS uses CR LF plus a ^Z EOF marker. QNX needs no EOF character. **QNX 2 text files use RS = 0x1E as the line terminator.** When RustyBackup extracts text files it should offer an RS→LF/CRLF conversion. DFS performs exactly this conversion in text mode.

### 1.4 Real MS-DOS diskettes in QNX drives

- QDOS and DFS can both read a real MS-DOS diskette in the fileserver drive (2) or a workstation drive (5), e.g. `dfs start a=[7]2`, `qdos b=[node]2,r` (QDOS-Impl p52 [5-8]; QDOS-Ops p39 [3-3]). The QDOS set-up menu shows such a drive as `[1]2 360K R` (QDOS-Ops p33 [2-5]). The QNX diskette driver could therefore handle 360K PC-format media in the same drive that holds native QNX diskettes. Native ICON QNX floppies are 640 KB per the repo docs, so the format differs.
- *"Warning: Do not attempt to write to MS-DOS diskettes from QNX. You should write to pseudo-disks only."* (QDOS-Impl p48 [5-4]).
- MS-DOS `FORMAT`, `DISKCOPY`, `DISKCOMP`, `FDISK`, `RECOVER` and `VERIFY` do not work under QDOS (QDOS-Ops p81 [B-1]).

### 1.5 QNX utilities named (structure-level)

These QNX commands "cannot be used to manipulate MS-DOS files" because they act on filesystem structure: **`chkfsys`, `dcheck`, `dcopy`, `dinit`, `query`** (QDOS-Impl p47 [5-3]). Other QNX commands used in the manuals: `ls`, `copy`, `frel`, `drel`, `cat`, `backup`, `arch` ("QNX arch command", an archive utility, p55 [5-11]), `chattr`, `files`, `eo` ("execute a command on a list of files", QDOS-Ops p43 [3-7]), `led` (line editor; the full-screen `edit` mangles tabs), `edit`, `mount`, `print`, `type #n`, `nv_config`, `patch` (`patch /cmds/qterm +c` sets change-owner), `stty` (`n=name` renames a tty), `timer` (Timer Administrator), `cron`, `comm`.

`backup` syntax examples:
- `backup [1]2:/ [1]1:/ +a +o +d p=^bitmap p=^copyright.notice` (Qterm-Inst step 1; the OCR shows `p="bitmap`, which almost certainly reads `p=^bitmap`). It installs a release diskette onto the hard disk **while excluding the diskette's root `bitmap` file and `copyright.notice`**. This supports the QNX2 layout of a **visible `/bitmap` file in the root of every volume**, which must not be copied between disks.
- `backup 1:/ 3:/ s=c +all` (Qterm-Ref p11).
- `backup a:/ [4]2:/ +a -p -v`, `backup b:/ a:/ +a -p -v` (QDOS-Impl p55, p57).
- Around the backup, `chattr [1]1:/cmds a=+c` … `chattr [1]1:/cmds a=c` toggles an attribute "c" on the `/cmds` **directory** (meaning not stated; possibly a "contiguous" or "change" attribute). `chattr file p=+rw`, `p=+e a=+e` and `n=newname` are also shown. So `chattr` has **p= permissions (r, w, e) and a= attributes (including c)**.
- User IDs take the form **group.member**. The super user is **255.255**, and "super-user group (255.XXX)" (Qterm-Ref p11, p26).
- QCP transfers *"the file's pathname, attributes, permissions, date and data"* (Qterm-Ref p9, p83). These are the per-file metadata fields a QNX2 directory entry carries.
- Xmodem warning: padding to 128 bytes makes *"a QNX load module… corrupt"* (Qterm-Ref p86). QNX executables are "load modules" whose length matters.

---

## 2. Boot, network and config facts

- **Node-numbered init files.** `/config/u_sys.init.NNNN` is loaded at boot by node NNNN. *"Remember that the node number must be represented in hex notation if you are working with a QNX 4.XX.XX system"*: for node 3 the file is `u_sys.init.0003`, and `type #n` prints the node's hex number (Qterm-Inst step 5). Here "QNX 4.XX.XX" means **ICON System Software 4.x**, not real QNX 4. Qterm-Ref (generic QNX) also refers to `sys.init` as the place to start `timer &`.
- **Node roles.** "Network Fileserver (node 1)", where install_dos_pgm runs, versus other fileservers. The Figure 3-2 fileserver picker lists *Node Name / Node Type / Node Number / Drive*, with types **"network"** (node 1, drives 1 and 3) and **"cluster"** (other nodes) (QDOS-Impl p29, p31, p33). This confirms the network-fileserver plus cluster-fileserver hierarchy. Workstations "boot from the Network Fileserver (node 1)" (p31 [3-5]).
- Node numbers can be at least 16 (`dfs stop 16`), and the Figure 3-2 OCR suggests three-digit node numbers.
- **Netcard** fileservers and workstations exist: the repo already lists the "Netcard FileServer Boot Diskette".
- **Devices:** `$mdm` (modem port), `$slpr` (serial printer), `$plpr` (parallel printer), `$sp` (spooler device), `$ttyNN`, `$termN`, `$iti01` (X.25). Spool dirs are `/spool/parallel` and `/spool/serial`. Example remote device: `[2]$mdm`.
- **Directories:** `/cmds` (system commands, with `/cmds/dfs` and `/cmds/dosfsys` DFS administrator task, `/cmds/qterm`), `/user_cmds` (site commands such as `start.qterm` and app launchers), `/dos_cmds` (MS-DOS app start-up commands, created by QDOS install), `/dos`, `/config` (`/config/dfs`, `/config/u_sys.init.NNNN`), `/user/<name>`, `/spool/...`, `/qterm` (`phone.dbase`, `config`, `modems`, `protocol`, `qnx.trm`, `vt100.trm`, `dial.qcl`, `login.qcl`, `hangup.qcl`, `quics.qcl`, `src/protocol.doc.z`), and `/languages` from the repo.
- `/config/dfs` format (QDOS-Ops p72–73 [5-22, 5-23]): lines starting with `"` are comments; keyword `text` or `binary`; then one extension per line, each **prefixed by a single TAB**; a bare TAB means "no extension". The sample contains text = asm, bat, lst, txt and binary = com, exe, (blank), obj.
- **Support:** the "ICONIX Hotline" is Toronto 495-8450 (also printed 485-8450), Ontario 1-800-387-5252, outside Ontario 1-800-263-ICON. Remote support works by dialling in at 2400 baud with Qterm; the "local CHIMO number" is an ANSI BBS (Qterm-Inst).
- No ROM, boot-loader, NVRAM self-test or diagnostic codes appear in any of these manuals. **Nothing matches the ROM codes 09–5a.** The only NVRAM-related item is the **`nv_config`** command, which reports workstation RAM (QDOS-Ops p27 [1-13], p75 [A-1]). This implies the RAM size is stored in or read from NVRAM, which links it to ROM error "11 : Missing or failed NVRAM".

---

## 3. Hardware facts

- **QDOS does NOT need an 80286 or an ICON2.** It runs on **ICON, ICON2 and Netcard** workstations (QDOS-Ops p19–22 [1-5…1-8]). QDOS *"emulates the BIOS"* and passes requests to QNX. MS-DOS code runs natively on the x86 CPU, and the 80186 in an ICON is enough.
  - **ICON**: *"does not have XTM"*. It is **monochrome** and supports only "standard" DOS apps that are well-behaved through the BIOS, plus apps that run in monochrome mode.
  - **ICON2**: has **XTM, an "XT emulator", a QNX task that uses ICON2 hardware to emulate some PC hardware components**. It **emulates CGA graphics**, and its **trackball emulates a Microsoft Mouse** (F11 = right button, F12 = left button; `-ms_mouse` disables it; the ICON2 Action keys do not work in QDOS) (p26 [1-12]; QDOS-Impl p38 [4-2]).
  - **Netcard**: real MS-DOS PCs with a network card, for example the **Unisys Personal Workstation2 (PW2) Series 300**, which has **EGA** and supports mono and CGA. An ICON2 keyboard with trackball can be attached to some Netcard PCs; then `a:tball` loads the mouse driver.
- **RAM vs. maximum DOS memory** (QDOS-Ops p27–28 [1-13, 1-14]):
  ```
  384 KB workstation -> apps up to 128 KB
  512 KB workstation -> apps up to 256 KB
  1 MB ICON          -> apps up to 550 KB
  ICON2              -> apps up to 610 KB   ("610Kbytes on a 1 MByte ICON2", p75 [A-1])
  1.6 MB PC          -> 640 KB
  ```
  The print spooler costs about 20 KB (`-p` frees it). qdos `m=` defaults to 192K (quick ref) or the maximum available. **The ICON2 is a 1 MB machine.**
- Supported DOS versions: **PC-DOS 2.1, PC-DOS 3.1, PC-DOS 3.2, MS-DOS 3.2** (QDOS-Ops p77 [A-3]). The Unisys-supplied version is MS-DOS 3.2.
- `qdos` options `-floppy` and `-keyboard` select the *BIOS* diskette or keyboard driver instead of QNX's (for Netcard PCs). The set-up menu options 6 and 7 toggle QNX/BIOS, and *"QDOS works properly only if option 6 [and 7] is set to QNX"* (p32–33). `make_dos_disk d=drive` is *"Not usable on the ICON"*.
- MS-DOS `GRAPHICS` and `MODE` do not work on the ICON. *"No communications package accessing a modem device will work under QDOS."* `DATE` sets only the local date, and `TIME` cannot reset the ICON clock. `BASIC`/`BASICA` (ROM BASIC) do not work, so GW-BASIC is supplied separately (QDOS-Ops p81–82).
- Copy-protected programs mostly will not run (p27).
- ICON keyboard: there is no labelled Alt/Home/PgUp; Appendix D maps PC keys onto ICON keys, and the ICON2 has them labelled. Scrolling edit boxes on the ICON uses *"the blank key adjacent to the ACTION key"* plus the trackball, and on the ICON2 uses F12 plus the trackball (QDOS-Impl p59). Alt+Del enters the QDOS set-up menu.
- **Logo colour** (Logo-LG p103–105 [91–93]): 4 "inkpots" (0–3; 0 = background) chosen from **16 colours** (0 black, 1 dark red, 2 dark green, 3 dark yellow, 4 dark blue, 5 dark violet, 6 dark turquoise, 7 grey, 8 dark grey, 9 red, 10 green, 11 yellow, 12 blue, 13 violet, 14 turquoise, 15 white). The display can therefore show **4 simultaneous colours from a 16-colour palette** in Logo's graphics mode. Screen print is **Ctrl+Shift+8**: a lit bar sweeps down the screen and the dump goes to the shared printer (p27 [15]). Printed aspect needs `setscrunch .6`.
- Qterm "zoom" (80×25 to 80×43 on EGA) needs QNX 2.1+ (generic QNX PC behaviour, not ICON).

---

## 4. Software and version facts

### QDOS package (QDOS-Impl, QDOS-Ops)
- **QDOS 4.01** needs **ISSSW 4.00 or later** and Unisys-supplied **MS-DOS 3.2**. Install order: QDOS, then MS-DOS, then apps, all via `install_software` (non-system software, *Software Installation and Implementation Guide* §7). QDOS goes on fileservers only.
- Package contents: `qdos` (BIOS emulator, *"Quantum Trademark"*), **DFS** (`/cmds/dfs` command interpreter plus `/cmds/dosfsys` administrator; *"a product of Quantum Software Systems, Ltd."*, portions © 1987 Quantum), `make_dos_disk`, `install_dos_pgm` (menu installer, runs only on node 1). The **XTM** task lives on the ICON2. QDOS was thus a Quantum product (QNX's maker), OEM'd by Unisys.
- DFS commands: `dfs start|add|rem|mode|query|stop [node]`. Permissions are `(rwm)`, where m = create/delete directories. `F=` sets open files (min/default 10) and `N=` sets buffers (min/default 20). Mode options are `+t`, `+b`, `+d` and `D=t|b`, extension lists are `B=`, `T=` and `E=file`, and per-file suffixes `^t`/`^b` force text or binary (OCR shows "t / "b). DFS on a fileserver serves all workstations: `ls [4]a:/`. Only Site Administrators may use these except `dfs query`.
- `qdos` syntax (QDOS-Ops p85 [C-3]): `qdos [m=size] [-floppy] [-keyboard] [a=..] [b=..] [c=..] [d=..] [s1=serial] [p1=printer] [-hold] [-verbose] [s2=..] [p2=..] [p=priority] [-menu] [+banner] [-ms_mouse] [-print_spooler]`. Drive values look like `[node]drive` or `path[,r|,rw]`. Printer values look like `$sp,[node]/spool/parallel`, `$plpr` or a file name.
- **Verified MS-DOS applications** (install_dos_pgm list, QDOS-Impl p32 Fig 3-1; heavy OCR, corrected): Typing Tutor IV; Bedford Integrated Accounting v3.23c (1988); dBase (III Plus or IV?) v1.1 (OCR "Dhn IV Version 1.1"); Deluxe Paint II; Harvard Presentation Graphics (1986); Microsoft Flight Simulator v3.0; Smart (1986; Innovative Software "Smart" integrated suite, probable); Speed Reading Tutor IV; Turbo Pascal v5.0; VP-Planner Plus (1987); WordPerfect v5.0; WordStar Professional Release 4; Xerox Ventura Publisher v1.1. WordStar is the worked example in QDOS-Ops §4.

### Qterm (Qterm-Ref, Qterm-Inst)
- A Quantum product (Kanata, Ontario). It comprises `qterm` (with subtasks `qterm.kbd` and `qterm.mdm`), `qcp` (QNX Communication Protocol: CRC-16, 2048-byte packets of 8×256-byte subpackets, sends path, attributes, permissions and date), `xmodem` (Xmodem/Modem7), `qcl` (QNX Communications Language, a scripting language), and `qterm_util`. Version banner: "Qterm V1.00", "QCP V1.00", "XMODEM V1.00". The remote enquiry `ESC ^E` returns "Q10".
- QUICS is Quantum's on-line update system. The ICON package adds a QTERM SETUP diskette with `install.util`, `start.qterm` (`qnx_tty_start` / `qterm m=[node]dev -h +m +b …` / `qnx_tty_stop`), an ANSI-capable qterm core, a public dialing directory, and the `u_sys.init.node` template.

### Logo (Logo-LG)
- "ICON Series Logo", **Logo 2.00**, ISSSW 3.00 (July 1987). Opening banner: **"LOGO: Version 2"**. Started with `% logo`; exit with `goodbye`. Saved workspaces are "notebooks", files ending **`.logo`** (`save "name`, `load "name`, `catalogue`, `erasefile`). An auto-load file named **`startup.logo`** is created with the ICON editor. It has packages, `bury`/`unbury`, and the trackball-driven Logo editor. Prerequisite manual: *ICON Series Operations Guide, Vol. 1: Getting Started*. The guide covers both ICON and ICON2. The repo catalog lists the disk "ICON Series Logo – Ver 3.00.00 (Dec 1987)", so Logo 2.00 is the language version and 3.00.00 the ISSSW release number.

### Other manuals referenced
*ICON Series Software Installation and Implementation Guide*; *ICON Series Administration Guide*; *ICON Series System Software Operation(s) Guide*; *ICON2 Keyboard Installation Guide*; *ICON Series Operations Guide, Vol. 1: Getting Started*; *MS-DOS manual*.

---

## 5. Titles for ICON_software_catalog.md

| Title | Type | Source |
|---|---|---|
| QDOS 4.01 (Quantum BIOS emulator for MS-DOS) | System | QDOS-Ops p5 |
| DFS – DOS File System (`dfs`, `dosfsys`) (Quantum) | System utility | QDOS-Ops p52 |
| make_dos_disk | Utility | QDOS-Ops C-11 |
| install_dos_pgm | Utility | QDOS-Impl §3 |
| install_software | Utility | QDOS-Impl p23 |
| XTM (XT emulator task, ICON2) | System | QDOS-Ops p21 |
| MS-DOS 3.2 (Unisys-supplied); PC-DOS 2.1/3.1/3.2 supported | OS | QDOS-Ops A-3 |
| Qterm / QCP / QCL / xmodem / qterm_util V1.00 (Quantum, 1987) | Communications | Qterm-Ref |
| QTERM Communication Package + QTERM SETUP diskette (ICONIX) | Communications | Qterm-Inst |
| Logo 2.00 ("LOGO: Version 2") | Language | Logo-LG |
| Typing Tutor IV | DOS app (verified) | QDOS-Impl Fig 3-1 |
| Bedford Integrated Accounting 3.23c (1988) | DOS app | idem |
| dBase (III+/IV?) 1.1 | DOS app | idem (OCR unclear) |
| Deluxe Paint II | DOS app | idem |
| Harvard Presentation Graphics (1986) | DOS app | idem |
| Microsoft Flight Simulator 3.0 | DOS app | idem |
| Smart (1986) | DOS app | idem (OCR "S•art") |
| Speed Reading Tutor IV | DOS app | idem |
| Turbo Pascal 5.0 | DOS app | idem |
| VP-Planner Plus (1987) | DOS app | idem |
| WordPerfect 5.0 | DOS app | idem |
| WordStar Professional Release 4 | DOS app | idem; QDOS-Ops §4 |
| Xerox Ventura Publisher 1.1 | DOS app | idem |
| "Other" (non-verified apps) | — | QDOS-Impl §4 |

---

## 6. Corrections and contradictions to the repo docs

1. **ICON_web_research.md l.33** says the standalone ICON could run MS-DOS 3.2 [AA], with VC guessing it needed the WD controller and other jumpers. **Correction:** MS-DOS 3.2 on ICONs ran under **QDOS on a networked ICON**, booting from a pseudo-diskette on the fileserver. No standalone floppy boot or special controller is needed. The ICON (80186) is explicitly supported, though monochrome only and with "standard" apps only. The Anjo memory may conflate the two.
2. **Drive numbering:** the repo lists only `1` (HD), `2` (floppy) and `8` (RAM disk). Add **`3` = second partition of the fileserver hard disk**, **`5` = workstation (local) diskette drive**, **`A`–`F` = DFS virtual drives**, and the note that drives are numbered **hex 1–F**, with physical drives 1–8.
3. **ICON_software_catalog.md l.271:** "*QDOS Operations and Reference Manual* (1215944/1216157)" mixes up two manuals. **1215944** = QDOS Operations and Reference Manual (Jan 1989). **1216157** = QDOS Implementation Guide (Apr 1989).
4. **ICON_software_catalog.md l.26** gives QDOS as just "Unisys". It is a **Quantum Software Systems** product (QDOS is "Quantum Trademark", DFS is "a product of Quantum"), packaged by Unisys. ICON_web_research l.124 says QDOS was for "later systems"; more precisely, it **needs ISSSW ≥ 4.00** (QDOS 4.01).
5. **Node-number files in hex:** in System 4.x, node-specific config file names use **4-digit hex** (`u_sys.init.0003`). Check whether the repo's boot docs assume decimal.
6. **"QNX 4.XX.XX"** in the ICONIX Qterm instructions means ICON System 4.x (QNX 2-based), not QNX 4. Readers of our docs should not misread it.
7. **ICON2 RAM:** the QDOS manual implies the **ICON2 is a 1 MB machine** (610 KB DOS max) and that ICONs came in **384 KB, 512 KB and 1 MB** variants. This matches the repo's 384→512 KB on board plus the upgrade board.
8. **Logo:** the repo calls it "Logo / Logo français (Natal Logo)". This manual calls it simply "ICON Series Logo", **version 2.00**, with no vendor stated (the repo's "C and Logo came from Unisys" is not contradicted). Saved files use the `.logo` extension, and `startup.logo` loads automatically.
