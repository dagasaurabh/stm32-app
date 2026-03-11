#include <stdint.h>
#include "time_math.h"

#define USEC_PER_SEC 1000000

static void
normalize(time_val_t *t)
{
    if (t->tv_usec >= USEC_PER_SEC)
    {
        t->tv_sec += t->tv_usec / USEC_PER_SEC;
        t->tv_usec %= USEC_PER_SEC;
    }
    else if (t->tv_usec < 0)
    {
        int64_t borrow = (-t->tv_usec + USEC_PER_SEC - 1) / USEC_PER_SEC;
        t->tv_sec -= borrow;
        t->tv_usec += borrow * USEC_PER_SEC;
    }
}

void
timeradd(const time_val_t *a, const time_val_t *b, time_val_t *res)
{
    res->tv_sec = a->tv_sec + b->tv_sec;
    res->tv_usec = a->tv_usec + b->tv_usec;
    normalize(res);
}

void
timersub(const time_val_t *a, const time_val_t *b, time_val_t *res)
{
    res->tv_sec = a->tv_sec - b->tv_sec;
    res->tv_usec = a->tv_usec - b->tv_usec;
    normalize(res);
}

int
timercmp(const time_val_t *a, const time_val_t *b)
{
    if (a->tv_sec != b->tv_sec)
        return (a->tv_sec > b->tv_sec) ? 1 : -1;

    if (a->tv_usec != b->tv_usec)
        return (a->tv_usec > b->tv_usec) ? 1 : -1;

    return 0;
}
