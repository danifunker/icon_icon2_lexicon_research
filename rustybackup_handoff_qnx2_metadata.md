# Handoff to RustyBackup: QNX 2 permissions and dates when copying (qnx2fs 0.2)

Paste everything below the line into RustyBackup's main session. It follows `rustybackup_handoff_qnx2fs.md` (the move onto the `qnx2fs` crate). Do that one first, or together with this.

---

Goal: when RustyBackup copies, replaces, imports or exports files on QNX 2 volumes (Unisys ICON / LEXICON disks), keep their **permissions, owners and dates** exactly, and write **host dates outside 1980–1995** with the agreed 1994 rule (below).

## Background

- **`qnx2fs` 0.2.0 is on crates.io** (<https://crates.io/crates/qnx2fs>, MIT, same author). It adds:
  - `Attributes`: an entry's QNX metadata exactly as stored. That is owner attributes (`rwaem` / `rcbm`), group and other permissions, owner (`member`, `group`), and the raw two-word date. Read it with `Entry::attributes()`.
  - `NewEntry::attributes` and `NewEntry::like(&entry)`: create a file or directory with exactly that metadata. `mtime`, if set as well, replaces the date.
  - `Volume::copy_from(&mut src, src_path, dst_dir, replace) -> CopyReport`: copies a file or folder tree from another volume, keeping every entry's metadata.
    - Directories get their own dates back after being filled.
    - Existing directories are merged into; existing files need `replace`.
    - `/` copies a disk's contents without its `bitmap`.
  - `Volume::replace(path, data, opts)`: overwrites a file, keeping its permissions and owner, with a new date. It checks the space first, so a full disk keeps the old file.
  - `Volume::set_attributes(&entry, attributes)`.
  - **Breaking:** `NewEntry` has a new public field. The adapter's `NewEntry { mode, uid, gid, mtime }` literal in `src/fs/qnx2.rs` (branch `qnx2fs-crate`) needs `..Default::default()`.
- **icon-emu already uses all of this.** Its Files panel, `icon-ci cp` and `icon-cli disk … cp` copy between disks this way.
  - Logo copied from the YorkU Logo diskette image onto a hard disk runs on the emulated ICON with no `chattr`.
  - The copies keep their 1985 and 1987 dates.
  - QNX's own `chkfsys` is clean afterwards.

## What is wrong in RustyBackup today

RustyBackup carries metadata through `CreateFileOptions { mode, uid, gid, unix_times, … }`, which suits Unix-like filesystems. For QNX 2 this is lossy:
- **Permissions:** QNX has separate `w` (write) and `a` (append) bits, which both map to Unix `w`. Writing back sets both.
- **The `m` bit:** owner `m` (may change attributes) has no Unix equivalent; the adapter always sets it.
- **Directory bits:** `c` and `b` have no Unix meaning.
- **Group and other permissions** go through the same lossy mapping.
- **The date** round-trips exactly only while it is a valid 1980–1995 ICON date (see below).

So `rb-cli cp` from one ICON disk image to another, `put --replace` over a QNX file, and `tar` → `untar` round trips can all change a file's permissions. They also change folder dates, because adding entries re-dates a folder and nothing restores the date.

## What to do

1. **Move to `qnx2fs = "0.3"`** (0.2.0 if 0.3.0 isn't published yet) in all four manifests: main, `rb-cli-vintage`, `rb-cli-ppc` and `rb-cli-sol9`. Fix the `NewEntry` literal, and refresh only the `qnx2fs` entry in each lockfile.
2. **QNX 2 → QNX 2: copy the raw attributes.** Pick whichever fits RustyBackup's design:
   - a raw carrier, the way `os_type` / `os_creator` carry Mac type and creator bytes. For example, `CreateFileOptions.native_qnx2: Option<qnx2fs::Attributes>`, filled from the source `FileEntry` when the source is QNX 2 and passed to `NewEntry::attributes` in the adapter;
   - or, when both ends are QNX 2 volumes, hand the whole operation to `Volume::copy_from`.

   Directory dates must be restored after their contents are copied; `copy_from` does this.
3. **Replacing a file:** use `Volume::replace`, or carry the replaced file's `Attributes`. RustyBackup's `put` already "preserves from the replaced file" through mode/uid/gid, which has the same loss as above.
4. **QNX 2 → host** (`get`, `tar`):
   - ICON dates are **local wall-clock time with no time zone**. `decode_date` returns those wall-clock seconds as if they were UTC. Displaying them as UTC shows the right clock time.
   - **Setting a host file's mtime, or a tar header's mtime, should read them in the host's local zone.** Otherwise every file is off by the zone offset; icon-emu was 4 hours early in New York before this was fixed.
   - icon-emu's `icon-cli disk get` does it with `chrono::Local.from_local_datetime`.
5. **Host → QNX 2** (`put`, `untar`, `import`): permissions come from the host mode as now. That's fine; a host `0o755` gives QNX `e`. Dates follow the 1994 rule below.
6. **Tests:** extend the `edit.qnx2.*` regression cases with:
   - a cross-image `cp` that checks `ls -o` attributes and dates are identical on both sides;
   - a `put --replace` that keeps `rwaem`;
   - a directory copy whose date survives.

   The real check is QNX itself in icon-emu:
   - `icon-cli disk IMG check` uses the same crate;
   - for QNX's own verdict, boot the image with `icon-cli serve --hd IMG` and run `icon-ci run 'chkfsys 1'`;
   - **copy a program, then run it on the ICON.**

## Dates: decided, the 1994 fold

**The format.** An ICON / QNX 2 date is two 16-bit words:
- word 0 has the year minus 1980 in bits 15–12, the month in bits 11–8, the day in bits 5–1 and PM in bit 0;
- word 1 is seconds since 12 o'clock.

It is local time with no zone, and **only 1980–1995 can be stored**.

**The rule (decided with the user):** a date outside that range is written as **the same month, day and time of day, in 1994**. **29 February goes to 1992**, the last leap year the ICON can store. Dates already in 1980–1995 are kept.
- **1994 sorts after all the ICON system software**; the last release is 4.10.05, November 1991.
- **It survives New Year's Eve:** a clock running past 31-Dec-1994 reaches 1995, which still encodes. With 1995 it would roll into 1996 and wrap round to 1980.
- **It's in the crate:** `qnx2fs::fold_date`, applied automatically from **qnx2fs 0.3.0** to "now" and to `NewEntry::mtime`. Copied `Attributes` keep their stored date exactly. The crate README explains it.
- **icon-emu's emulated clock uses the same function**, so the login screen shows, for example, `October 29 1994`, and files QNX writes in the emulator match RustyBackup's.

**What RustyBackup needs to do:**
- **Depend on `qnx2fs = "0.3"`** once it is published; 0.3.0 also contains 0.2's `copy_from`, `replace` and `Attributes`. Host files passed with an mtime are then folded instead of all becoming 31-Dec-95 11:59:59 pm.
- **Say so in the output** when a `put` / `untar` / `import` writes dates outside 1980–1995. For example: "QNX 2 stores years 1980–1995: 12 files were dated today's day in 1994".
- **Document it** where RustyBackup describes QNX 2 support. Tell users to check the date an ICON shows at start-up: the login screen shows it, and it is what everything written from then on is dated.
- **For ICON → host** (item 4 above), dates are exact. Only the time zone needs handling.

## Where things are

- `~/repos/qnx2fs`: the crate, its CHANGELOG and tests. `copy_from` and `replace` are in `src/lib.rs`; the tests are `copies_between_volumes_keep_permissions_owners_and_dates` and `replace_keeps_permissions_and_owner_with_a_new_date`. Releases go through its pipeline only (bump the version and CHANGELOG, then `gh workflow run publish-crates-io.yml --ref main`).
- `~/repos/icon-emu`:
  - `crates/icon-core/src/files.rs`: `copy`, `write_file` (uses `replace`);
  - `crates/icon-core/src/devices/rtc.rs`: `icon_time`, the Fold rule;
  - `crates/icon-cli/src/disk.rs`: `get` setting host dates in local time.
- `~/repos/icon_icon2_research/ICON_qnx2_filesystem.md`: the format notes, including text encoding.
