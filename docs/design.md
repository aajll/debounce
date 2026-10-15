# Design

`debounce` is a header-only C11 library that filters a boolean condition sampled once per tick. A saturating counter asserts the output only after the condition has held for a configured number of consecutive ticks, and a sticky latch records the assertion until the application acknowledges it. The library owns no timer, no input sampling, and no scheduling; the caller decides what a tick is.

## Goals and non-goals

Goals:

- Deterministic, constant-time updates usable from interrupt context and hard real-time loops.
- Separate level and fault-hold outputs, so one object serves both signal cleaning and fault reporting.
- Caller-owned state with no heap and no global state.
- One writer and concurrent readers, without locks and without libatomic.

Non-goals:

- Sampling hardware inputs or measuring time. A tick is whatever the caller's update rate is.
- Analogue filtering, hysteresis on a numeric value, or majority voting over a window.
- Combining several conditions in one object. Compose objects instead.
- Serialising several writers on one object.

## State model

Each `struct debounce` holds two thresholds, two counters, and four flags.

- `trip` is the number of consecutive true ticks needed to assert. It is never zero after a successful initialisation, which `debounce_init` and `debounce_set_trip` enforce.
- `counter` counts consecutive true ticks and saturates at `trip`, so it always lies in `[0, trip]` and never wraps, however long the condition holds.
- `fall_trip` is the number of consecutive false ticks needed to de-assert. Zero means de-assertion is immediate.
- `fall_counter` counts consecutive false ticks while the output is true, and saturates at `fall_trip`.
- `output` is the debounced level.
- `latch` becomes true whenever `output` does, and only `debounce_clear_latch` and `debounce_reset` clear it.
- `enabled` is the processing gate.
- `prev_output` holds `output` from before the most recent update, for edge detection.

### Level output and latch

The two outputs answer different questions. `output` says whether the condition is currently, cleanly true; it needs no acknowledgement and suits a debounced switch or threshold. `latch` says whether the condition has asserted since the last acknowledgement; it suits a fault indicator that must stay visible after the fault clears. An application that only needs a level never touches the latch.

`debounce_clear_latch` clears only the latch. If the output is still true, the next update sets it again, which keeps the latch from hiding a fault that is still present.

## Update rules

`debounce_update` first copies `output` to `prev_output`, then applies one tick:

- A true input resets `fall_counter` and advances `counter` up to `trip`. Once `counter` reaches `trip`, `output` and `latch` become true.
- A false input resets `counter`. With `fall_trip == 0`, `output` clears at once. Otherwise, while `output` is true, `fall_counter` advances, and `output` clears when it reaches `fall_trip`.

A single false tick therefore restarts the assertion count, and in symmetric mode a single true tick restarts the de-assertion count. Each update does a fixed amount of work with no loops, so its cost does not depend on the thresholds.

Edges come from comparing `output` with `prev_output`. `debounce_rose` and `debounce_fell` are true for exactly the one update in which the output changed. `debounce_reset` and `debounce_disable` clear `prev_output` together with `output`, so neither creates an edge.

### Enable gate

A disabled object ignores updates and reports `false`. `debounce_disable` also clears the counters, the output and the edge state, so `debounce_enable` restarts from a clean state. It keeps the latch, because a disable and enable cycle, such as around a start-up transient, must not discard a fault record.

The gate is how two objects compose: one debounces a "ready" condition, and its output enables the second. Keeping arming outside the object keeps each object a single concern and each condition separately observable.

### Configuration changes

`debounce_set_trip` resets `counter` and `output`, because a count accumulated under a larger threshold could exceed a smaller one and break the `counter <= trip` invariant. `debounce_set_fall_trip` resets `fall_counter` for the same reason. Neither touches the latch.

Transition callbacks are optional at compile time. When `DEBOUNCE_ENABLE_CALLBACKS` is `0`, the callback field and the dispatch code are not compiled, so the object and `debounce_update` carry no cost for them. When enabled, the callback runs at the end of `debounce_update`, after the state is consistent, and is treated as configuration that survives reset and disable.

## Concurrency

The contract is one writer and any number of readers per object:

- The writer is the one context that calls `debounce_update`, `debounce_reset`, `debounce_enable`, `debounce_disable`, `debounce_set_*`, and `debounce_clear_latch`.
- Readers may call `debounce_is_active`, `debounce_is_latched`, `debounce_get_counter`, `debounce_get_trip`, and `debounce_get_fall_trip` concurrently with the writer.
- Two contexts that both mutate one object must be serialised by the application, for example with a mutex or by masking an interrupt. No mode protects that case.

