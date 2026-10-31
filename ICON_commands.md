# ICON / QNX Command Reference

Compiled from *The Book of ICON* (John Herriott, Wiley, 1985), which describes QNX ~1.13 on the CEMCORP ICON / LEXICON network. Page links jump to the matching spot in [book_of_icon.md](book_of_icon.md); **PDF page − 20 = printed page**.

> **Reading the OCR'd book:** the scan often turns `>` into `)`, `>>` into `))`, `^` into `A`, `|` into `'` or `l`, and `ls` into `Is`. The commands below are written the way they should actually be typed.

## Where commands run: ICON vs. LEXICON

The book never describes a keyboard or screen on the LEXICON. **Every command is typed at an ICON workstation's shell prompt** (`%` for users, `$` for the site administrator). The two machines split the work like this:

| | ICON (workstation, diskless when networked) | LEXICON (fileserver, "node 1") |
|---|---|---|
| Role | Runs the programs: shell, editors, BASIC/Pascal/Logo interpreters | Holds the 10 MB hard disk (`1:`) and 640 KB floppy (`2:`), spools printing, owns the printer ports |
| How programs get there | Commands live in `/cmds` and `/util` **on the LEXICON's disk** and are loaded over the network. Loading BASIC makes the fileserver's light flash while it "loads into your ICON" ([p53](book_of_icon.md#p53)); the glossary calls this *downloading* ([p156](book_of_icon.md#p156)) | — |
| Commands aimed at it | — | Commands that touch the server's own drives are prefixed with the **node number `[1]`**, e.g. `[1] fdformat 2`, `[1] dinit 2` ([p43](book_of_icon.md#p43)). An earlier release used `rmtshell fdformat`, i.e. a *remote shell* ([p17](book_of_icon.md#p17)) |

So the command set is the same everywhere. The difference is **where a command executes**. Normal commands run on your ICON and read and write files over the network on the LEXICON's disk. Disk-hardware commands have to run on the LEXICON, so you send them there with `[1]`. *(Web sources confirm that QNX paths take the form `[node]drive:/path`, with the LEXICON as node 1. More commands that the book doesn't cover, such as `help`, `task`, `apb`, `say`, `new_password` and `pwd`, are listed in [ICON_web_research.md §3](ICON_web_research.md#3-qnx-on-the-icon-additions-to-icon_commandsmd).)*

The book also warns that command names changed between releases. If something fails, look in the file `user_.cmds` for the current name ([p17](book_of_icon.md#p17)).

---

## 1. Session and keyboard

| Command / key | What it does | Page |
|---|---|---|
| `Login:` → name, then password | Log in; `icon` is the shared general-user login | [28](book_of_icon.md#p28) |
| `CTRL-d` | Log out; also ends keyboard input to `copy`, `cat`, `echo` | [29](book_of_icon.md#p29), [104](book_of_icon.md#p104) |
| `CTRL-c` | Stop the running program | [29](book_of_icon.md#p29) |
| `CTRL-x` | Erase the whole command line before pressing ENTER | [104](book_of_icon.md#p104) |
| `CTRL-l` | Clear the screen | [104](book_of_icon.md#p104) |
| `CTRL--` (hyphen) | Bring back a cursor that has disappeared | [104](book_of_icon.md#p104) |
| `PAUSE` key | Freeze scrolling output | [25](book_of_icon.md#p25) |

## 2. Moving around the file tree

| Command | What it does | Page |
|---|---|---|
| `cd /path` | Change to an absolute path (`cd /demo`, `cd /config`) | [28](book_of_icon.md#p28) |
| `cd name` | Change into a subdirectory of the current one | [48](book_of_icon.md#p48) |
| `cd` | Go to your home directory (`/user/<login>`) | [58](book_of_icon.md#p58) |
| `cd ^` / `cd ^^` | Up one / two levels | [58](book_of_icon.md#p58), [155](book_of_icon.md#p155) |
| `cd /` | Go to the root | [61](book_of_icon.md#p61) |
| `wmi` | "Where am I?": print the current path | [48](book_of_icon.md#p48) |
| `vcd` | *Visual change directory*: trackball-driven tree browser that shows the path name | [59](book_of_icon.md#p59) |
| `ui` | *User interface*: icon-based file manager (doors = directories, notebooks = files, briefcase = move files) | [63](book_of_icon.md#p63) |

## 3. Listing files and disks

| Command | What it does | Page |
|---|---|---|
| `ls` | "List sorted": names only; directories have a `+` in front | [51](book_of_icon.md#p51) |
| `files` | List directory contents | [155](book_of_icon.md#p155) |
| `files +d` | …with dates | [155](book_of_icon.md#p155) |
| `files +v` | Verbose: owner, blocks, links, disk location, group, attributes, date, time | [38](book_of_icon.md#p38) |
| `ls 2:/`, `files 2:/`, `files +v 2:/` | Same, for the floppy in drive 2 | [102](book_of_icon.md#p102) |
| `query 2:/` | Free blocks on a disk | [44](book_of_icon.md#p44) |
| `size file` | Character and line count | [103](book_of_icon.md#p103) |

## 4. Creating, viewing and removing files

| Command | What it does | Page |
|---|---|---|
| `mkdir name` | Make a directory | [48](book_of_icon.md#p48) |
| `drel name` | Delete ("release") an **empty** directory | [52](book_of_icon.md#p52) |
| `frel file` | Delete ("release") a file | [51](book_of_icon.md#p51) |
| `p file` | Print the file to the screen | [51](book_of_icon.md#p51) |
| `ed file` | Line editor. `i` = insert, `ESC q` = leave insert mode, `w` = write, `q` = quit, `z` = jump to end | [49](book_of_icon.md#p49) |
| `edit file` | Full-screen ICON text editor (icons, trackball) | [69](book_of_icon.md#p69) |
| `copy >file` | Type text straight into a new file (end with `CTRL-d`) | [55](book_of_icon.md#p55) |
| `copy >>file` | Append keyboard input to a file | [56](book_of_icon.md#p56) |
| `copy src dest` | Copy one file to another | [155](book_of_icon.md#p155) |
| `cat file …` | Concatenate / display files (also used to show graphics files such as `owl.t`) | [62](book_of_icon.md#p62) |
| `sort file` | Sort lines alphabetically; `sort a >b` saves the result | [56](book_of_icon.md#p56) |
| `locate word file` | Find every line containing a word | [102](book_of_icon.md#p102) |
| `diff file1 file2` | Show lines that differ | [103](book_of_icon.md#p103) |
| `echo` | Echo keyboard lines back (end with `CTRL-d`) | [62](book_of_icon.md#p62) |
| `date` | Show date and time | [155](book_of_icon.md#p155) |

### Permissions

| Command | What it does | Page |
|---|---|---|
| `chattr file po=-r` | Change **owner** permissions (`r` read, `w` write, `a` append, `m` modify, `e` execute) | [40](book_of_icon.md#p40) |
| `chattr file pg=+wa` | Change **group / other** permissions | [40](book_of_icon.md#p40) |
| `chattr file a=+e` | Make a file executable (needed for command files) | [42](book_of_icon.md#p42) |
| `chattr /user/name o=N` | Set the owner number (admin) | [35](book_of_icon.md#p35) |

## 5. Redirection, pipes and tasks

| Syntax | Meaning | Page |
|---|---|---|
| `cmd >file` | Send output to a file | [56](book_of_icon.md#p56) |
| `cmd >>file` | Append output to a file | [56](book_of_icon.md#p56) |
| `cmd1 \| cmd2` | Pipe, e.g. `copy \| sort >cities` | [57](book_of_icon.md#p57) |
| `cmd &` | Run in the background; prints a task ID | [104](book_of_icon.md#p104) |
| `tasks` | List running tasks | [156](book_of_icon.md#p156) |
| `kill <taskID>` | Stop a task | [104](book_of_icon.md#p104) |

## 6. Command files (shell scripts)

Write commands into a file with `edit`, then `chattr file a=+e` and run it by typing its name ([p98](book_of_icon.md#p98)). Useful building blocks:

| Command | What it does | Page |
|---|---|---|
| `type text…` | Print a line of text (like BASIC `print`) | [42](book_of_icon.md#p42) |
| `clearscreen` | Clear the screen | [42](book_of_icon.md#p42) |
| `wait_for_char` | Wait for a keypress (like BASIC `GET`/`INKEY$`) | [99](book_of_icon.md#p99) |

## 7. Screen

| Command | What it does | Page |
|---|---|---|
| `lo_res` | 40-column text mode (used by Logo) | [103](book_of_icon.md#p103) |
| `hi_res` | Back to the normal 80-column mode | [103](book_of_icon.md#p103) |

## 8. Printing

Printers normally hang off the **LEXICON**, and jobs are spooled there (`/spool/parallel`, `/spool/serial`) so every ICON can print ([p22](book_of_icon.md#p22), [p62](book_of_icon.md#p62)).

| Command | What it does | Page |
|---|---|---|
| `print file` / `sprint file` | Print a text file to the parallel / serial printer | [100](book_of_icon.md#p100) |
| `list file` / `slist file` | Print a program listing to the parallel / serial printer | [100](book_of_icon.md#p100) |

**How it works (traced in icon-emu, 2026; details in icon-emu `docs/printer.md`):**
- **Hardware:** the parallel data is PPI B port B in 8255 mode 1, STROBE is PPI A PC2, and BUSY is PPI B PC6. ACK raises INTR_B (PPI B PC0), which comes in on INT2 and is handled by INT 61h.
- **Setup:** `attach_printer n=1 d=3 t=fx80` (device 3 = `$plpr`). It needs the fileserver in the hardware database; if it says "Node 1 not found in network", run `verify_net` first.
- **What `print` sends an FX-80:**
  - it copies the ROM font to RAM (`ESC :`) and selects it (`ESC %`);
  - it downloads 13 characters (`ESC &`): lowered capitals A E I O U, circumflex, cedilla and diaeresis pieces, and « » ÷ × ß;
  - it prints accents by overstriking, accent + BS + letter.
- **Screen dumps:** `/sys/print_screen` sends `ESC * 4` bit images at 80 dpi, sideways.

`print` options (put them before the file name; `-` turns an option off, `+` turns it on) ([p100–102](book_of_icon.md#p100)):

| Option | Effect |
|---|---|
| `-h` / `+h` | Header (date, time, file name, page number) off / on |
| `-r` | Don't restart page numbering at 1 |
| `-p` / `+p` | Pagination (and header) off / on |
| `+d` | Double-strike |
| `+e` | Emphasized |
| `+u` | Print your user number on each page |
| `+b` | Banner page |
| `s=N` | Line spacing (lines per inch; 6 = normal, 12 = double) |
| `w=N` | Characters per line (default 80) |
| `o=N` | Left offset |

## 9. Languages and graphics tools

| Command | What it does | Page |
|---|---|---|
| `basic` | Load Waterloo/WATCOM BASIC into the ICON. Inside it: `run`/`r`, `list`/`l`, `save name`, `old name`, `bye` | [52](book_of_icon.md#p52), [109](book_of_icon.md#p109) |
| `pascal` | WATCOM Pascal | [124](book_of_icon.md#p124) |
| `logo` / `logo.français` | Logo in English / French | [129](book_of_icon.md#p129) |
| `natal` | NATAL authoring language | [156](book_of_icon.md#p156) |
| `ied` | Icon editor | [85](book_of_icon.md#p85) |
| `fged` | Graphics editor | [89](book_of_icon.md#p89) |
| `font_ed` / `fed` | Font editor | [94](book_of_icon.md#p94) |

## 10. Demo programs (`cd /demo`)

| Command | What it does | Page |
|---|---|---|
| `sayer` | Speech demo: speaks the keys you type | [28](book_of_icon.md#p28) |
| `line.bw`, `line.pack`, `line.color` | Line-graphics demos | [29](book_of_icon.md#p29) |
| `hanoi` | Towers of Hanoi | [29](book_of_icon.md#p29) |

---

## 11. Site administrator / LEXICON-side commands

Log in as the administrator (prompt `$`). Owner number 255 is reserved for the administrator, which leaves 254 user numbers ([p38](book_of_icon.md#p38)).

### Users

| Command | What it does | Page |
|---|---|---|
| `cd /config` then `ed pass` | Edit the password file by hand. Each entry: login, password, user number, home dir, optional start-up language | [35](book_of_icon.md#p35) |
| `new_user` | Prompted add-user (assigns the number and makes `/user/<name>`) | [36](book_of_icon.md#p36) |
| `delete_user` | Remove a user (run `drel /user/<name>` first) | [36](book_of_icon.md#p36) |
| `p pass` | Show the password file | [36](book_of_icon.md#p36) |

### Disks and backup: these act on the LEXICON's drives

| Command | What it does | Page |
|---|---|---|
| `[1] fdformat 2` | Format the floppy in the **LEXICON's** drive 2 (run from `/util`, and make sure no other station is busy) | [43](book_of_icon.md#p43) |
| `[1] dinit 2` | Initialize the floppy's file system (creates `2:/bitmap`). **Leaving out the `2` targets the hard disk** | [43](book_of_icon.md#p43) |
| `dcheck` | Check disk integrity | [155](book_of_icon.md#p155) |
| `backup src dest [p=file] +a +d +o` | Copy trees between `1:` (hard disk) and `2:` (floppy). `+d` keeps dates, `+o` keeps owner numbers, `+a` keeps all dates, `p=` picks one file | [44](book_of_icon.md#p44) |
| `sysback` | Command file the book has you write in `/util` to back up all system directories onto 4 diskettes (see [the disk layout notes](ICON_disk_layout.md)) | [40](book_of_icon.md#p40) |
| `park` / `rmt_park` | Park the hard disk / floppy heads before moving the LEXICON | [62](book_of_icon.md#p62) |
| `attach` / `detach` (in `/util`) | Add or remove ICON stations when setting up the network | [62](book_of_icon.md#p62) |

Older names for the floppy format command, all obsolete by the time the book came out: `wdflopform` → `rmtshell fdformat` → `fdformat 2` ([p17](book_of_icon.md#p17)).

---

## 12. ISSSW 3.00 command set (official Unisys manuals, 1987)

The book documents QNX 1.13. By ISSSW 3.00, which runs on QNX 2.x, several commands had been renamed or changed. The source is the *System Software Operations Guide* (SSOG) §9; full notes are in [manual_notes/operations.md](manual_notes/operations.md).

| Book (1.13) | ISSSW 3.00 | Note |
|---|---|---|
| `tasks` | `task` | |
| `wmi` | `pwd` | |
| `ui` | `house` | `vcd` no longer listed |
| `attach` / `detach` | `attach_net` / `detach_net` (+ `attach_printer` / `detach_printer`) | These write the node ID into **NVRAM** and edit `/config/net.config` |
| `ed` | `led` (line editor); `edit` (screen editor) | |
| `dcheck` "check disk" | `dcheck` = **bad-block scan**; `chkfsys` = filesystem check and bitmap rebuild | |
| `chattr … o=N` | `chattr … g=G m=M` | Owners are **group.member**; 255.255 = Site Administrator, g.255 = Classroom Administrator |
| `files +d` "with dates" | `files +d` = list **directories** only | |
| `rmt_park` | `[1] park` (10 MB disks only) | |
| `sysback`, `rmtshell`, `fged`, `clearscreen`, `natal` | not in the 3.00 list | |

Prompts: `$` Site Administrator, `#` Classroom Administrator, `%` user. The default administrator login is `root` with password `superuser`. Text files end lines with **RS (0x1E)**.

