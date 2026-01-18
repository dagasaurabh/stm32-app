#pragma once

/*
 * Enter a low-power idle state until an interrupt occurs.
 *
 * Contract:
 *  - Must return after any interrupt
 *  - May enter low-power state
 *  - Safe to call repeatedly
 *
 * Ownership:
 *  - Implemented by MCU family code
 *  - Used by syscalls and common services
 */
void cpu_idle(void);

