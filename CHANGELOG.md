# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [1.3.0] - 2026-10-22

### Added

- MISRA C:2023 analysis with `misch`, configured by `misra.toml`, for the C11 and volatile modes. CI fails on any finding outside the baselines in `analysis/baseline/`.
- A TI C2000 job in CI that compiles the header with CGT 22.6.2 and 25.11.1 under COFF and EABI, warnings as errors, and asserts the volatile mode is selected.
- A `clang-format` check through `scripts/format.sh`, which pins the formatter release and runs it with `uvx`.

### Changed

- `debounce.h` defines `DEBOUNCE_VERSION_MAJOR`, `_MINOR`, `_PATCH`, `DEBOUNCE_VERSION`, `DEBOUNCE_VERSION_STRING` and a new `DEBOUNCE_VERSION_NUMBER` (`long`, `MAJOR * 10000 + MINOR * 100 + PATCH`), so a copied-in header carries its version. Meson fails to configure when the header and `project(version:)` disagree, and the build needs Meson 0.57 or later.
- The atomicity mode defaults to volatile when the compiler defines `__STDC_NO_ATOMICS__`, so toolchains without `<stdatomic.h>` build without a flag; the Meson `atomicity_mode` option gains an `auto` default that leaves the choice to the header.
- The C11 mode fails at compile time on targets whose 8-bit or 16-bit atomics are not always lock-free, instead of calling libatomic helpers on every access. Such single-core targets select `DEBOUNCE_USE_VOLATILE_ATOMIC`.
- The C11 mode uses relaxed atomic loads and stores instead of sequentially consistent access, and increments counters with a load and store instead of an atomic read-modify-write. Each query reads one field, so no ordering is lost; `debounce_update()` no longer emits fences or locked instructions on x86-64 and AArch64.
- CI installs Meson, Ninja, gcovr and misch with `uv` instead of `pip`, and the coverage gate rises to 100% line and branch, the coverage already measured.
- The README is a short landing page, with installation and configuration in `docs/integration.md`, the API reference in `docs/api.md`, and the design record in `docs/design.md`. A `debounce docs examples` test compiles every C example in them against the header. CONTRIBUTING and AGENTS describe the `uv`, `misch` baseline and `scripts/format.sh` workflow.
- Added SPDX headers and a project security policy, cleaned ignore rules, and updated contributor guidance to the shared coverage baseline.

### Removed

- `debounce_version.h`. `debounce.h` defines the version macros.
- The `extern "C"` guards in `debounce.h`. The library is C11 only and its headers make no C++ claim.

### Fixed

- `-Datomicity_mode` and `-Denable_callbacks` reach consumers: the defines are compile arguments of `debounce_dep` and `Cflags` in `debounce.pc`, so a Meson subproject or pkg-config consumer compiles the header with the configured mode and callbacks.
- `debounce_set_trip` clears `prev_output` with `output`, so `debounce_fell` no longer reports a falling edge straight after the call when the output had been held high; no update produced that edge and no callback fired for it.
- `trip` and `fall_trip` are atomic like the other fields, so `debounce_get_trip()` and `debounce_get_fall_trip()` no longer race with `debounce_set_trip()` and `debounce_set_fall_trip()` under the single-writer, many-readers contract.

## [1.2.0] - 2026-05-27

### Added

- Added configurable atomicity mode, transition callbacks, ThreadSanitizer coverage, CI workflow improvements, and project metadata files.

### Changed

- Documented the single-writer / many-readers contract and made unsupported atomicity modes fail explicitly.

## [1.1.0] - 2026-04-03

### Added

- Added symmetric debounce, edge detection, runtime trip reconfiguration, optional transition callbacks, and expanded unit-test coverage.
