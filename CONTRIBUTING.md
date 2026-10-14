# Contributing to debounce

`debounce` is a small, header-only C11 library for a saturating-counter debounce with a sticky latch. Two contracts carry most of the design weight: every update does constant work and keeps each counter within its threshold, and an object has one writer and lock-free readers. Keep both in mind when you change anything.

## Getting started

Install a C11 compiler and install the Python tools with [uv](https://docs.astral.sh/uv/), as CI does. The MISRA analysis also needs a current `cppcheck`; CI installs it from Homebrew because the 2.13 release in Ubuntu 24.04 does not model `<stdatomic.h>`. The same commands CI runs, locally:

```sh
# Python tools
uv tool install meson
uv tool install ninja
uv tool install gcovr
uv tool install misch

# Configure with tests and sanitisers (CI default)
meson setup build --buildtype=debug -Dbuild_tests=true -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose

# ThreadSanitizer on the concurrency test, C11 mode
meson setup build_tsan --buildtype=debug -Dbuild_tests=true -Dtsan=true
meson compile -C build_tsan && meson test -C build_tsan

# Single-core mode
meson setup build_vol --buildtype=debug -Dbuild_tests=true -Datomicity_mode=volatile
meson compile -C build_vol && meson test -C build_vol

# Coverage (CI gate is 100% line and 100% branch for include/)
meson setup build_cov --buildtype=debug -Dbuild_tests=true -Db_coverage=true -Denable_callbacks=true -Dc_args=-fprofile-update=atomic
meson compile -C build_cov && meson test -C build_cov
gcovr --root . --filter 'include/' --print-summary

# MISRA C:2023 against the recorded baselines, one per atomicity mode
misch run --baseline
misch run --baseline --profile volatile

# Formatting (pinned clang-format through uvx)
scripts/format.sh --fix
```

## Source style

- Formatting is mandatory. Run `scripts/format.sh --fix` before submitting, or `scripts/format.sh` to check without writing, as CI does. The script pins the clang-format release and runs it through `uvx`, so an editor formatter gives the same result only when it runs the same release.
- 8-space indent, Linux brace style, 80-column limit. Match the existing conventions and do not reformat unrelated code.
- The Meson build system is the single source of truth. Update `meson.build` and `tests/meson.build` when you add or remove files.
- No CMake, no Make, and no other build systems.

## C language rules

- C11 only. The library is header-only: every function is `static inline` in `include/debounce.h`, and there is no source file.
- Use fixed-width types from `<stdint.h>` and `<stdbool.h>`. Do not use `uint8_t`, and never plain `int` for counters or thresholds.
- No heap allocation (`malloc`, `free`, VLAs), no recursion, and no data-dependent loops. All state lives in the caller-owned `struct debounce`.
- Route every field access through `DB_LOAD` and `DB_STORE`, and keep both atomicity modes building.
- Validate the object pointer at every public function and return a safe default (`false` or `0`) for `NULL`. Functions that can be misconfigured return `false`.
- Give each function a single exit through a `result` variable.

## Concurrency

The supported contract is one writer and many readers per object, described in [docs/design.md](docs/design.md#concurrency). The C11 mode must stay clean under ThreadSanitizer. The `volatile` mode is correct only on a single core and is expected to report races under ThreadSanitizer on a multi-core host; do not change the algorithm to silence that.

## MISRA C:2023

The header is analysed with `misch` (cppcheck-backed MISRA C:2023 analysis), configured by `misra.toml`. Because the library is header-only, the analysis runs on `analysis/debounce_header.c` through the committed `analysis/compile_commands.json`, so it needs no build directory. The default run covers the C11 mode, and the `volatile` profile covers the single-core mode. Each has a baseline in `analysis/baseline/`, and CI fails on any finding that is not in it.

The baselines are recorded without rule texts, and the texts change the messages a finding is matched on. If you have `MISRA_RULE_TEXTS` set, unset it for `misch run --baseline` as well, as CI does.

A change must not add findings. If it removes one, regenerate the affected baselines with `MISRA_RULE_TEXTS` unset:

```sh
env -u MISRA_RULE_TEXTS misch baseline
env -u MISRA_RULE_TEXTS misch baseline --profile volatile
```

Full tool-driven compliance beyond this analysis needs a certified static analyser, which this repository does not vendor.

## Tests and coverage

- Add a test for every bug fix and every new behaviour.
- Tests live in `tests/test_*.c` and are registered in `tests/meson.build`. `tests/test_installed_consumer.c` builds against the installed headers in CI; keep it building.
- New code must build and pass in both atomicity modes, and with callbacks on and off.
- CI enforces 100 percent line and branch coverage of `include/`, measured with callbacks enabled.
- The `debounce docs examples` test compiles every C code block in `README.md`, `CONTRIBUTING.md`, and `docs/` against the public header, through `tests/docs/check_snippets.py`. A block compiles at file scope by default. On the line directly above the fence, put `<!-- snippet: body -->` for a block of statements, or `<!-- snippet: skip -->` for pseudo-code or a copy of a header definition. Declare illustrative application functions in `tests/docs/doc_stubs.h`.

## API stability

- `struct debounce` is caller-owned but opaque by contract: use the API functions, never the fields. Its layout can change between minor versions.
- The public function signatures in `debounce.h` are stable across the 1.x line. Breaking changes go in a new major release with a note in `CHANGELOG.md`.

## Documentation

| File | Purpose |
| --- | --- |
| `README.md` | Landing page: quick start, description, validated toolchains, and links. Keep it short. |
| `docs/integration.md` | Installation, configuration, building, and cross builds. |
| `docs/api.md` | Reference for every public function. |
| `docs/design.md` | State model, update rules, concurrency, portability, and verification. |

Update `docs/api.md` and the header Doxygen together when an interface changes. Update `docs/design.md` when behaviour changes. Keep it standalone: describe how the library works, not how it was built or which other projects use it. Write prose in British English, one line per paragraph.

## Commits

Use Conventional Commits:

- `feat: ...` new feature
- `fix: ...` bug fix
- `docs: ...` documentation only
- `test: ...` test-only changes
- `chore: ...` build, CI, release work
- `refactor: ...` a change that neither fixes a bug nor adds a feature

Keep the subject under about 70 characters. Use the body to explain why the change is needed.

## Pull requests

- Open an issue first for non-trivial changes so the design can be agreed before implementation.
- Keep pull requests focused: one feature or one fix.
- All CI checks must pass.

## When in doubt

Open an issue. The library is small enough that a modest change to its contract reaches every signal it debounces.
