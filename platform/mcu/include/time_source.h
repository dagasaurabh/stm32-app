#pragma once
#include <stdint.h>

/*
 * MCU-provided monotonic time source.
 *
 * Contract:
 *  - Monotonic (never goes backward)
 *  - Starts at 0 on boot
 *  - ISR-safe (read-only)
 *  - Resolution is implementation-defined
 *
 * Ownership:
 *  - Implemented by MCU family code.
 *  - Consumed by syscalls and common services
 */
uint64_t
time_source_now_us(void);
