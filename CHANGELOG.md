# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Fixed

- `trip` and `fall_trip` are atomic like the other fields, so `debounce_get_trip()` and `debounce_get_fall_trip()` no longer race with `debounce_set_trip()` and `debounce_set_fall_trip()` under the single-writer, many-readers contract.

### Changed

- The atomicity mode defaults to volatile when the compiler defines `__STDC_NO_ATOMICS__`, so toolchains without `<stdatomic.h>` build without a flag; the Meson `atomicity_mode` option gains an `auto` default that leaves the choice to the header.
- The C11 mode fails at compile time on targets whose 8-bit or 16-bit atomics are not always lock-free, instead of calling libatomic helpers on every access. Such single-core targets select `DEBOUNCE_USE_VOLATILE_ATOMIC`.
- Added SPDX headers and a project security policy, cleaned ignore rules, and updated contributor guidance to the shared coverage baseline.

## [1.2.0] - 2026-05-27

### Added

- Added configurable atomicity mode, transition callbacks, ThreadSanitizer coverage, CI workflow improvements, and project metadata files.

### Changed

- Documented the single-writer / many-readers contract and made unsupported atomicity modes fail explicitly.

## [1.1.0] - 2026-04-03

### Added

- Added symmetric debounce, edge detection, runtime trip reconfiguration, optional transition callbacks, and expanded unit-test coverage.
