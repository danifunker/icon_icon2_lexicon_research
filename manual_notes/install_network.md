# Installation and network manuals: findings

Covers four manuals (source PDFs: YorkU `UNISYS_ICON/Manuals/`). Short names used in citations:

| Short name | Manual | Priced item |
|---|---|---|
| **NPG** | ICON Series *Network Planning Guide*, Aug 1987 (96 PDF pp) | 1212090 |
| **SSIG300** | ICON Series *System Software Installation Guide*, ISSSW 3.00 SA, Jul 1987 (116 pp) | 1212875 |
| **SIIG400** | ICON Series *Software Installation and Implementation Guide*, ISSSW 4.00 EZ, Jan 1989 (58 pp) | 1215936 |
| **SIIG401** | Same title, ISSSW 4.01 EZ, Jan 1990 (56 pp) | 2992343 |

Citations are "manual pPDF (printed page)". Other manuals referenced but **not** in our set: *ICON Series Fileserver Installation Guide*, *ICON Series Netcard Fileserver and Workstation Implementation Guide*, *ICON Series Network Expander Operations Guide*, *ICON2 Keyboard Installation Guide*.

Overall: these are procedural manuals for administrators. They contain **no on-disk structure detail, no ROM error codes, no jumper or EEPROM documentation**. Their value is in the network model (node numbers, fileserver roles), the boot-source rules, the diskette set and its use, disk-size and partition facts, and the version-to-version changes.

---

## 1. Facts relevant to a QNX 2 filesystem reader

