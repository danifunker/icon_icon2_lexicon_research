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

So the command set is the same everywhere. The difference is **where a command executes**. Normal commands run on your ICON and read and write files over the network on the LEXICON's disk. Disk-hardware commands have to run on the LEXICON, so you send them there with `[1]`. *(Reading `[1]` as "run on node 1" is my inference. It fits the book's description of the prefix as "the node number" and the older `rmtshell` name.)*

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
