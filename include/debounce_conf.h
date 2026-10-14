/**
 * SPDX-License-Identifier: MIT
 *
 * @file: debounce_conf.h
 *
 * @brief
 *    Compile-time configuration for the debounce library.
 *
 * @details
 *    Users may override any option below by defining the macro before
 *    including this header, or via a compiler flag (e.g.
 *    -DDEBOUNCE_ENABLE_CALLBACKS=1).  The #ifndef guards ensure that
 *    user-supplied definitions take precedence.
 */

#ifndef DEBOUNCE_CONF_H_
#define DEBOUNCE_CONF_H_

/*
 * DEBOUNCE_ENABLE_CALLBACKS
 *   Set to 1 to include the transition-callback mechanism.  When 0
 *   (the default), no function-pointer field is added to struct debounce
 *   and no callback-dispatch code is compiled.
 */
#ifndef DEBOUNCE_ENABLE_CALLBACKS
#define DEBOUNCE_ENABLE_CALLBACKS 0
#endif

/*
 * DEBOUNCE_ATOMIC_MODE
 *   Selection of the concurrency model.
 *
 *   DEBOUNCE_ATOMIC_MODE_C11:
 *     Uses standard C11 <stdatomic.h>. Best for modern OS/RTOS.
 *
 *   DEBOUNCE_ATOMIC_MODE_VOLATILE:
 *     Uses 'volatile' for atomicity. Suitable for single-core MCUs
 *     (like TI C2000) where 16/32-bit accesses are atomic.
 *
 *   To select a mode, define the corresponding macro via compiler flag:
 *     -DDEBOUNCE_USE_C11_ATOMIC
 *     -DDEBOUNCE_USE_VOLATILE_ATOMIC
 *
 *   If no flag is provided, the mode is DEBOUNCE_ATOMIC_MODE_C11, or
 *   DEBOUNCE_ATOMIC_MODE_VOLATILE when the compiler defines
 *   __STDC_NO_ATOMICS__ (e.g. TI C2000 codegen).
 *
 *   NOTE: DEBOUNCE_ATOMIC_MODE_C11 requires lock-free 8-bit and 16-bit
 *   atomics and fails to compile without them.  Single-core targets that
 *   provide <stdatomic.h> through library helpers (8-bit and 16-bit cores,
 *   ARMv6-M) must select VOLATILE.
 */
#ifndef DEBOUNCE_ATOMIC_MODE
#if defined(DEBOUNCE_USE_C11_ATOMIC)
#define DEBOUNCE_ATOMIC_MODE 1 /* C11 */
#elif defined(DEBOUNCE_USE_VOLATILE_ATOMIC)
#define DEBOUNCE_ATOMIC_MODE 2 /* VOLATILE */
#elif defined(__STDC_NO_ATOMICS__)
#define DEBOUNCE_ATOMIC_MODE 2 /* VOLATILE: no <stdatomic.h> */
#else
#define DEBOUNCE_ATOMIC_MODE 1 /* C11 Default */
#endif
#endif

/* Mode constants */
#define DEBOUNCE_ATOMIC_MODE_C11      1
#define DEBOUNCE_ATOMIC_MODE_VOLATILE 2

#endif /* DEBOUNCE_CONF_H_ */
