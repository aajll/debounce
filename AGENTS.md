# AGENTS.md

---

## 1) Project-specific instructions

**Project:** `debounce`
**Primary goal:** A header-only C11 saturating-counter debounce primitive with a sticky latch, for embedded systems, with no allocation and no global state.

### 1.1 Essential commands

#### Configure and build (library only)

```sh
meson setup build --wipe --buildtype=release -Dbuild_tests=false
meson compile -C build
```

#### Configure, build, and run unit tests

```sh
meson setup build --wipe --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

#### MISRA C:2023 (runs on the committed `analysis/` compilation database)

```sh
misch run --baseline
misch run --baseline --profile volatile
```

#### Formatting (pinned clang-format through `uvx`)

```sh
scripts/format.sh          # check, as CI does
scripts/format.sh --fix    # rewrite in place
```

### 1.2 Notes

- `meson setup` generates the optional `debounce_version.h` into the build directory.
- `-Datomicity_mode` and `-Denable_callbacks` are project arguments: they affect this project's tests, not a consumer's translation units.
- Regenerate a MISRA baseline only with `MISRA_RULE_TEXTS` unset (`env -u MISRA_RULE_TEXTS misch baseline`), because the rule texts change the recorded messages.

---

## 2) Non-negotiable design invariants

- `counter` stays in `[0, trip]` and `fall_counter` in `[0, fall_trip]`. Any change that alters a threshold resets the matching counter.
- `trip` is never zero after a successful initialisation.
- The latch is set whenever the output asserts and is cleared only by `debounce_clear_latch()` and `debounce_reset()`. `debounce_disable()` never clears it.
- Every update does constant work: no loops, no recursion, no allocation, no blocking.
- One writer and many readers per object. Reader queries load exactly one field; `debounce_rose()` and `debounce_fell()` are writer-context only.
- The C11 mode keeps its static assertions that the 16-bit and `bool` atomics are lock-free. Never let an access fall through to libatomic.

---

## 3) CI / source of truth

- CI definitions live in `.github/workflows/ci.yml`.
- Prefer running the same commands locally as CI runs (see §1.1 above).
- Install Python tools (Meson, Ninja, gcovr, misch) with `uv tool install`, never `pip`; CI does the same through `astral-sh/setup-uv`.
- The CI jobs are: tests under ASan/UBSan with an installed-consumer smoke test, ThreadSanitizer on the concurrency test, the `volatile` mode, a release build, a TI C2000 header build (CGT 22.6.2 and 25.11.1, COFF and EABI, callbacks off and on, warnings as errors), `clang-format`, a 100 percent line and branch coverage gate, and `misch` against the baselines for both modes.

---

## 4) Docs / commit conventions

### Documentation layout

- `README.md` is a short landing page (about 70 lines). Put detail in `docs/`, never in the README; link to the owning file instead.
- `docs/integration.md` owns requirements, install methods, configuration, building, and cross builds.
- `docs/api.md` owns the API reference. Change it together with `include/debounce.h`; keep signatures in fenced `c` blocks and describe them in prose, not tables.
- `docs/design.md` is the single authoritative design record.
- `CONTRIBUTING.md` documents the contributor workflow; `CHANGELOG.md` holds release history; `.github/SECURITY.md` covers vulnerability reporting.
- Code blocks in `README.md`, `CONTRIBUTING.md`, and `docs/` are compiled by the `debounce docs examples` test (`tests/docs/check_snippets.py`). Each document is compiled as one source file: a `c` fence is file-level code, `<!-- snippet: body -->` wraps statements, and `<!-- snippet: skip -->` excludes pseudo-code. Do not use `skip` to silence a broken example. Declare illustrative application symbols in `tests/docs/doc_stubs.h`.
- Prose uses British spelling. Do not hard-wrap Markdown: write one line per paragraph. The 80-column limit applies to C source, not Markdown.

### Commits

- Use **Conventional Commits** format when asked to commit.
- Keep commits focused; explain *why* in the message body.

---

## 5) C style expectations

### Build & configuration

- Use the Meson build system. Do not introduce CMake, Make, or other systems.
- The library is header-only. Update `tests/meson.build` when adding or removing test files.

### Formatting

- `.clang-format` is present and **mandatory**. Run `scripts/format.sh --fix` before committing. It runs the clang-format release pinned in the script through `uvx`. Do not use a system `clang-format`: another release can format the same code differently.
- Do not reformat unrelated code.
- Key settings: 8-space indent, `BreakBeforeBraces: Linux`, column limit 80.

### Style & correctness

- Match conventions in the existing files (indentation, braces, naming).
- Validate pointer arguments at every public API boundary.
- No heap allocation (`malloc` / `free` / VLAs).
- Use `uint16_t` and `bool` from `<stdint.h>` / `<stdbool.h>`; never plain `int` for counters or thresholds, and no `uint8_t`.
- Access fields only through `DB_LOAD` and `DB_STORE`.
- One exit per function, through a `result` variable.

### Error handling

- Public functions return `bool` or a value; a misconfiguration returns `false`.
- No `errno`; no exceptions.

### Comment placement (Doxygen)

- Inline trailing annotations (`/**< ... */`) on `enum`/`struct` members are allowed only when the resulting line fits the 80-column limit.
- If any member's annotation would overrun, move **all** of that aggregate's member docs into a single structured Doxygen block above the type, as an `@details` list of `- ::SYMBOL  description` entries.
- Never mix inline and block forms within one aggregate, and never leave a trailing comment that clang-format would wrap onto a second line.
- After editing, run `scripts/format.sh` and an 80-column scan.

### Testing

- Run `meson test -C build` after every change.
- Add a test case for each bug fix.
- Tests live in `tests/test_*.c`; all tests must pass in both atomicity modes.
- Keep `include/` at the coverage gate (100 percent line and branch).
