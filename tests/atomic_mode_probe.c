/**
 * SPDX-License-Identifier: MIT
 *
 * @file: atomic_mode_probe.c
 *
 * @brief
 *    Translation unit for the configure-time atomicity checks in
 *    tests/meson.build. It compiles only if debounce.h accepts the target
 *    and, when DEBOUNCE_PROBE_EXPECT_MODE or DEBOUNCE_PROBE_EXPECT_CALLBACKS
 *    is defined, selects that mode or callback setting.
 */

#include "debounce.h"

#ifdef DEBOUNCE_PROBE_EXPECT_MODE
_Static_assert(DEBOUNCE_ATOMIC_MODE == DEBOUNCE_PROBE_EXPECT_MODE,
               "debounce: unexpected atomicity mode");
#endif

#ifdef DEBOUNCE_PROBE_EXPECT_CALLBACKS
_Static_assert(DEBOUNCE_ENABLE_CALLBACKS == DEBOUNCE_PROBE_EXPECT_CALLBACKS,
               "debounce: unexpected DEBOUNCE_ENABLE_CALLBACKS");
#endif

struct debounce debounce_probe_object;
