# Handoff to RustyBackup: move QNX 2 onto the `qnx2fs` crate

Paste everything below the line into RustyBackup's main session.

---

Goal: finish moving RustyBackup's QNX 2 support onto the shared **`qnx2fs`** crate, and land it on `main`. The work is done on branch **`qnx2fs-crate`**, but it is **not committed yet**. Read this whole note before touching the branch.

## Why

The QNX 2 reader / writer / fsck in `src/fs/qnx2.rs` was extracted into its own crate, `qnx2fs`, and relicensed **MIT** (same author). icon-emu, which is MIT, now uses it for its browser **Files** panel and its headless `icon-ci ls/get/put/mkdir/rm` commands. RustyBackup should use the same crate, so there is one copy of the QNX 2 logic. From now on, **QNX 2 format fixes go into `qnx2fs`, not into RustyBackup.**

An MIT dependency inside the AGPL-3.0 RustyBackup is fine.

## The crate

- Repository: <https://github.com/danifunker/qnx2fs> (public), local checkout `~/repos/qnx2fs`.
- **qnx2fs 0.2.0** (pushed; it goes to crates.io once its Release workflow runs) adds `Attributes`, `NewEntry::attributes` / `NewEntry::like`, `Volume::copy_from` (file or folder tree from another volume, keeping permissions, owners and dates), `Volume::replace` (overwrite keeping permissions and owner) and `Volume::set_attributes`. **Breaking:** `NewEntry` has a new field, so the adapter's `NewEntry { mode, uid, gid, mtime }` literal needs `..Default::default()`. Worth using: RustyBackup's `cp` between two images, and any "replace" path, can keep QNX metadata exactly through these.
- **Published on crates.io as `qnx2fs` 0.1.0** (<https://crates.io/crates/qnx2fs>). Releases go through its opticaldiscs-rs-style pipeline (`ci.yml` + manual `publish-crates-io.yml`). icon-emu depends on `qnx2fs = "0.1"`.
- No dependencies. `chrono` is replaced by civil-date arithmetic. `rust-version = "1.73"`. Its CI checks Rust 1.73, `wasm32-unknown-unknown`, fmt, clippy `-D warnings` and docs on Linux / macOS / Windows. The first run passed.
- The on-disk logic is the code from `src/fs/qnx2.rs` at `8460029f`, unchanged. The API is standalone:
  - `Volume<D>`: `open(dev, offset)`, `root()`, `list_dir(&Entry, include_deleted)`, `read_file`, `lookup(path)`, `check() -> Report`, `total_blocks()`, `free_blocks()`, `root_first()`, `set_clock(unix)`.
  - With `D: Write`: `create_file`, `create_dir`, `delete`, `rename`, `set_permissions`, `set_owner`, `flush`. Path helpers: `put`, `get`, `mkdir`, `remove`, `rename_path`, `list`.
  - `Entry`: name, path, `first` (first extent block), `is_dir`, size, owner / permission bytes, raw date, status, `deleted: Option<DeletedState>`. Methods: `unix_mode()`, `owner_label()`, `describe()` (the same string as RustyBackup's old `native_attrs`), `modified()`.
  - Free functions: `looks_like_qnx2`, `looks_like_rs_text`, `rs_to_lf`, `lf_to_rs`, `decode_date`, `encode_date`, `format_date`, `validate_name`, `sanitize_name`, `RS`.
  - Errors are `qnx2fs::Error`: `Io`, `NotQnx2`, `InvalidData`, `NotFound`, `AlreadyExists`, `DiskFull`, `Unsupported`.

## What is on branch `qnx2fs-crate`

The branch was cut from `main` at `8460029f`. Its changes are in the working tree, partly staged; check `git status` first.

1. **`src/fs/qnx2.rs`** is now a thin adapter, about 350 lines instead of about 1,600.
   - It implements `Filesystem` / `EditableFilesystem` on `qnx2fs::Volume`, and maps `qnx2fs::Error` to `FilesystemError`.
   - It keeps the `[deleted]` / `[deleted N]` naming, `native_attrs`, owner labels, Unix modes and the fsck stats (`Blocks`, `Free blocks`, `Deleted entries`).
   - It re-exports `decode_date`, `encode_date`, `looks_like_qnx2`, `looks_like_rs_text`, `rs_to_lf` and `RS`, so `src/partition/mod.rs`, `src/fs/mod.rs`, `src/cli/verbs/get.rs` and `src/gui/browse_view.rs` compile unchanged.
   - The detailed format tests moved into the crate. Two adapter tests remain: listing with deleted-entry tagging, and a trait-level edit round trip with fsck.
2. **All four manifests** (`Cargo.toml`, `rb-cli-vintage/`, `rb-cli-ppc/`, `rb-cli-sol9/`) still say `qnx2fs = { git = "https://github.com/danifunker/qnx2fs" }`, written before the crate was published.
   - **First step:** change each to `qnx2fs = "0.2"` (or `"0.1"` until 0.2.0 is published), the published crate, then refresh only that entry in each lockfile (`cargo update -p qnx2fs --manifest-path <manifest>`). The lockfiles currently hold just the git entry; nothing else was re-resolved.

## What was checked

All of this was run with `rb-cli` built from the branch, in release mode, with `--no-default-features --features native-zstd,native-zlib,chd`:

- `cargo test --lib fs::qnx2`: both adapter tests pass, with no warnings in `qnx2.rs`.
- The steps of `edit.qnx2.lexicon-10mb-hd`, replayed by hand on a local copy of the installed LEXICON 10 MB disk (`~/repos/icon-emu/disks/lexicon-4.10.01-10mb.img`, not the NAS fixture):
  - get, mkdir and put of the 115,509-byte `hd_atp_scsi_boot`, which round-trips byte-identical;
  - put, mv, chmod 750 and chown 4:12 of `payload.bin`;
  - `ls -o` prints `-rwxr-x---`, `12.4` and `renamed  (owner rwaem group r--e- other -----`;
  - fsck is clean;
  - after `rm -r`, the count is back to the image's starting count. That count is 425 files / 22 dirs on this local image; the NAS fixture says 20 dirs, because it is a different image.
- 64 MB LEXICON disk: the partitions are at 88 / 65623 / 131158. A put and get on `@2` round-trips identical, and fsck is clean.
- YorkU System SW 4.10.01 disk 1: lists `QNX_1` (4494) and `bitmap` (200); fsck finds 142 files / 5 dirs, no errors.
- The real ST-251 image (`icon4-st251-4.10.01.img`, unpadded): fsck finds 431 files / 20 dirs.
- On the icon-emu side, real QNX `chkfsys` was clean on disks written through the crate: a hard disk written while the fileserver was off, and a diskette written while it ran.

## Not done: the commit is blocked by the pre-commit hook

The hook runs `cargo clippy --all-targets -- -D warnings`. The toolchain floats on stable, now **1.99.1** (`rustfmt` and `clippy` were added with `rustup component add`). Clippy 1.99 reports **71 errors, none in this branch's files**. They are mostly the new `chunks_exact_to_as_chunks` lint, plus others, in files such as `src/rbformats/qcow2.rs`, `src/fs/hfsplus.rs`, `src/fs/efs_v1.rs`, `src/rbformats/flux/mfm_emu.rs`, `src/partition/*` and `src/rbformats/icon_hd.rs`. So `main` fails the hook too. opticaldiscs-rs fixed the same lint in its commit `3e4f414`. CONTRIBUTING forbids `--no-verify`.

Suggested order:
1. On `main`, or on its own branch first: fix the clippy 1.99 lints in one `style:` commit. Keep the Rust 1.73 floor in mind: `as_chunks` is newer than 1.73, so engine code may need the `compat` / `rust173_compat` shims or an `#[allow]`, not the suggested rewrite.
2. Rebase `qnx2fs-crate` onto that and commit the switch. A draft message:
   `refactor(fs): QNX 2 through the shared qnx2fs crate`, with a body noting that the adapter's output is unchanged and the checks above.

## Still to verify before merging

- **Default-feature build** (GUI included): only the slim build above was compiled. Run `cargo build --all-targets` (zero warnings, per CONTRIBUTING) and `cargo test --lib`.
- **The regression suite** with the NAS corpus: at least tier 2 `read.qnx2.*` and tier 3 `edit.qnx2.*`, via `rb-regress run --tiers 0-3 --require-clean`. The `fs.qnx2.*` fixtures are on the NAS, not in the repo.
- **The vintage build on 1.73.0**, as in `release.yml`'s 1.73 jobs. The crate's own CI already builds and tests on 1.73, so this should pass. `rb-cli-ppc` and `rb-cli-sol9` (mrustc, still "scoping") need only the manifest line; `cargo vendor` picks up a crates.io dependency like any other.
- **Docs:** `docs/PLAN-qnx.md`, the README filesystem table row for QNX 2, and `OUTSTANDINGWORK.md` should say that the QNX 2 code now lives in `qnx2fs`.
- Optionally, a THIRD_PARTY / dependency note that `qnx2fs` is MIT.

## Disk space

Building RustyBackup in debug from scratch filled this Mac's disk once today (it was down to 0 bytes). Prefer release builds, keep an eye on `df -h`, and follow `docs/build-memory-crashes.md`.
