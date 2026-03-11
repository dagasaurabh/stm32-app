#include <stdint.h>
#include "time_source.h"
#include "stm32u5xx_hal.h"

/*
 * STM32U5 time source
 *
 * Implementation:
 *  - SysTick-based
 *  - Resolution: 1 ms
 *  - Monotonic as long as SysTick runs
 */
uint64_t
time_source_now_us(void)
{
    return (uint64_t)HAL_GetTick() * 1000ULL;
}
