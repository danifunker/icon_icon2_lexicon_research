# Handoff to RustyBackup: QNX 2 permissions and dates when copying (qnx2fs 0.2)

Paste everything below the line into RustyBackup's main session. It follows `rustybackup_handoff_qnx2fs.md` (the move onto the `qnx2fs` crate). Do that one first, or together with this.

---

Goal: when RustyBackup copies, replaces, imports or exports files on QNX 2 volumes (Unisys ICON / LEXICON disks), keep their **permissions, owners and dates** exactly. Then decide, with the user, how **host dates outside 1980–1995** are written.

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

1. **Move to `qnx2fs = "0.2"`** in all four manifests: main, `rb-cli-vintage`, `rb-cli-ppc` and `rb-cli-sol9`. Fix the `NewEntry` literal, and refresh only the `qnx2fs` entry in each lockfile.
2. **QNX 2 → QNX 2: copy the raw attributes.** Pick whichever fits RustyBackup's design:
   - a raw carrier, the way `os_type` / `os_creator` carry Mac type and creator bytes. For example, `CreateFileOptions.native_qnx2: Option<qnx2fs::Attributes>`, filled from the source `FileEntry` when the source is QNX 2 and passed to `NewEntry::attributes` in the adapter;
   - or, when both ends are QNX 2 volumes, hand the whole operation to `Volume::copy_from`.

   Directory dates must be restored after their contents are copied; `copy_from` does this.
3. **Replacing a file:** use `Volume::replace`, or carry the replaced file's `Attributes`. RustyBackup's `put` already "preserves from the replaced file" through mode/uid/gid, which has the same loss as above.
4. **QNX 2 → host** (`get`, `tar`):
   - ICON dates are **local wall-clock time with no time zone**. `decode_date` returns those wall-clock seconds as if they were UTC. Displaying them as UTC shows the right clock time.
   - **Setting a host file's mtime, or a tar header's mtime, should read them in the host's local zone.** Otherwise every file is off by the zone offset; icon-emu was 4 hours early in New York before this was fixed.
   - icon-emu's `icon-cli disk get` does it with `chrono::Local.from_local_datetime`.
5. **Host → QNX 2** (`put`, `untar`, `import`): permissions come from the host mode as now. That's fine; a host `0o755` gives QNX `e`. The **date** is the open question, below.
6. **Tests:** extend the `edit.qnx2.*` regression cases with:
   - a cross-image `cp` that checks `ls -o` attributes and dates are identical on both sides;
   - a `put --replace` that keeps `rwaem`;
   - a directory copy whose date survives.

   The real check is QNX itself in icon-emu:
   - `icon-cli disk IMG check` uses the same crate;
   - for QNX's own verdict, boot the image with `icon-cli serve --hd IMG` and run `icon-ci run 'chkfsys 1'`;
   - **copy a program, then run it on the ICON.**

## The 1980–1995 date problem (needs a decision with the user)

**The format.** An ICON / QNX 2 date is two 16-bit words:
- word 0 has the year minus 1980 in bits 15–12, the month in bits 11–8, the day in bits 5–1 and PM in bit 0;
- word 1 is seconds since 12 o'clock.

The year field is 4 bits, so **only 1980–1995 can be stored**. There's also an unset value: the clock never set gives month and day 0, which QNX lists as `0-----80`.

**Where it bites:**
- **ICON → ICON copies:** no problem. The raw words are copied (`Attributes`).
- **ICON → host:** no problem. Every ICON date fits a host date; only the time zone matters (item 4 above).
- **Host → ICON:** a problem. Any modern file (anything after 1995) can't be dated truthfully. Today `qnx2fs::encode_date` **clamps**, so every file RustyBackup imports from a modern host is dated **31-Dec-95 11:59:59 pm**.
- **The emulator's clock** has the same problem, and it is handled there by moving "now" to **1991**, or **1992** when the host year is a leap year. The same month, day and time are kept, so leap days exist. Files QNX writes in icon-emu, and files its Files panel uploads, are therefore dated "today in 1991".

**Options for host → ICON dates:**

| Policy | 2026-03-14 10:00 becomes | Keeps | Loses | Notes |
|---|---|---|---|---|
| **Clamp** (current) | 1995-12-31 23:59:59 | that it's "modern" | everything else: all imports get one date | Simple, honest about the limit, useless for sorting |
| **Fold** (icon-emu's clock) | 1991-03-14 10:00 | month, day, time; leap days (1992 for leap years) | the year, so files from different years interleave | Matches what QNX itself writes in the emulator |
| **Shift by whole years** so the current year becomes 1995 | 1995-03-14 10:00 (2015 → 1984; before 2011 → clamped to 1980) | order, month, day and time, and relative age within 16 years | absolute years; 29 Feb in a shifted non-leap year has to become 28 Feb or 1 Mar | Best for sorting an imported archive; the result depends on the year you run it in |
| **Same-calendar year** | 1987-03-14 (1987 has 2026's calendar) | weekday and day, so `files` shows sensible dates | the year; no in-range match for leap years starting Mon / Thu / Sat | Cute, rarely what people want |
| **Fixed / "now" on the target's clock** | e.g. the date of the copy | nothing about the source | the source date | What QNX's `copy` from a host would do |
| **Side channel** | real date kept elsewhere | everything | — | Each entry's 4-byte "seconds" field (+19) is 0 on every ICON file. It could hold a full Unix time, but whether any QNX 2 tool reads, keeps or zeroes it is **unknown**. Research it (QNX 2.x later versions, `backup`, `chkfsys`) before relying on it |

**Recommendation** (to confirm with the user):
- **Put the policy in the crate.** Add a `DatePolicy` (`Clamp`, `Fold`, `ShiftToYear(1995)`), plus an `encode_date_with(unix, policy)` in qnx2fs 0.3.0, so RustyBackup and icon-emu share one implementation and one definition. Keep `encode_date` as `Clamp` for compatibility.
- **In RustyBackup**, add a flag such as `--qnx2-dates clamp|fold|shift` for `put` / `untar` / `import`. Default to **`shift`** for archival imports (it keeps order and relative age), and **say in the output** when dates were moved or clamped.
- **Keep icon-emu's emulated clock on `Fold`.** It must match what QNX writes on the emulated machine. Its upload path could take the same flag later.
- Whatever is chosen, **never silently clamp without telling the user.** A log line such as "12 files dated after 1995 were written as …" is enough.

## Where things are

- `~/repos/qnx2fs`: the crate, its CHANGELOG and tests. `copy_from` and `replace` are in `src/lib.rs`; the tests are `copies_between_volumes_keep_permissions_owners_and_dates` and `replace_keeps_permissions_and_owner_with_a_new_date`. Releases go through its pipeline only (bump the version and CHANGELOG, then `gh workflow run publish-crates-io.yml --ref main`).
- `~/repos/icon-emu`:
  - `crates/icon-core/src/files.rs`: `copy`, `write_file` (uses `replace`);
  - `crates/icon-core/src/devices/rtc.rs`: `icon_time`, the Fold rule;
  - `crates/icon-cli/src/disk.rs`: `get` setting host dates in local time.
- `~/repos/icon_icon2_research/ICON_qnx2_filesystem.md`: the format notes, including text encoding.
