# Integration guide

This guide shows how to add `debounce` to a project, configure it, build it, and run its tests. The API is described in [api.md](api.md), and the state model and concurrency rules in [design.md](design.md).

## Requirements

- A C11 compiler with `uint16_t`. The default C11 mode also needs `<stdatomic.h>` with lock-free 8-bit and 16-bit atomics; other targets use the `volatile` mode.
- For the tests: Meson, Ninja, Python 3, and POSIX threads.

## Copy-in

Copy-in is the recommended method for embedded targets. The library is header-only, so copy two files into your include path:

```
include/debounce.h
include/debounce_conf.h
```

Include the header from any translation unit that uses it:

```c
#include "debounce.h"
```

No generated header and no link step are needed. `debounce.h` defines the version macros described in [api.md](api.md#version).

## Meson subproject

Put the repository in `subprojects/debounce`, as a checkout or through a wrap file, then declare the dependency:

```meson
debounce_dep = dependency('debounce', fallback: ['debounce', 'debounce_dep'])
```

The build calls `meson.override_dependency('debounce', ...)`, so other subprojects in the same build find `debounce` by name. Tests are off by default, so a subproject build adds only the include path. Include the header as `"debounce.h"`.

## Installed package

```sh
meson setup build --buildtype=release -Dbuild_tests=false --prefix=/usr/local
meson compile -C build
meson install -C build
cc app.c $(pkg-config --cflags debounce) -o app
```

After installation the headers live in a `debounce/` subdirectory:

<!-- snippet: skip -->
```c
#include <debounce/debounce.h>
```

## Configuration

Every translation unit that includes `debounce.h` must see the same configuration, because it changes the layout of `struct debounce`. Set it with compiler flags, or in a project header included before `debounce.h`.

| Macro | Default | Meaning |
| --- | --- | --- |
| `DEBOUNCE_ENABLE_CALLBACKS` | `0` | Set to `1` to add a transition callback pointer to `struct debounce` and the dispatch code to `debounce_update()`. |
| `DEBOUNCE_USE_C11_ATOMIC` | selected when the compiler has C11 atomics | Every field is `_Atomic` and accessed with relaxed loads and stores. |
| `DEBOUNCE_USE_VOLATILE_ATOMIC` | selected when the compiler defines `__STDC_NO_ATOMICS__` | Every field is `volatile`, for single-core targets. |

Define at most one `DEBOUNCE_USE_*` macro. Select `DEBOUNCE_USE_VOLATILE_ATOMIC` on a single-core target whose 8-bit or 16-bit atomics are not lock-free, such as an 8-bit or 16-bit core or ARMv6-M; the C11 mode fails to compile there. [design.md](design.md#atomicity-modes) explains the choice.

The Meson options `-Datomicity_mode={auto,c11,volatile}` and `-Denable_callbacks=true` set the same macros for this project's own tests. They are project arguments, so they do not reach a consumer's translation units: a consumer defines the macro in its own build. `auto`, the default, leaves the mode to the header.

## Build and test

```sh
# Header install only
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# Unit, callback, concurrency, and documentation tests
meson setup build --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

The suite tests the API in both atomicity modes, the callback build, a threaded single-writer, many-readers run, and the compiled documentation examples. Configuration also checks that the header picks the right mode with and without `__STDC_NO_ATOMICS__`, and, when Clang is available, that the C11 mode refuses a 16-bit target. Sanitiser, coverage, formatting, and MISRA commands are in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Cross builds

The consuming firmware owns its cross file, linker command file, and memory placement. No cross file ships with this project.

- CI compiles the header with warnings as errors using two releases of a vendor C11 compiler for a 16-bit-MAU DSP target, where `CHAR_BIT` is 16, under both of its object formats, with and without callbacks. That compiler has no C11 atomics, so the header selects the `volatile` mode.
- Configuration checks the lock-free rule with Clang for a 16-bit `msp430` target, without a cross toolchain.

These results cover the source dialect and type assumptions only. Linking, execution, interrupt timing, and the final map-file footprint remain checks for the consumer.
