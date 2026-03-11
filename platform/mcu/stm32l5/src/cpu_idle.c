#include "cpu_idle.h"
#include "stm32l5xx_hal.h"

void
cpu_idle(void)
{
    __WFI();
}
