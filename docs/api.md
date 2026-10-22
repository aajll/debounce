# API reference

Every declaration lives in [`include/debounce.h`](../include/debounce.h), with configuration in [`include/debounce_conf.h`](../include/debounce_conf.h). Both carry the full Doxygen contract. The reasons behind these rules are in [design.md](design.md).

## Conventions

- All state lives in a caller-owned `struct debounce`. Every function takes a pointer to it, and there is no global state.
- Every function is `static inline` in the header, so there is nothing to link.
- A `NULL` object is never dereferenced. Functions that return a value return `false` or `0`; the rest do nothing.
- One context mutates an object, and any number of contexts may read it, within the limits in [design.md](design.md#concurrency).
- There is no `errno` and no logging. Functions that can be misconfigured return `false`.

## The object

<!-- snippet: skip -->
```c
struct debounce {
        DB_ATOMIC(uint16_t) trip;          /* assertion threshold, never 0 */
        DB_ATOMIC(uint16_t) counter;       /* consecutive asserted ticks */
        DB_ATOMIC(uint16_t) fall_trip;     /* de-assertion threshold, 0 = immediate */
        DB_ATOMIC(uint16_t) fall_counter;  /* consecutive de-asserted ticks */
        DB_ATOMIC(bool) output;            /* debounced output */
        DB_ATOMIC(bool) latch;             /* sticky latch */
        DB_ATOMIC(bool) enabled;           /* processing gate */
        DB_ATOMIC(bool) prev_output;       /* output before the last update */
#if DEBOUNCE_ENABLE_CALLBACKS
        debounce_callback_t callback;
#endif
};
```

`DB_ATOMIC` is `_Atomic` or `volatile`, depending on the [atomicity mode](integration.md#configuration). Allocate the object anywhere, normally statically, and use only the functions below: direct field access bypasses both the invariants and the atomicity mode. The layout can change between minor versions.

## Lifecycle

<!-- snippet: skip -->
```c
bool debounce_init(struct debounce *db, uint16_t trip);
bool debounce_init_symmetric(struct debounce *db, uint16_t rise_trip, uint16_t fall_trip);
void debounce_reset(struct debounce *db);
```

`debounce_init` sets a one-sided debouncer: assertion needs `trip` consecutive ticks and de-assertion is immediate. It clears the counters, output, latch and edge state, enables the object, and clears any callback. It returns `false` and leaves the object untouched when `db` is `NULL` or `trip` is zero. Call it once before any other function; treat a `false` return as a configuration fault.

`debounce_init_symmetric` does the same with a separate de-assertion threshold. `rise_trip` must be non-zero; a `fall_trip` of zero gives the one-sided behaviour of `debounce_init`.

`debounce_reset` clears the counters, output, latch and edge state. It keeps the thresholds, the enable gate and the callback.

```c
static struct debounce button;

void
button_init(void)
{
        /* 5 true ticks to assert, 3 false ticks to de-assert. */
        if (!debounce_init_symmetric(&button, 5u, 3u)) {
                configuration_fault();
        }
}
```

## Update

<!-- snippet: skip -->
```c
bool debounce_update(struct debounce *db, bool cond);
```

`debounce_update` processes one tick and returns the debounced output. While `cond` is true the assertion counter rises, saturating at `trip`; when it reaches `trip` the output and the latch become true. When `cond` is false the assertion counter resets, and then:

- with `fall_trip == 0`, the output clears at once;
- with `fall_trip > 0`, the output stays true until `fall_trip` consecutive false ticks have been seen.

The latch is never cleared here. A disabled object returns `false` and changes nothing. With callbacks enabled, a registered callback runs inside this call on each output edge.

## Queries

<!-- snippet: skip -->
```c
bool     debounce_is_active(const struct debounce *db);
bool     debounce_is_latched(const struct debounce *db);
bool     debounce_is_enabled(const struct debounce *db);
uint16_t debounce_get_counter(const struct debounce *db);
uint16_t debounce_get_trip(const struct debounce *db);
uint16_t debounce_get_fall_trip(const struct debounce *db);
```

`debounce_is_active` returns the debounced output without processing a tick. `debounce_is_latched` returns `true` once the output has asserted, until `debounce_clear_latch` or `debounce_reset`. `debounce_is_enabled` returns the enable gate. The getters return the assertion counter and the two thresholds.

Each query reads one field, so it is safe from a reader context while another context updates the object.

## Edges

<!-- snippet: skip -->
```c
bool debounce_rose(const struct debounce *db);
bool debounce_fell(const struct debounce *db);
```

`debounce_rose` returns `true` for the one tick after the output changes from false to true, and `debounce_fell` for the one tick after it changes from true to false. In symmetric mode, `debounce_fell` fires when the output clears, after `fall_trip` false ticks, not on the first false input. Neither reports an edge after `debounce_reset` or a disable and enable cycle.

Both compare two fields, so call them from the context that calls `debounce_update`, normally straight after it.

<!-- snippet: body -->
```c
(void)debounce_update(&button, read_button_pin());
if (debounce_rose(&button)) {
        on_press();
}
if (debounce_fell(&button)) {
        on_release();
}
```

## Runtime reconfiguration

<!-- snippet: skip -->
```c
bool debounce_set_trip(struct debounce *db, uint16_t trip);
bool debounce_set_fall_trip(struct debounce *db, uint16_t fall_trip);
```

`debounce_set_trip` changes the assertion threshold. It returns `false` when `db` is `NULL` or `trip` is zero. On success it also resets the assertion counter, the output and the edge state, because the old count may exceed the new threshold; `debounce_rose` and `debounce_fell` report no edge until the next update. The latch, the gate, and the de-assertion state are unchanged.

`debounce_set_fall_trip` changes the de-assertion threshold, where zero selects immediate de-assertion, and resets the de-assertion counter. It returns `false` only when `db` is `NULL`.

## Latch

<!-- snippet: skip -->
```c
void debounce_clear_latch(struct debounce *db);
```

`debounce_clear_latch` clears the sticky latch and nothing else. If the output is still true, the next `debounce_update` sets the latch again, so clear it after the condition has cleared.

## Enable gate

<!-- snippet: skip -->
```c
void debounce_enable(struct debounce *db);
void debounce_disable(struct debounce *db);
```

`debounce_disable` closes the gate and clears both counters, the output and the edge state. It keeps the latch, so a disable and enable cycle never discards a fault record. `debounce_enable` opens the gate and changes nothing else, so the object restarts from a clean state without a reset.

Use the gate to ignore known transients, such as start-up, or to arm one debouncer from another:

```c
static struct debounce ready_db;
static struct debounce fault_db;

void
monitor_init(void)
{
        (void)debounce_init(&ready_db, 20u);
        (void)debounce_init(&fault_db, 5u);
        debounce_disable(&fault_db); /* inhibit until the system is ready */
}

void
monitor_tick(bool system_ready, bool fault_cond)
{
        if (debounce_update(&ready_db, system_ready)) {
                debounce_enable(&fault_db);
        }
        (void)debounce_update(&fault_db, fault_cond);
}
```

## Transition callbacks

<!-- snippet: skip -->
```c
#if DEBOUNCE_ENABLE_CALLBACKS
typedef void (*debounce_callback_t)(struct debounce *db, bool rose);
void debounce_set_callback(struct debounce *db, debounce_callback_t cb);
#endif
```

Available only when `DEBOUNCE_ENABLE_CALLBACKS` is `1`. `debounce_set_callback` registers a function that `debounce_update` calls on each output edge, with `rose` true on a rising edge and false on a falling edge; `NULL` removes it. The callback is configuration, so `debounce_reset` and `debounce_disable` keep it, and `debounce_init` clears it. It runs in the updating context, which may be an interrupt, so keep it short and non-blocking.

## Version

`debounce.h` defines the release it belongs to, so a copied-in header still identifies itself:

<!-- snippet: skip -->
```c
#define DEBOUNCE_VERSION_MAJOR 1
#define DEBOUNCE_VERSION_MINOR 2
#define DEBOUNCE_VERSION_PATCH 0
#define DEBOUNCE_VERSION "1.2.0"
```

`DEBOUNCE_VERSION_NUMBER` is `MAJOR * 10000 + MINOR * 100 + PATCH` as a `long`, so it holds any version where `int` is 16 bits. Compare it in the preprocessor to require a minimum release:

```c
#if DEBOUNCE_VERSION_NUMBER < 10200L
#error "debounce 1.2.0 or later is required"
#endif
```

`DEBOUNCE_VERSION_STRING` is the same string as `DEBOUNCE_VERSION`.
