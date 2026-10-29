# Handoff to RustyBackup: QNX 2 dates with qnx2fs 0.3

Paste everything below the line into RustyBackup's main session. It is the date part of `rustybackup_handoff_qnx2_metadata.md`, updated for the released crate. Do it with, or after, the move onto the crate (`rustybackup_handoff_qnx2fs.md`).

---

Goal: RustyBackup writes and reads QNX 2 dates (Unisys ICON / LEXICON disks) the way `qnx2fs` 0.3 and icon-emu do. Modern dates are folded into the ICON's range; the ICON's local wall-clock time is converted correctly to and from host time; users are told what happened.

## The format and the rule

- **The format.** An ICON / QNX 2 date is two 16-bit words:
  - word 0 has the year minus 1980 in bits 15–12, the month in bits 11–8, the day in bits 5–1 and PM in bit 0;
  - word 1 is seconds since 12 o'clock.

  It is the **local wall-clock time** of the machine that wrote it, with no time zone. **Only 1980–1995 can be stored.** An unset clock gives month and day 0, which QNX lists as `0-----80`; `decode_date` reads that as 1 January 1980.
- **The rule (decided with the user, now in the crate):** a date outside 1980–1995 is written as **the same month, day and time of day, in 1994**. **29 February goes to 1992**, the last leap year the ICON can store. Dates already in range are kept.
  - **1994 sorts after all the ICON system software**; the last release is 4.10.05, November 1991.
  - **It survives New Year's Eve:** a clock running past 31-Dec-1994 reaches 1995, which still encodes. With 1995 it would roll into 1996 and wrap round to 1980.
- **`qnx2fs` 0.3.0** (crates.io; the README has a Dates section):
  - **`fold_date(unix)`** implements the rule.
  - **Folding is automatic** for "now" (`Volume::set_clock`, or the system clock) and for `NewEntry::mtime`. A host file passed with its mtime is folded, not clamped to 31-Dec-95 11:59:59 pm as before 0.3.
  - **`encode_date` on its own still clamps**, for compatibility; `encode_date(fold_date(t))` folds.
  - **Copies keep the stored date words exactly:** `Attributes`, `NewEntry::like`, `Volume::copy_from`.
- **icon-emu** uses the same function for its emulated ICON clock. Its login screen shows, for example, `October 29 1994`, and files QNX writes there match RustyBackup's.

## RustyBackup today (branch `qnx2fs-crate`)

- **The adapter** (`src/fs/qnx2.rs`) re-exports `decode_date` and `encode_date`. It sets `FileEntry.modified_unix` from `decode_date`, which gives the wall-clock seconds as if they were UTC: the FAT convention in `times::dos_datetime_to_unix`. The display string `modified` comes from `format_unix_timestamp`, so it shows the ICON's wall clock correctly.
- **Writes** get their date from `CreateFileOptions.unix_times` (via `mtime`) through `NewEntry::mtime`. With qnx2fs 0.3, out-of-range dates are folded with no adapter change.
- **Exports** (`fork_export::apply_host_mtime`, tar) stamp host files with `modified_unix` taken as UTC. A host then shows the ICON time shifted by the zone offset. The ICON said 23:41; a Mac in New York shows 19:41.

## What to do

1. **Depend on `qnx2fs = "0.3"`** in all four manifests: main, `rb-cli-vintage`, `rb-cli-ppc` and `rb-cli-sol9`. 0.3.0 still needs Rust 1.73 and has no dependencies; its CI checks both. Since 0.2, `NewEntry` has an `attributes` field, so the adapter's struct literal needs `..Default::default()`. Refresh only the `qnx2fs` lockfile entries.
2. **Choose the time-zone convention for QNX 2** (`times.rs` already has both styles). The two consistent options:
   - **(a) Local, like classic HFS / MFS. Recommended.** The ICON's clock was a local wall clock, and icon-emu's web page runs it on the viewer's local time.
     - Reading: `modified_unix` = the ICON wall-clock time interpreted in the host's local zone.
     - Writing: take the source's UTC mtime, turn it into a local wall-clock time, then let the crate fold and encode it.
     - The host then shows the time the ICON showed.
     - Prefer `chrono::Local.from_local_datetime(…)` (handles DST for the date itself) over a single current `local_utc_offset_secs()`. icon-emu's `icon-cli disk get` does it that way.
   - **(b) UTC, like FAT.** Leave it as it is. Round trips inside RustyBackup are exact, but host files show the ICON time shifted by the zone offset.

   Whichever is chosen, keep the round trip exact: ICON → tar → ICON must give back the same date words for any date in range.
3. **Don't write dates with `encode_date` anywhere in RustyBackup.** Re-export and use `fold_date` (or just pass `mtime` and let the crate fold). Keep `decode_date` for reading.
4. **Tell the user when dates were moved.** In `put`, `untar`, `import` and `cp` from a non-QNX source, count the files whose date was outside 1980–1995 and print one line. For example: *"QNX 2 stores years 1980–1995: 12 files were dated with today's day and time in 1994 (29 February in 1992)."* Never move dates silently.
5. **Document it** wherever RustyBackup describes QNX 2 support (README filesystem table, the man page or CLI reference):
   - the 1980–1995 limit and the 1994 rule;
   - that copies between QNX 2 images keep dates exactly;
   - the time-zone convention chosen in step 2;
   - **check the date an ICON shows at start-up.** Its login screen shows it, and everything it writes from then on gets that date.
6. **Tests** (regression cases and unit tests):
   - `put` of a host file dated 2026-03-14 10:00 local: `ls` shows **1994-03-14 10:00**;
   - a file dated 2024-02-29: **1992-02-29**; one dated 2024-03-01: **1994-03-01**;
   - a host file dated 1988-03-23 keeps its date;
   - `get` / `tar` of an ICON file dated 1985-10-03 23:41: the host file shows **23:41 local** with convention (a);
   - `tar` then `untar` back to QNX 2: the same date words;
   - `cp` between two QNX 2 images: dates identical, folders included.
7. **Check against QNX itself**, in icon-emu:
   - `icon-cli disk IMAGE ls PATH` shows dates through the same crate;
   - boot the image with `icon-cli serve --hd IMAGE` and compare with QNX's own listing: `icon-ci run 'files 1:/PATH'`;
   - `chkfsys` should stay clean.

## Where things are

- **`~/repos/qnx2fs`:**
  - `src/lib.rs`: `fold_date`, `encode_date`, `decode_date`, and the module doc "Time";
  - `README.md`, section Dates;
  - the test `modern_dates_fold_into_1994_keeping_day_and_time`.

  Releases go through its pipeline only (bump the version and CHANGELOG, then `gh workflow run publish-crates-io.yml --ref main`).
- **`~/repos/icon-emu`:**
  - `crates/icon-core/src/devices/rtc.rs`: the emulated clock, via `fold_date`;
  - `crates/icon-cli/src/disk.rs`: `get` stamps host files in local time;
  - `docs/ADMIN.md`, section Dates.
- **RustyBackup:** `src/fs/times.rs` (the FAT and classic-Mac conventions, `local_utc_offset_secs`), `src/fs/fork_export.rs::apply_host_mtime`, and `src/fs/qnx2.rs` (the adapter).