Each reader query loads one field, so it always sees a value the field really held. `debounce_update` as a whole is not atomic: a reader can see `output` from after an update and `counter` from before it. `debounce_rose` and `debounce_fell` load two fields, so a concurrent reader could see a spurious or missed edge; they belong to the writer's context.

### Atomicity modes

Every field access goes through three macros in `debounce.h`: a qualifier, a load, and a store. One of two modes defines them.

| Mode | Selector | Fields | Accesses |
| --- | --- | --- | --- |
| C11 | `DEBOUNCE_USE_C11_ATOMIC` | `_Atomic` | `atomic_load_explicit` and `atomic_store_explicit` with `memory_order_relaxed` |
| Single core | `DEBOUNCE_USE_VOLATILE_ATOMIC` | `volatile` | plain loads and stores |

Without a selector, the header picks the C11 mode, or the single-core mode when the compiler defines `__STDC_NO_ATOMICS__`. An unknown mode is a compile error.

Relaxed ordering is enough because each reader query depends on one field and orders nothing else. Counters advance with a load and a store rather than an atomic read-modify-write, which is correct because only the writer modifies them. On common 32-bit and 64-bit hosts the C11 mode compiles to plain loads and stores, with no fences and no locked instructions.

The C11 mode requires lock-free atomics. Static assertions require `ATOMIC_BOOL_LOCK_FREE` to be 2 and the `ATOMIC_*_LOCK_FREE` macro of the type that matches `uint16_t` to be 2. A target without native 8-bit and 16-bit atomics, such as an 8-bit or 16-bit core or ARMv6-M, fails these assertions and must select the single-core mode. Without them, every field access would call into libatomic, which may take a lock that an interrupt handler can deadlock on.

### Single-core mode

`volatile` stops the compiler from caching or reordering field accesses, but adds no hardware ordering and no atomicity. The mode is correct only when the writer and every reader run on one core and each naturally aligned 16-bit and `bool` access is a single instruction. ThreadSanitizer reports races for it on a multi-core host, as it should; that is not a defect to fix in the algorithm.

## Portability

- C11 with `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`, and `<limits.h>`, plus `<stdatomic.h>` in the C11 mode. No other dependencies.
- No `uint8_t`, so the header builds where `CHAR_BIT` is 16.
- Counter arithmetic is in `uint16_t` with an explicit cast after each increment, and saturation keeps every counter at or below its threshold, so nothing wraps.
- Every function has a single exit through a `result` variable.
- A static assertion keeps `struct debounce` at or below 64 bytes.

## Verification

The unit suite covers every public function, including `NULL` objects, zero thresholds, counter saturation, one-sided and symmetric de-assertion, edges after reset and after a disable and enable cycle, latch behaviour across clear, reset and disable, and runtime threshold changes. A separate executable covers the callback build. CI runs the suite under AddressSanitizer and UndefinedBehaviorSanitizer and again in the single-core mode, and `include/` reaches 100 percent line and branch coverage with callbacks enabled.

A threaded test runs one writer, which alternates between two configurations, against a reader that checks every query returns a value the writer could have stored; ThreadSanitizer must report no race in the C11 mode. Configuration checks that the header selects the C11 mode by default and the single-core mode under `__STDC_NO_ATOMICS__`, and, when Clang is available, compiles a probe for the 16-bit `msp430` target that requires the C11 mode to fail with the lock-free diagnostic and the single-core mode to succeed. A documentation test compiles every C example in the README and `docs/` against the header, and CI builds an installed consumer against the packaged headers.

CI compiles the header with warnings as errors under TI C2000 Code Generation Tools 22.6.2.LTS and 25.11.1.LTS in C11 C28x mode, under both the COFF and EABI ABIs, with callbacks off and on, and asserts that the single-core mode is selected. This validates the source dialect and type assumptions only. Linking, target execution, interrupt latency, and final map-file footprint remain consumer-owned checks.

MISRA C:2023 analysis with `misch` runs on both modes against a recorded baseline in `analysis/baseline/`, and CI fails on any finding outside it. The version macros in `debounce.h` carry an inline deviation of Rule 2.5 (unused macro), because they exist for consumers and the library does not use them.
