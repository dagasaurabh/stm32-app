#pragma once

#include <stdint.h>
#include <stdio.h>

/*
 * pfmt.h — integer-based float printing helpers.
 *
 * newlib-nano's %f/%lf promotes float→double and pulls in _dtoa_r plus all
 * __aeabi_d* double soft-FP routines (~14 KB).  These helpers avoid that by
 * doing the float→integer conversion in hardware (FPU handles float * scale)
 * and printing only integer parts with %ld.
 *
 * pf1(v)  — 1 decimal place:  e.g.  23.4
 * pf2(v)  — 2 decimal places: e.g.  23.45
 * pf3(v)  — 3 decimal places: e.g.  23.456
 */

static inline void
pf1(float v)
{
    int32_t x = (int32_t)(v * 10.0f);
    if (x < 0)
        printf("-%ld.%ld", (long)(-x / 10), (long)(-x % 10));
    else
        printf("%ld.%ld", (long)(x / 10), (long)(x % 10));
}

static inline void
pf2(float v)
{
    int32_t x = (int32_t)(v * 100.0f);
    if (x < 0)
        printf("-%ld.%02ld", (long)(-x / 100), (long)(-x % 100));
    else
        printf("%ld.%02ld", (long)(x / 100), (long)(x % 100));
}

static inline void
pf3(float v)
{
    int32_t x = (int32_t)(v * 1000.0f);
    if (x < 0)
        printf("-%ld.%03ld", (long)(-x / 1000), (long)(-x % 1000));
    else
        printf("%ld.%03ld", (long)(x / 1000), (long)(x % 1000));
}
