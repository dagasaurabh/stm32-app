#include <stdint.h>
#include "time_source.h"

#define US_PER_MS 1000

uint64_t time_monotonic_ms(void)
{
    return time_source_now_us() / US_PER_MS;
}

uint64_t time_monotonic_us(void)
{
    return time_source_now_us();
}
