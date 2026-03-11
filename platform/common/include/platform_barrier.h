#pragma once

#include "cmsis_compiler.h"   /* __DMB, __get_PRIMASK, __set_PRIMASK, __disable_irq,
                                * __get_IPSR — Cortex-M CMSIS intrinsics */

/*
 * platform_barrier.h — portable concurrency primitives for Cortex-M firmware.
 *
 * Centralises all CMSIS intrinsic usage so that platform/common facade code
 * (i2c.c, spi.c) and application code never include cmsis_compiler.h or
 * cmsis_gcc.h directly.  Porting to a non-CMSIS toolchain only requires
 * updating this one header.
 *
 * Three groups of primitives are provided:
 *
 *   1. Memory barriers      — ISR-to-main shared data ordering
 *   2. Critical sections    — save/restore interrupt enable state (PRIMASK)
 *   3. ISR context detection — determine if running inside an interrupt handler
 */

/* =========================================================================
 * 1. Memory barriers
 * =========================================================================
 *
 * platform_barrier.h — portable memory barrier wrappers for ISR-to-main
 * data sharing on Cortex-M.
 *
 * Problem context
 * ---------------
 * When an ISR writes a set of data fields and a flag, and the main loop reads
 * the flag then reads the data, two hazards exist:
 *
 *   1. Compiler reordering: the compiler may reorder stores/loads to different
 *      objects even when they are declared volatile (volatile only prevents
 *      reordering of accesses to the *same* object).
 *
 *   2. Hardware reordering: Cortex-M3/M4/M33 have an in-order pipeline, so
 *      hardware reordering is not a concern in practice.  The macros below
 *      still emit __DMB() to be correct on any future port to an out-of-order
 *      core (Cortex-A) and to communicate intent clearly.
 *
 * Usage pattern (ISR side — writer):
 *
 *   g_sample = *(const my_data_t *)data;   // write data first
 *   PLATFORM_RELEASE_STORE();              // barrier: all stores above this
 *   g_ready = true;                        // set flag last
 *
 * Usage pattern (main loop — reader):
 *
 *   if (g_ready) {
 *       g_ready = false;
 *       PLATFORM_ACQUIRE_LOAD();           // barrier: flag cleared before data read
 *       use(g_sample);                     // read data after barrier
 *   }
 *
 * Why the barrier is placed where it is
 * --------------------------------------
 * PLATFORM_RELEASE_STORE() ensures that all memory writes before it (the data
 * struct copy) are globally visible before the flag write that follows.
 *
 * PLATFORM_ACQUIRE_LOAD() ensures that the flag write (g_ready = false) is
 * visible to the ISR before the main loop reads the data, preventing a new ISR
 * from overwriting g_sample while the main loop is reading it on a system
 * where re-arming could happen from a higher-priority context.
 *
 * On Cortex-M33 with sensor_mgr, the read_pending flag prevents re-arming
 * until after the main loop calls sensor_mgr_poll(), so the race cannot
 * actually occur in current firmware.  The barriers are still present to
 * make the intent explicit and to remain correct if the execution model changes.
 */

/* Insert AFTER writing all data fields, BEFORE setting the ready flag (ISR) */
#define PLATFORM_RELEASE_STORE()  __DMB()

/* Insert AFTER clearing the ready flag, BEFORE reading data fields (main loop) */
#define PLATFORM_ACQUIRE_LOAD()   __DMB()

/* =========================================================================
 * 2. Critical sections — save/restore PRIMASK
 * =========================================================================
 *
 * Saves the current interrupt-enable state (PRIMASK register), disables all
 * maskable interrupts, executes the critical section, then restores the
 * original state.  Nesting is safe: if interrupts were already disabled
 * before PLATFORM_IRQ_SAVE, they remain disabled after PLATFORM_IRQ_RESTORE.
 *
 * Usage:
 *
 *   uint32_t irq_state;
 *   PLATFORM_IRQ_SAVE(irq_state);
 *   // ... critical section ...
 *   PLATFORM_IRQ_RESTORE(irq_state);
 *
 * Important: the variable passed to PLATFORM_IRQ_SAVE must be declared before
 * the macro is invoked.  Do not use these macros across function boundaries.
 */
#define PLATFORM_IRQ_SAVE(state) \
    do { (state) = __get_PRIMASK(); __disable_irq(); } while (0)

#define PLATFORM_IRQ_RESTORE(state) \
    __set_PRIMASK(state)

/* =========================================================================
 * 3. ISR context detection
 * =========================================================================
 *
 * Returns non-zero (true) when called from within an interrupt handler,
 * zero (false) when called from thread (main loop) context.
 *
 * On Cortex-M, the IPSR (Interrupt Program Status Register) holds the
 * exception number of the currently active exception.  It is zero in thread
 * mode and non-zero in any exception (IRQ, SysTick, fault, etc.).
 *
 * Usage:
 *
 *   if (PLATFORM_IN_ISR()) return -1;   // reject call from interrupt context
 */
#define PLATFORM_IN_ISR()   (__get_IPSR() != 0u)
