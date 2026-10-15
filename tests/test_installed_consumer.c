/*
 * SPDX-License-Identifier: MIT
 *
 * @file test_installed_consumer.c
 * @brief Consumer built against the installed headers and pkg-config package.
 */

#include <stdint.h>

#if __has_include(<debounce/debounce.h>)
#include <debounce/debounce.h>
#else
#include "debounce.h"
#endif

#if __has_include(<debounce/debounce_version.h>)
#include <debounce/debounce_version.h>
#else
#include "debounce_version.h"
#endif

#if DEBOUNCE_VERSION_NUMBER                                                    \
    != ((DEBOUNCE_VERSION_MAJOR * 10000L) + (DEBOUNCE_VERSION_MINOR * 100L)    \
        + DEBOUNCE_VERSION_PATCH)
#error "DEBOUNCE_VERSION_NUMBER does not match its parts"
#endif

int
main(void)
{
        struct debounce db;

        if (!debounce_init(&db, (uint16_t)(DEBOUNCE_VERSION_MINOR + 1))) {
                return 1;
        }

        return debounce_get_trip(&db) == (uint16_t)(DEBOUNCE_VERSION_MINOR + 1)
                   ? 0
                   : 2;
}