### Full utility inventory

**New?** = not covered elsewhere in this file (✱ = only in ICON_web_research §3). **Adm** ‡ = Site Administrator only. Pages are printed / PDF page in SSOG.

| Command | Purpose | Printed / PDF | Adm | New? |
|---|---|---|---|---|
| alive | Tell poller or node which nodes are up, down or busy; `+n` at boot | 9-13 / p121 | ‡ | new |
| apb | All-points bulletin to every screen (max 63 chars) | 9-19 / p127 | ‡ | ✱ |
| apl | WATCOM APL interpreter (opt) | 9-21 / p129 | | new |
| arch | Multi-diskette append-only archive: create, directory, save, restore | 9-23 / p131 | | new |
| attach_net | Assign a node number in NVRAM; +workstation/+cluster/+storage, n=, c= | 9-29 / p137 | ‡ | new (replaces `attach`) |
| attach_printer | Create /config/print init files, spool directory, daemon | 9-31 / p139 | ‡ | new |
| back | Toggle display of background task ID | 5-14 / p78 | Sh | new |
| backup | Copy trees by date or pattern (+a +d +o, d= t= l= p= pd= r=) | 9-33 / p141 | | have |
| base | Shell variable display base 16/10 | 5-14 / p78 | Sh | new |
| basic | WATCOM BASIC | 9-39 / p147 | | have |
| bbt | Show or add entries in the hard-disk Bad Block Table | 9-41 / p149 | ‡ | new |
| break | Send a break to a task | 9-43 / p151 | Sh | new |
| cancel_macro | Stop a NAPLPS macro | 9-45 / p153 | | new |
| cat | Concatenate files | 9-47 / p155 | | have |
| cc | Compile and link C (opt) | 9-49 / p157 | | new |
| cco | Compile C without linking (opt) | 9-55 / p163 | | new |
| cd | Change directory (`^` = up) | 9-57 / p165 | Sh | have |
| chattr | Change status (b/m/u), attributes, permissions, owner (g= m=), name (n=), date | 9-59 / p167 | ‡ | have (fix o=) |
| chkfsys | Filesystem consistency check; rebuild bitmap | 9-63 / p171 | ‡ | new |
| cobol | WATCOM COBOL (opt) | 9-67 / p175 | | new |
| config77 | WATFOR-77 configuration (opt) | 9-69 / p177 | | new |
| convert_phrase | Old TMS5220 "codepack" → new ICON speech format | 9-71 / p179 | | new |
| copy | Copy files or devices | 9-73 / p181 | | have |
| date | Show or set date and time | 9-75 / p183 | | have |
| dcheck | Read-test every block for bad blocks; +m marks them in the bitmap | 9-77 / p185 | | have (purpose corrected) |
| dcopy | Sector copy of a whole disk (so=/do= offsets, +hard) | 9-81 / p189 | | new |
| decode_phrase | TMS5220 data → ASCII frame parameters | 9-85 / p193 | | new |
| defpipe | Set the pipe temp-file path | 5-15 / p79 | Sh | new |
| def_server | Set the global nameserver (node, task ID) | 9-87 / p195 | | new |
| delete_user | Remove users from /config/pass (Network and cluster FS) | 9-89 / p197 | ‡ | have |
| detach_net | Remove a node; +reset → ws 254 / fs 245 | 9-91 / p199 | ‡ | new (replaces `detach`) |
| detach_printer | Remove printer files, spool dir and daemon | 9-93 / p201 | ‡ | new |
| diff | Compare files | 9-95 / p203 | | have |
| dinit | Initialize a QNX filesystem (+hard, b=, l=loader, k=, p=/prefix, +suppress) | 9-99 / p207 | ‡ | have |
| drel | Remove an empty directory | 9-103 / p211 | | have |
| dump | Hex + ASCII dump, 16 bytes per line | 9-105 / p213 | | new |
| ec | Shell takes input from a file (like `source`) | 5-15 / p79 | Sh | new |
| echo | Echo arguments | 9-107 / p215 | | have |
| edit | Icon-driven full-screen Editor | 9-109 / p217 | | have |
| else / endif / if / then / goto | Shell flow control (`if +f/+d/+m/+a node`, lt/ge/eq/ne) | 5-15..5-20 / p79–84 | Sh | new |
| encode_phrase | ASCII frame parameters → TMS5220 data | 9-111 / p219 | | new |
| eo | Execute a command on a list of files (`@` substitution) | 9-113 / p221 | | new |
| exit | Exit the shell with a status (hex) | 5-16 / p80 | Sh | new |
| fdformat | Format a floppy (h= n= t=, -stagger, +Patch block 1) | 9-117 / p225 | ‡ | have |
| files | Recursive listing; +v shows Blk/X/Loc/Grp/Mem/Attr/Perm; -used shows erased entries | 9-121 / p229 | | have |
| font_ed | Font Editor | 9-127 / p235 | | have |
| frel | Delete files | 9-129 / p237 | | have |
| gks_start / gks_stop | Start or stop the WATCOM GKS graphics task | 9-131/9-133 / p239/241 | | new |
| help | Online help, same text as the manual (opt) | 9-135 / p243 | | ✱ |
| hi_res | gtask 80-column mode | 9-137 / p245 | | have |
| house | Icon-driven House shell | 9-139 / p247 | | new (book's `ui`) |
| icon_emul_start / icon_emul_stop | ICON emulator on ICON2 | 9-141/9-143 / p249/251 | | new |
| ied | Icon editor | 9-145 / p253 | | have |
| kill | Kill a task | 9-147 / p255 | Sh | have |
| kill_list / kill_print / kill_slist / kill_sprint | Kill the current print job (ws parallel / fs parallel / ws serial / fs serial) | 9-149..9-155 / p257–263 | ‡ | new |
| kill_vcs | Kill virtual circuits to a crashed node | 9-157 / p265 | | new |
| led | Line editor | 9-159 / p267 | | new (replaces `ed`?) |
| link / linkobj | Link objects (with C libraries) (opt) | 9-169/9-173 / p277/281 | | new |
| list | Print on a workstation-attached parallel printer | 9-177 / p285 | | have |
| locate | Find patterns in files | 9-183 / p291 | | have |
| login | New login (+stack); searches cluster then network pass | 9-187 / p295 | | have |
| logo | Logo (opt) | 9-189 / p297 | | have |
| logoff | Log off (-z) | 9-191 / p299 | | have (as CTRL-d) |
| lo_res | gtask 40-column mode | 9-193 / p301 | | have |
| ls | List names (directories marked +) | 9-195 / p303 | | have |
| make_solo_disk | /config script: build a solo-ICON boot floppy (80-track) | 9-197 / p305 | ‡ | new |
| mkdir | Make a directory [initial entries, default 10] | 9-199 / p307 | | have |
| mount | Mount disk/ramdisk/remdisk/lib; list mounts | 9-201 / p309 | ‡ | new |
| msort | Merge and sort files | 9-207 / p315 | | new |
| nacc | Network access control for drives, devices, CPU | 9-211 / p319 | ‡ | new |
| naplps_start / naplps_stop | NAPLPS decoder on $tel | 9-213/9-215 / p321/323 | | new |
| net | Per-node resources (memory, tasks, VCs, QNX version) | 9-217 / p325 | | new |
| new_password | Change password (Network + cluster pass) | 9-219 / p327 | | ✱ |
| new_user | Add users (c= cluster, f= batch, g=, l= entry program, u= base dir) | 9-221 / p329 | ‡ | have |
| nv_config | Show NVRAM configuration (hardware, serial number, boot info) | 9-225 / p333 | | new |
| ontty | Create a task on another tty | 5-18 / p82 | Sh | new |
| p | Display a file (+raw) | 9-227 / p335 | | have |
| park | Park 10 MB disk heads and hang (run as `[1] park`) | 9-229 / p337 | ‡ | have |
| pascal | WATCOM Pascal (opt) | 9-231 / p339 | | have |
| path | Command search path (`!dir1!dir2!!`) | 9-233 / p341 | Sh | new |
| pause | Wait for Enter | 5-19 / p83 | Sh | new |
| pri | Priority for later commands | 9-235 / p343 | Sh | new |
| print | Spool to the fileserver parallel printer | 9-237 / p345 | | have |
| prompt | Show tty number in the prompt | 5-19 / p83 | Sh | new |
| pwd | Print current [node]drive:/path | 9-243 / p351 | | ✱ (book's `wmi`) |
| qdos | MS-DOS emulator (a=–d= drives, m= memory, default 128K) (opt) | 9-245 / p353 | | new |
| qnx_tty_start / qnx_tty_stop | QNX vs ICON keyboard mode | 9-247/9-249 / p355/357 | | new |
| query | Free space from the bitmap (+d graphic map) | 9-251 / p359 | | have |
| say | Speak TMS5220 files | 9-253 / p361 | | ✱ |
| search | Local/remote drive search order | 9-255 / p363 | | new |
| set | WATFOR-77 environment task (opt) | 9-257 / p365 | | new |
| set_boot_type | Workstation boot file with or without full FSA (±fsys) | 9-259 / p367 | ‡ | new |
| set_colour | Screen colours (16-colour palette) | 9-261 / p369 | | new |
| sh | Execute shell commands / new shell (`[1] sh`) | 9-263 / p371 | | new |
| shift | Shift arguments | 5-19 / p83 | Sh | new |
| silence | Reset the speech chip | 9-267 / p375 | | new |
| size | Character count | 9-269 / p377 | | have |
| slice | Timeslice in 1/20-s ticks (1–255) | 9-271 / p379 | | new |
| slist | Print on a workstation-attached serial printer | 9-273 / p381 | | have |
| sort | Sort | 9-279 / p387 | | have |
| sound_start / sound_stop | ICON2 Sound Administrator | 9-283/9-285 / p391/393 | | new |
| speech_start / speech_stop | ICON2 LPC Administrator | 9-287/9-289 / p395/397 | | new |
| split | Split a file | 9-291 / p399 | | new |
| spool | WATCOM print spooler (opt) | 9-293 / p401 | | new |
| sprint | Spool to the fileserver serial printer | 9-295 / p403 | | have |
| stop_data_mgr / stop_mmgr / stop_win_mgr | Stop stuck data manager / memory manager / window manager | 9-301..9-305 / p409–413 | | new |
| stty | Terminal and serial options (baud, bits, parity, edit, ...) | 9-307 / p415 | | new |
| stype | Type without newline | 9-313 / p421 | Sh | new |
| talk | Terminal emulator with block-send file transfer over $mdm | 9-315 / p423 | | new |
| task | List tasks (+i +m +c +s +v +net +registered, n=node) | 9-325 / p433 | | ✱ (book's `tasks`) |
| trap | Trap keyboard breaks | 5-20 / p84 | Sh | new |
| type | Print arguments | 9-329 / p437 | Sh | have |
| verbose | Shell verbose mode | 5-21 / p85 | Sh | new |
| watfor77 | WATFOR-77 compiler (opt) | 9-331 / p439 | | new |
| wc | Count characters, words, lines | 9-333 / p441 | | new |
| wedit | WATCOM full-screen editor | 9-335 / p443 | | new |
| who | Who is on this workstation / node / net (`who net`) | 9-337 / p445 | | new |
| ws | Walk a tree and run a command per file (date/time filters) | 9-339 / p447 | | new |
| xlat | Translate characters | 9-345 / p453 | | new |
| zap | Remove a busy or damaged file's directory entry without freeing its blocks | 9-347 / p455 | ‡ | new |
