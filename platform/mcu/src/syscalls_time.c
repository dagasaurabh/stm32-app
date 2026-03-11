#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/time.h>

#include "cpu_idle.h"
#include "time_source.h"

#define US_PER_S 1000000ULL

/* BSP-defined clock IDs (POSIX-compatible subset) */
#undef CLOCK_REALTIME
#undef CLOCK_MONOTONIC

#define CLOCK_REALTIME ((clockid_t)0)
#define CLOCK_MONOTONIC ((clockid_t)1)

static uint32_t boot_epoch = 0; /* seconds since epoch at boot */

static uint32_t
s_time(time_t *t)
{
    uint32_t now = boot_epoch + (uint32_t)(time_source_now_us() / US_PER_S);

    if (t)
        *t = now;

    return now;
}

time_t
_time(time_t *t)
{
    return (time_t)s_time(t);
}

unsigned int
sleep(unsigned int seconds)
{
    uint64_t start = time_source_now_us();
    uint64_t delay = (uint64_t)seconds * 1000000ULL;

    while ((time_source_now_us() - start) < delay)
    {
        cpu_idle(); /* sleep until interrupt */
    }

    return 0;
}

int
usleep(useconds_t usec)
{
    uint64_t start = time_source_now_us();

    while ((time_source_now_us() - start) < (uint64_t)usec)
    {
        cpu_idle(); /* sleep until interrupt */
    }

    return 0;
}

int
_gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    if (!tv)
    {
        errno = EINVAL;
        return -1;
    }

    uint64_t us = time_source_now_us();
    uint32_t sec = boot_epoch + (uint32_t)(us / US_PER_S);

    tv->tv_sec = sec;
    tv->tv_usec = us % US_PER_S;
    return 0;
}

int
_clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    if (!tp)
    {
        errno = EINVAL;
        return -1;
    }

    uint64_t us = time_source_now_us();
    uint32_t sec = boot_epoch + (uint32_t)(us / US_PER_S);

    switch (clk_id)
    {
        case CLOCK_MONOTONIC:
            tp->tv_sec = us / US_PER_S;
            tp->tv_nsec = (us % US_PER_S) * 1000ULL;
            return 0;
        case CLOCK_REALTIME:
            tp->tv_sec = sec;
            tp->tv_nsec = 0;
            return 0;
        default:
            errno = EINVAL;
            return -1;
    }
}

int
_clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *req, struct timespec *rem)
{
    (void)rem; /* no signal interruption on bare metal */

    if (!req)
    {
        errno = EINVAL;
        return -1;
    }

    /* TIMER_ABSTIME not supported */
    if (flags != 0)
    {
        errno = ENOTSUP;
        return -1;
    }

    /* Convert request to microseconds */
    uint64_t delay_us = (uint64_t)req->tv_sec * 1000000ULL + (uint64_t)req->tv_nsec / 1000ULL;

    uint64_t start_us;

    switch (clock_id)
    {
        case CLOCK_MONOTONIC:
            start_us = time_source_now_us();
            break;
        case CLOCK_REALTIME:
            /* realtime sleep == monotonic delay on bare metal */
            start_us = time_source_now_us();
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    while ((time_source_now_us() - start_us) < delay_us)
    {
        cpu_idle();
    }

    return 0;
}