| Fact | Source |
|---|---|
| **"Old format" 10 MB fileserver:** body text in both 4.x guides says *"If the unit is a 10 Mbyte fileserver, run the query command. If the total number of blocks returned is **20808**, the fileserver is of the old format and must be initialized. Otherwise the fileserver is of the new format."* | SIIG400 p27 (4-1); SIIG401 p25 (4-1), p30 (5-2) |
| **Contradiction:** the glossary of both 4.x guides says *"A 10 Mbyte ICON Series fileserver with **20208** blocks of allocated storage space"*. The body figure (20808) appears 3 times and the glossary figure twice. Treat 20808 as more likely but **check both** against a real image. 20808 × 512 = 10,653,696 bytes; 20208 × 512 = 10,346,496 bytes. | SIIG400 p52; SIIG401 p51 |
| **The old format predates 3.00.** SSIG300's status messages already include *"Cannot Install - The Disk Has Old Format: You cannot install any software onto a fileserver that still has its disk in the old format; you will have to backup the fileserver ... then initialize the fileserver"*. So a 10 MB hard-disk layout change happened at or before 3.00, probably the QNX 1.x to 2.x transition. A reader should expect **two 10 MB hard-disk layouts**: the old one (20808/20208 blocks) and the new one (some other block count). | SSIG300 p108 (A-2) |
| Under 4.x, an old-format fileserver is **forced** to reformat and initialize. "You do not have the option not to initialize an old format fileserver." | SIIG400 p28, p32; SIIG401 p26, p30 |
| **64 MB disks are split into two 32 MB partitions, each seen as a separate drive.** "A 10 Mbyte fileserver has one drive; a 64 Mbyte fileserver is partitioned into two drives." | SSIG300 p104–105; SIIG400 p51, p53; SIIG401 p50–51 |
| **Drive numbers:** partition 1 = drive **1**, partition 2 = drive **3**. Drive 2 is the floppy. *"Default search order is drive 1 for 10MByte fileservers or drive 1 3 for 64MByte disks where drive 1 is partition 1 of locally mounted hard disk or cluster fileserver and 3 is partition 2 ... These drives are mounted as REM disk on the local node."* | NPG p39 (1-27) |
| Example search order `1 3 2` on a fileserver. Square-bracketed numbers are nodes and bare numbers are drives: `[3]1:/`, `[1]1:/`, `[1]3:/`, `[1]2:/`. | NPG p37 (1-25) |
| Initialization (configure's "Initialize this fileserver") **erases only the first partition**. The second partition is initialized separately (per the Administration Guide). | SSIG300 p17 (1-5), p51 (4-5), p71 (5-5) |
| Initialization times: 10 MB disk about 10 min; one 32 MB partition of a 64 MB disk about 30 min. Configuring without initializing: 5 min. Installing system software: 90–120 min (3.00). | SSIG300 p17; SIIG400 p29; SIIG401 p27 |
| **Bootable diskettes carry hidden boot files.** *"A bootable diskette is one that contains special hidden boot files which allow you to boot a node in the network. Boot diskettes have the word 'bootable' printed on the label."* A reader should look for boot code or hidden files outside the normal directory tree on boot diskettes. | SIIG400 p14 (1-4); SIIG401 p15 (1-3) |
| **Package sizes in 512-byte blocks:** "ICON Series SSW Release 3.00.00" shows as `?700 blocks`. The OCR is garbled ("H00", "i700", "mo"), but 4700 is the likely value, since 4 floppies × ~1276 free blocks ≈ 5100. "ICON Series On_line Help Rel. 3.00.00" is **575 blocks**. | SSIG300 p34, p88 (7-6) |
| install_software reads "package information" (name and size) from **the first diskette of each package**. So each package's disk 1 carries a machine-readable package descriptor, probably a file on the diskette. It also detects a wrong diskette in the sequence ("informs you of your error and waits for the correct diskette"), so each diskette carries an identity (package name plus disk number N of M). | SSIG300 p33 (3-7), p87 (7-5); SIIG401 p38 (7-2) |
| Disk-space accounting: install_software works out the free space on each target fileserver and refuses packages that don't fit. | SSIG300 p18 (1-6) |
| QDOS (4.00) stores MS-DOS disks as **"pseudo-disks": ordinary QNX files holding an image of an MS-DOS file system**. A QNX reader that finds these could hand them to a FAT parser. | QDOS Ops & Ref Manual (4.00) p22 (1-8), cross-reference |

## 2. Boot behaviour and roles (relevant to the boot ROM)

### 2.1 Boot sources a node can use
- *"A fileserver may be booted from its own hard disk, from a diskette, from the Network Fileserver or from a cluster fileserver."* (SIIG400 p13; SIIG401 p14)
- **Floppy boot:** insert a bootable diskette, **lock the drive lever** (3.00: "turn the lever clockwise"), and power on.
- **Hard-disk boot:** make sure the **diskette lever is unlocked** ("facing straight up") and the OS is on the hard disk, then power on. (SSIG300 p16–17; SIIG401 p14)
- **Network boot of a machine that has a hard disk:** a storage fileserver has no OS on its disk, and *"turn it off, wait 30 seconds, make sure its diskette drive door is open and reboot it. The operating system will be loaded from the cluster fileserver."* (SSIG300 p93, 7-11)
- **Inferred ROM loader order:** floppy (if a lever-locked bootable diskette is present), then the local hard disk (if it holds an OS), then a network boot from the cluster fileserver. This fits the ROM strings "no loader specified" and "no specified loader option has succeeded" (codes 55 and 5a): the ROM tries a list of loader options. **The manuals do not document the ROM codes.** Also, nothing here says whether the loader list comes from NVRAM or jumpers.
- Workstations have no disk and always boot from their cluster fileserver. The fileserver must be up first. Reboot means off, wait (30 s in 3.00, 10 s in 4.x), then on. (SSIG300 p17; SIIG401 p15)
- After rebooting a fileserver from its hard disk, *"Wait 5 minutes before"* rebooting the workstation. (SSIG300 p45, p69, p83)

### 2.2 Fileserver front panel and self-test (3.00)
*"The front panel of the fileserver displays a red power light, a yellow trouble light and two red activity lights."* The yellow trouble light *"briefly flashes when you first turn on the fileserver but should not stay on. If this light stays on or if it starts to flash more than once, it is indicating a problem ... Note: If you have an older model fileserver, you will not have the yellow trouble light."* The two red activity lights are the diskette drive and the hard disk.
*"Everytime you turn on the fileserver, it runs a series of automatic self-tests ... If the two red activity lights are off, and the yellow trouble light does not flash on and off more than once and does not stay on, the fileserver has passed its self-tests and booted successfully."* Failures are referred to the troubleshooting section of the *Administration Guide* (Vol 3 Diagnostics) or the *Fileserver Installation Guide*. (SSIG300 p24–25, 2-2/2-3)
→ A ROM self-test failure on a headless fileserver is reported by **flashing the yellow trouble light**. The codes themselves are probably documented in Admin Guide Vol 3, which is not my assignment.

### 2.3 Node numbers

| Rule | Source |
|---|---|
| Network Fileserver is **always node 1** | all |
| **Factory default node for every fileserver is 245**. *"All fileservers are the same when received from the factory. You decide what type it will be."* | NPG p17; SSIG300 p25, p105 |
| **Factory default node for every workstation is 254.** *"All new workstations have the default node number 254. If you turn on more than one new workstation you will have node number conflicts."* Hence "turn on only one new workstation at a time". | NPG p17; SIIG401 p34 (6-2) |
| Fileserver nodes: 3.00 says 2–253 except 245. 4.x: *"Valid nodes for a fileserver are from 2 to 244 and from 246 to 253."* | SSIG300 p71, p79; SIIG400 p32; SIIG401 p30 |
| Workstation nodes: *"Valid workstation numbers are 16 to 253, with the exception of 245"* (SSIG300, SIIG400, SIIG401). **NPG instead says 17 to 254 except 245** (where 254 is the factory default). | SSIG300 p96; SIIG401 p34; NPG p17, p20, p88 |
| Global range is 1–253 (245 reserved for factory-fresh fileservers) | glossaries |
| attach_net assigns "the next available node number" unless you give `n=`. The cluster (boot fileserver) is set with `c=`; without it, the station boots from node 1. | SIIG401 p34 |
| NPG suggests reserving 16 consecutive numbers per segment (e.g. 17–32, then 33–48). | NPG p20 (1-8) |

**On the "where is the node ID stored" question:** these manuals show that the node number is **set in software** (attach_net for workstations, configure for fileservers), changes from a factory default (254/245) to the assigned value, and survives reboots. It must therefore live in **writable non-volatile storage on each unit** (the 32-byte EEPROM/NVRAM fits), **not in EPROM or jumpers**. This contradicts [JE2]'s claim that Burroughs set it in the EPROMs at order time. The process: a new workstation boots as node 254, the administrator logs in on it and runs `attach_net`, then reboots it. So attach_net runs **on the workstation itself** and writes its node and its boot (cluster) node locally. The ROM must therefore also store the **cluster/boot-server node** (c=). Changing a node requires `detach_net` first. (SIIG401 p33–35; SSIG300 p96–97)

`[245]configure` (3.00): to configure a newly added fileserver, you boot it from the Configuration & Installation diskette and then, from a workstation, run configure **remotely on node 245**. *"Do not put a space between [245] and configure."* In 4.x, "The program will select the correct fileserver to be configured." (SSIG300 p69, p77; SIIG401 p30)

## 3. Fileserver types and network topology

### 3.1 Roles
| Role | What it is | Source |
|---|---|---|
| **Network Fileserver** | Node 1. The only holder of **net.config** (list of assigned nodes, and in 4.x fileserver names) and of the **Master Password File** (user names, numbers, passwords, home directories, login program). 3.00 glossary adds the **"Poller program"**. Also acts as the cluster fileserver of logical cluster 1. attach_net, detach_net and user changes need it reachable. | NPG p31–32; SSIG300 p104 |
| **Cluster fileserver** | Boots from its own hard disk. Holds *"The boot files of all the machines in the cluster (/boot)"*, the OS commands and utilities (/cmds, /util, etc.), optionally user directories and applications, and a cluster password file **/config/pass**. Its node number **is** the cluster number. At most **32 workstations** (workstations plus storage fileservers) can boot from it at once. | NPG p32–33 |
| **Storage fileserver** | Has a hard disk but **no OS**. It **boots over the network from its cluster fileserver**, no nodes boot from it, and it holds no password, print-format or other OS files. It holds user homes, archives, libraries and applications. A printer attached to it gets its daemon and spooling locally. System software cannot be installed on it ("Cannot Install O/S On Storage Fileserver"). | NPG p33; SSIG300 p14, p107 |
| **Stand-by fileserver** | Optional. *"has all the properties of a cluster fileserver"*, holds copies of net.config and the Master Password File plus boot files and commands, and should have high node numbers. If the Network Fileserver fails, *"you can change the node number of the stand-by fileserver to that of the failed Network Fileserver using the 'Configuration and Installation Diskette'"*. | NPG p34, p36 (1-22, 1-24) |
| **ICON fileserver** | ICON-hardware fileserver (LEXICON-type). Booted with the *ICON Fileserver Boot Diskette*. Sizes: 10 MB or 64 MB. | SIIG401 p16 |
| **Netcard fileserver** (4.x) | *"fileservers that have a Netcard installed"*. You may install the Netcard yourself, in which case you run **setup** from the Configuration Diskette; ICON or pre-assembled Netcard fileservers skip this. Its UI uses PC keys: **Scroll Lock = Action**, Enter = OK, Esc = Cancel, **Print Screen = Help**, and cursor keys move the pointer. It may run headless ("If the fileserver does not have a monitor refer to the appropriate installation guide"). | SIIG400 p18, p23, p47–49; SIIG401 p45–47 |
| **Netcard workstation** (4.x) | *"Netcard computers are MS-DOS personal computers containing hardware features that support a wide variety of MS-DOS applications"*, e.g. a **Unisys Personal Workstation2 Series 300** (EGA). Some can take an ICON2 keyboard with trackball. | QDOS Ops & Ref p22, p26 (cross-reference) |

So: **Netcard = an ICONET/ARCNET network adapter card for an IBM-compatible PC**, letting a PC act as a QNX fileserver or workstation on the ICON network. The manuals never say "ARCNET card" outright, but the card replaces the ICON's network hardware, and NPG says ICONET is a "modified ARCNET". 4.00 introduced the type. Workstation types in 4.x are ICON, ICON2 and Netcard. ICON2 has F11/F12 next to the Action keys and contains an XT emulator (XTM) task for QDOS. (SIIG401 p42; QDOS Ops p21)

### 3.2 Topology limits (NPG, 1987)
- **Basic network:** at most **17 nodes** (1 fileserver plus 16 workstations) and **61 m (200 ft)** of coax end to end. (NPG p17, 1-5)
- **Network Expander:** *"There are two ARCNET ports and two ICONET ports on Network Expanders."* ICONET ports drive **segments**. ARCNET ports link **expander to expander**. The expander regenerates the signal ("boosting"). It has a **keyswitch with an "Isolate" position** that cuts a segment off. (NPG p15, p20, p24, p31)
- **Segment:** at most 17 nodes and 61 m. Two segments per expander. The term is purely physical; "There is nothing in the software that defines a segment." (NPG p20)
- **ARCNET link between expanders:** at most **300 m (1000 ft)**.
- **Software limit:** at most **32 workstations booting from one fileserver**.
- **Whole network:** at most **128 nodes** and **6,000 m (20,000 ft)** of cable, *"minus a load of 166 metres (500 feet) per Network Expander"*. (NPG p45, 2-3; SSIG300 p14)
- **Physical cluster** = everything on one expander. **Logical cluster** = a cluster fileserver plus everything that boots from it. The two need not match. (NPG p27)
- Standard inter-unit cable is 2 m (6.67 ft). (NPG p14)
- **Implication:** ICONET (the workstation coax) and ARCNET (between expanders) are **electrically different ports**, even though "the cables attached to the two kinds of ports look the same". This supports "modified ARCNET" for ICONET: the protocol is ARCNET and the physical layer differs.

### 3.3 Shell search order and path
Every node has a **search order** (nodes and drives), and the shell has a **path**. Path syntax uses `!` as the separator and `!!` to mean the current directory at the end, e.g. `!/cmds/!/user_cmds/!/util/!!`. The search order is outer and the path inner: `[3]1:/cmds/speak`, `[1]1:/cmds/speak`, `[1]3:/cmds/speak`, `[1]2:/cmds/speak`, then `[3]1:/user_cmds/speak` ... The default workstation search order is `[bootnode] 1` (3 as well on 64 MB disks). Remote drives are **"REM disks" (remote mounted disks)**. (NPG p36–39, 1-24..1-27)

## 4. The diskette set and how each disk is used

### 4.1 What the manuals list
**3.00 (1987):** *"Your System Software 3.00 package contains four diskettes plus a Configuration and Installation diskette."* (SSIG300 p27, 3-1). The Configuration and Installation Diskette **is bootable**: you put it in the Network Fileserver and power on. *"Caution: Do not attach a write protect tab to the Configuration and Installation diskette"* (SSIG300 p15), so it is **written to during configuration**. On-line Help is a **separate package**, "ICON Series On_line Help Rel. 3.00.00", 575 blocks (SSIG300 p88).

**4.00 / 4.01:** *"Your System Software release consists of the following diskettes:"* (SIIG400 p14–15; SIIG401 p15–16)
> 1. ICON Series ICON Fileserver Boot Diskette — Use this diskette to boot ICON fileservers only
> 2. ICON Series Netcard Fileserver Boot Diskette — Use this diskette to boot fileservers that have a Netcard installed.
> 3. ICON Series Configuration Diskette — This diskette can be used to run various utilities, including [setup and] configure, on either ICON fileservers or Netcard fileservers. It is not a bootable diskette.
> 4. Diskettes labelled ICON Series System Software — Insert these diskettes as requested by the configure utility.

Patches: *"If you have received a patch as well as a full release of the System Software, you must run the configure program twice—once for the full release, and once for the patch. Be sure to install the full release ... first."* A patch is *"a small part of the operating system"*. *"You should not initialize the Network Fileserver if you are upgrading with a System Software patch release. If you do this only the patch ... will be installed, and the fileserver will not function."* (SIIG401 p18, p23, p26)

### 4.2 Mapping the user's disk images

| Image title | What the manuals say | Confidence |
|---|---|---|
| **ICON FILESERVER BOOT DISKETTE** | 4.x item 1. Bootable, with "special hidden boot files". Boots an ICON-hardware fileserver (LEXICON) from floppy with the drive lever locked. You log in as root/superuser, then **remove it** and insert the Configuration Diskette. In 3.00 its role was filled by the bootable Configuration & Installation diskette. Expect a minimal QNX boot image, drivers and a login, plus whatever lets a workstation boot from this fileserver over the net (step 2 of the procedure boots a workstation off the floppy-booted fileserver). | High |
| **NETCARD FILESERVER BOOT DISKETTE** | 4.x item 2. Boots a PC fileserver with a Netcard. Its boot loader must therefore be **PC-BIOS bootable** (x86 boot sector), unlike the ICON one, which the ICON ROM loads. Worth comparing sector 0 of the two images. | High (role); boot sector difference is inference |
| **CONFIGURATION DISKETTE** | 4.x item 3. **Not bootable.** Holds `configure` and (4.00) `setup`, plus "various utilities". You run it from the floppy after booting from the boot diskette. configure asks for the fileserver type (Network/Cluster/Storage), whether to initialize, node ID, cluster node ID and fileserver name (≤16 characters, stored in net.config), then prompts for the System Software diskettes. It is also used to renumber a stand-by fileserver. In 3.00 it was the bootable "Configuration and Installation Diskette" and must not be write-protected. | High |
| **SYSTEM SW DISKETTE 4.10.01 DISK 1 OF 5** (to 5 OF 5) | 4.x item 4, "Diskettes labelled ICON Series System Software", requested in order by configure. Copied onto the hard disk of the Network Fileserver and (4.01) "to all fileservers on the network" that hold the OS, i.e. cluster fileservers. 3.00 had 4 such diskettes; 4.10 has 5. Disk 1 should hold the package descriptor (name, size in blocks). The manuals don't cover 4.10 itself. | High (role) |
| **HELP DISKETTE** | Not named in the 4.x guides. In 3.00, help shipped as a separate package, "ICON Series On_line Help Rel. 3.00.00" (575 blocks, about half a floppy), installed with install_software. The help *command* (`help <cmd>`) presumably reads this. Most likely the same thing for 4.x. | Medium (inference) |
| **UPGRADE DISKETTE** | **Not mentioned** in any of my four manuals. Possibilities: a utility disk for upgrading from a previous release (detaching and reattaching old-format servers, data migration), or the 4.10-era equivalent of "Section 4 Upgrading". Inspect its contents. | Unknown |
| **4.10.05 PATCH** | A "patch (system update)" / "System Software patch diskettes": a small part of the OS. Applied by **running configure a second time** after the full release, and never with "initialize" selected. | High (role) |

### 4.3 Installation procedures (condensed)

**3.00 new network** (SSIG300 §3, p27–45):
1. Put the Configuration & Installation diskette in the Network Fileserver and power on. Wait until the diskette activity light has been off for 10 s, then power on a workstation.
2. Log in on the workstation as `root` / `superuser`, then run `$ configure` ("Checking the net.config file. Please wait..."). Choose fileserver type **Network Fileserver** (menu: Network / Cluster / Storage), then *Initialize this fileserver* (screen text: "the Network Fileserver (Node ID = 1)"), then PERFORM.
3. Run `$ install_software`. Swap in SSW disk 1, *Enter Package* (it reads the name and size), *Continue*, request installation per node (status shows "Install (The Disk Is Empty)" or "Install (Disk Files Preserved)"), *Start Install*, then insert disks 2–4 when asked ("Please insert diskette 2 of ICON Series SSW Release 3.00.00 in the diskette drive of node one.").
4. Remove the floppy, reboot the fileserver from its hard disk, wait 5 min, then reboot the workstation.
5. Cluster and storage fileservers (SSIG300 §5–6): boot each from the Config diskette, then from a workstation run `[245]configure`, choose Cluster/Storage, choose initialize or not, assign a node ID (the defaults offered are 2, 3, ...) and, for storage, a cluster node ID (default 1). Afterwards install SSW on the cluster fileservers via install_software. **All package diskettes always go in node 1's drive in 3.00**, and data is copied over the network.
6. Workstations: `attach_net +w [n=40] [c=2]`, then turn the station off and on again (SSIG300 p96).
7. Printers: run `[node] attach_printer` (with a space after the bracket). Printer files live in **`/config/print`** on the cluster fileserver.

**4.x new network** (SIIG401 §3, p23–24):
1. Boot the Network Fileserver from the ICON or Netcard **Fileserver Boot Diskette**.
2. Boot a workstation and log in as root/superuser.
3. Remove the boot diskette, insert the **Configuration Diskette**, and type `configure`.
4. Choose your answers and Perform. Insert the SSW diskettes as asked ("several minutes per diskette").
5. Remove the last diskette and reboot the server from hard disk, then the workstation.
6. Then: cluster fileservers (§5), storage fileservers (§5), workstations (§6: in 4.00 run `/sys/setup` first, then `attach_net [n=] [c=]`), non-system packages via `install_software` (§7), change the root password from "superuser", and printers (Admin Guide Vol 2 §8).

**4.x upgrade** (SIIG401 §4, p25–28): boot everything, then `net` (checks every node is reachable), then `verify_net` (4.00+: checks net.config against the hardware actually present), then `detach_net` the old-format fileservers and note their nodes. Reboot node 1 from the boot diskette, insert the Configuration Diskette, run `configure` and choose whether to initialize. *"The system software will be copied to all fileservers on the network."* Reboot. Reattach old-format servers via §5, which forces initialization.

## 5. Files, directories and commands mentioned

**Files and directories:** `net.config` (on node 1: assigned nodes plus fileserver names; NPG also writes it "netconfig"), the Master Password File (node 1), `/config/pass` (per-cluster password file, built as users are added; the cluster file is checked first, then the master), `/boot` (boot files of all machines in the cluster), `/cmds`, `/util`, `/user_cmds` (in the example path), `/config/print` (printer files), `/sys/setup` (4.00), `/config/make_solo_disk` (cross-reference: SysSW Ops Guide 3.00 p528, a shell file that builds a **boot diskette for the solo, non-networked ICON** in drive 2. Relevant to floppy boot images; another agent's area).

3.00 printer files are **per-node suffixed**. The recovery procedure (SSIG300 p102, 9-4) renames them:
```
$ cd [node_number] 1:/config/print
$ chattr pprt.init.node_number n=pprt.init
$ chattr sprt.init.node_number n=sprt.init
$ chattr snap_p.init.node_number n=snap_p.init
$ chattr snap_s.init.node_number n=snap_s.init
$ chattr tdon_p.init.node_number n=tdon_p.init
$ chattr tdon_s.init.node_number n=tdon_s.init
$ [node_number] detach_printer
$ [node_number] attach_printer
```
(p = parallel, s = serial. "snap" and "tdon" are probably screen-snapshot and Telidon print formats. `chattr ... n=` renames a file.) Built-in printer support in 3.00: **Epson FX-80** and **Panasonic 1091**. 3.00 changed how printer files are named, so printers must be detached before upgrading (SSIG300 p19, p101).

**Commands:** `configure`, `install_software`, `attach_net` (3.00 takes `+w`; 4.x drops it; options `n=`, `c=`), `detach_net`, `verify_net` (new in 4.00), `net`, `query` (prints the total block count), `attach_printer`, `detach_printer`, `chattr`, `new_user`, `delete_user`, `new_password`, `setup` (4.00, Netcard), `files +v`. Default administrator: `root` / password `superuser`.

**Glossary terms:** "printer daemon", "spooler", "REM disk", "Poller program" (3.00, on node 1), "ICON Family Network" (3.00), "ICON Series Network" (4.x).

## 6. Release differences: 3.00 → 4.00 → 4.01

| Item | 3.00 (Jul 1987) | 4.00 (Jan 1989) | 4.01 (Jan 1990) |
|---|---|---|---|
| Manual | *System Software Installation Guide*, 9 sections, screen-by-screen | *Software Installation and Implementation Guide*, 7 sections, GUI described generically | Same as 4.00, lightly edited |
| Boot media | Bootable "Configuration and Installation Diskette" (must not be write-protected) | Separate **ICON Fileserver Boot** and **Netcard Fileserver Boot** diskettes; **Configuration Diskette not bootable** | Same as 4.00 |
| SSW diskettes | 4 (+ a separate On_line Help package) | "Diskettes labelled ICON Series System Software" (count not stated) | Same (the user's 4.10.01 set has 5) |
| How SSW is loaded | `configure`, then a separate `install_software` run that installs SSW onto each cluster fileserver | `configure` installs SSW itself ("system software will be added to the network"); `install_software` is for **non-system** packages only | "copied to all fileservers on the network" |
| Install source | Always node 1's floppy | Any node and drive (source node selectable; default drive 2) | Same |
| Netcard hardware | not mentioned | New: Netcard fileservers and workstations; `setup` utility (`/sys/setup`) | `setup` step removed from the procedures |
| Workstation attach | `attach_net +w [n=] [c=]` | `/sys/setup`, then `attach_net [n=] [c=]` | `attach_net [n=] [c=]`; explicit note that new stations default to node 254 |
| Net tools | — | `verify_net`, `detach_net`, `net` used in upgrade | Same |
| Fileserver name | — | ≤16 characters, "including spaces", no `;` | ≤16 characters, "letters, numbers, and spaces only" |
| Fileserver node range | 2–253 except 245 | 2–244, 246–253 | Same |
| Reboot wait | 30 s | 10 s | 10 s |
| Old-format 10 MB disks | install refused ("Disk Has Old Format") | auto-reformat and initialize, forced | Same |
| Workstation types | ICON, ICON2 ("workstation: either an ICON or ICON2") | ICON, ICON2, Netcard | Same |
| Pre-flight | — | "Disks are formatted" listed as a prerequisite | dropped |

NPG notes that multi-fileserver networks with Network Expanders need "version 3.00 or higher of the operating system" (NPG p13, 1-1). **So clusters, storage fileservers and expanders are 3.00-era features.**

## 7. Corrections and additions to the existing repo docs

1. **Node ID storage (ICON_web_research §1, ICON_boot_and_network):** the manuals show that node numbers are **assigned in software and persist**. Factory defaults are 254 (workstation) and 245 (fileserver); `attach_net` is run *on the new workstation* and the station is then rebooted; `detach_net` is needed before renumbering. This rules out "set in EPROM at order time" [JE2] as the general mechanism and **strongly favours the 32-byte EEPROM/NVRAM**, which also has to hold the **boot/cluster server node**. That fits ROM code **"11 : Missing or failed NVRAM"**.
2. **"LEXICON is node 1, ICONs 16–47, second LEXICON node 2 with ICONs 48–79" [JE2]:** this is a convention, not a rule. Workstations can be any of 16 (or 17) to 253 (except 245), assigned sequentially or by hand. Any fileserver except the Network Fileserver can be 2–253. The 32-per-server limit **is** confirmed as a software limit ("No more than 32 workstations can boot from a single fileserver at a time").
3. **Stations per server** (ICON_web_research §2, "32 vs 16 vs 12–16"): this is settled. **17 nodes per coax segment** (1 server + 16 workstations) is the electrical limit without an expander. **32 workstations per server** is the software limit, reachable only with a Network Expander. **128 nodes** is the network maximum.
4. **Drives (ICON_disk_layout §1):** add **drive 3 = second 32 MB partition of a 64 MB fileserver**. Initialization touches only partition 1 (drive 1). The default search order on 64 MB servers is `1 3`. These manuals say nothing about RAM disk drive 8.
5. **Workstation boot source:** stations boot from their **cluster fileserver** (set by `c=`), not necessarily the LEXICON at node 1. **Fileservers can also network-boot** (storage fileservers always do), so the same ROM supports floppy, hard-disk and network loaders on server hardware too. This addresses open question 3: the role comes from configuration (configure/attach_net) plus whether the local disk holds an OS, **not** from jumpers.
6. **`/boot` (open question 4):** NPG says the cluster fileserver holds *"The boot files of all the machines in the cluster (/boot)"*. In other words /boot holds boot files **for each node** in the cluster, which suggests per-node or per-type boot images (ICON / ICON2 / Netcard / storage fileserver).
7. **The book's 4-diskette `sysback` set vs the release set:** the 3.00 release itself shipped as **4 SSW diskettes plus a config/install diskette**. The match with the book's 4 backup diskettes is probably coincidence; they are different sets.
8. **ICON_boot_and_network "attach / detach in /util":** by 3.00 these are `attach_net` / `detach_net` (plus `attach_printer` / `detach_printer`), and they edit **net.config** on node 1.
9. **Path syntax:** the shell path uses `!` separators (`!/cmds/!/util/!!`). Add this to ICON_commands.
10. **Default root password** is `superuser` (all three installation guides). The admin role is called "Site Administrator".
11. **Fileserver hardware:** fileservers come in 10 MB and 64 MB sizes. Older-model fileservers lack the yellow trouble LED. 3.00 says all fileservers ship identical, as node 245.

## 8. Not found in these four manuals
- On-disk structures (bitmap, directory entries, extents, root block), floppy geometry and interleave, dinit/fdformat/backup formats, executable formats.
- ROM monitor, loader option names, ROM self-test error codes 09–5a, jumpers, EEPROM layout.
- The RAM disk (drive 8), ARCNET protocol details, Netcard hardware specifics.
- 4.10-series releases, the "Upgrade Diskette", and a "Help Diskette" by that name.
