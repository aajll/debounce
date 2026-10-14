# debounce

[![CI](https://github.com/aajll/debounce/actions/workflows/ci.yml/badge.svg)](https://github.com/aajll/debounce/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**A header-only saturating-counter debounce primitive with a sticky fault latch, for embedded C.**

## Quick start

Copy `include/debounce.h` and `include/debounce_conf.h` into your project. There is no source file to compile. For a Meson subproject or an installed `pkg-config` package, see the [integration guide](docs/integration.md).

```c
#include "debounce.h"

/* One debounce object per monitored signal, statically allocated. */
static struct debounce pressure_fault;

void
system_init(void)
{
        /* Require 10 consecutive asserted ticks before tripping. */
        if (!debounce_init(&pressure_fault, 10u)) {
                configuration_fault();
        }
}

void
control_loop_tick(bool pressure_over_limit)
{
        set_warning_led(debounce_update(&pressure_fault, pressure_over_limit));

        if (debounce_rose(&pressure_fault)) {
                log_fault_event();
        }
}

void
operator_acknowledge(void)
{
        debounce_clear_latch(&pressure_fault);
}
```

## Description

`debounce` turns a noisy boolean condition, sampled once per tick, into a clean one. The output asserts only after the condition holds for a configured number of consecutive ticks, and a separate sticky latch records that it asserted until the application clears it.

- A level output that tracks the debounced input, and a sticky latch for fault-hold and operator acknowledgement
- One-sided debounce by default, or independent rise and fall thresholds
- Single-tick rising and falling edge queries, and optional transition callbacks
- Runtime threshold changes and an enable gate that keeps the latch across a disable
- Caller-owned state with no heap, no recursion, no loops, and no global state, so it is usable from interrupt context
- One writer and any number of concurrent readers, through C11 `<stdatomic.h>` or a single-core `volatile` mode
- 100% line and branch coverage, sanitiser-clean tests, and a MISRA C:2023 baseline from [`misch`](https://pypi.org/project/misch/)

## Validated toolchains

| Toolchain               | Target                        | Validation                                       |
| ----------------------- | ----------------------------- | ------------------------------------------------ |
| GCC, Clang              | Linux hosts                   | Unit, threaded, and sanitiser tests in both modes |
| Clang                   | 16-bit `msp430`, freestanding | Lock-free checks reject the C11 mode             |
| DSP vendor C11 compiler | 16-bit MAU (`CHAR_BIT == 16`) | Header compile, two object formats               |

Target linking, execution, and timing belong to the consuming firmware. See [cross builds](docs/integration.md#cross-builds).

## Documentation

- [Integration guide](docs/integration.md) — copy-in, Meson subproject, installed package, configuration, and cross builds
- [API reference](docs/api.md) — every function, with its rules
- [Design](docs/design.md) — state model, update rules, concurrency, and verification
- [Changelog](CHANGELOG.md) — release history

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before you open a pull request. Report security issues as described in [SECURITY.md](.github/SECURITY.md).

## License

MIT. See [LICENSE](LICENSE).
