#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "timer.h"
#include "systime.h"

void mstimer_start(mstimer_t *t, uint32_t timeout_ms)
{
    t->expiry_ms = time_monotonic_ms() + timeout_ms;
}

bool mstimer_expired(const mstimer_t *t)
{
    return (int64_t)(time_monotonic_ms() - t->expiry_ms) >= 0;
}

uint64_t mstimer_remaining(const mstimer_t *t)
{
    int64_t rem = (int64_t)(t->expiry_ms - time_monotonic_ms());
    return (rem > 0) ? (uint64_t)rem : 0;
}

void sectimer_start(sectimer_t *t, uint32_t timeout_sec)
{
    t->expiry_sec = time(NULL) + timeout_sec;
}

bool sectimer_expired(const sectimer_t *t)
{
    return (int64_t)(time(NULL) - t->expiry_sec) >= 0;
}

uint32_t sectimer_remaining(const sectimer_t *t)
{
    int64_t rem = (int64_t)(t->expiry_sec - time(NULL));
    return (rem > 0) ? (uint32_t)rem : 0;
}
