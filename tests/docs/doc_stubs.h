/*
 * SPDX-License-Identifier: MIT
 *
 * @file doc_stubs.h
 * @brief Declarations-only stubs for the documentation examples.
 *
 * The examples in README.md and the guides under docs/ call application code
 * that the library does not provide. This header declares just enough of it
 * for those examples to type-check in the "debounce docs examples" test.
 * Nothing here is defined or linked; it exists so a signature drift in the
 * documentation fails the build. See CONTRIBUTING.md.
 */

#ifndef DEBOUNCE_DOC_STUBS_H
#define DEBOUNCE_DOC_STUBS_H

#include <stdbool.h>

void configuration_fault(void);
void set_warning_led(bool on);
void log_fault_event(void);
bool read_button_pin(void);
void on_press(void);
void on_release(void);

#endif /* DEBOUNCE_DOC_STUBS_H */
