#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    int64_t tv_sec;
    int32_t tv_usec;
} time_val_t;

void
timeradd(const time_val_t *a, const time_val_t *b, time_val_t *res);

void
timersub(const time_val_t *a, const time_val_t *b, time_val_t *res);

/*
 * Compare two time values
 * Returns:
 *   <0 if a < b
 *    0 if a == b
 *   >0 if a > b
 */
int
timercmp(const time_val_t *a, const time_val_t *b);
