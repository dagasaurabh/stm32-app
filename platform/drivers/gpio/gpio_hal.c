#include "gpio_hal_if.h"
#if defined(STM32L552xx)
 #include "stm32l5xx_hal.h"
#elif defined(STM32U585xx)
 #include "stm32u5xx_hal.h"
#else
 #error "Include STM32 HAL header file"
#endif

typedef struct {
    gpio_hal_irq_cb_t cb;
    void *ctx;
    EXTI_HandleTypeDef hexti;
} hal_exti_slot_t;

static hal_exti_slot_t hal_exti_slots[16];

#define DEFINE_EXTI_IRQ_HANDLER(line) \
void EXTI##line##_IRQHandler(void) \
{ \
    HAL_EXTI_IRQHandler(&hal_exti_slots[line].hexti); \
    if (hal_exti_slots[line].cb) \
        hal_exti_slots[line].cb(hal_exti_slots[line].ctx); \
}

DEFINE_EXTI_IRQ_HANDLER(0)
DEFINE_EXTI_IRQ_HANDLER(1)
DEFINE_EXTI_IRQ_HANDLER(2)
DEFINE_EXTI_IRQ_HANDLER(3)
DEFINE_EXTI_IRQ_HANDLER(4)
DEFINE_EXTI_IRQ_HANDLER(5)
DEFINE_EXTI_IRQ_HANDLER(6)
DEFINE_EXTI_IRQ_HANDLER(7)
DEFINE_EXTI_IRQ_HANDLER(8)
DEFINE_EXTI_IRQ_HANDLER(9)
DEFINE_EXTI_IRQ_HANDLER(10)
DEFINE_EXTI_IRQ_HANDLER(11)
DEFINE_EXTI_IRQ_HANDLER(12)
DEFINE_EXTI_IRQ_HANDLER(13)
DEFINE_EXTI_IRQ_HANDLER(14)
DEFINE_EXTI_IRQ_HANDLER(15)

void gpio_hal_init(void *port, uint32_t pin,
		gpio_hal_mode_t mode,
		gpio_pull_t pull,
		gpio_hal_speed_t speed)
{
	GPIO_TypeDef * __port = (GPIO_TypeDef *)port;
	uint32_t gpio_mode;

#define mycase(__in__, __out__) \
	case GPIO_HAL_MODE_##__in__: \
	gpio_mode = GPIO_MODE_##__out__; \
	break;

	switch(mode) {
		mycase(INPUT,     INPUT);
		mycase(OUTPUT,    OUTPUT_PP);
		mycase(OUTPUT_OD, OUTPUT_OD);
		mycase(AF,        AF_PP);
		mycase(AF_OD,     AF_OD);
		mycase(ANALOG,    ANALOG);
	}
#undef mycase

	uint32_t gpio_speed;

#define mycase(__in__, __out__) \
	case GPIO_HAL_SPEED_##__in__: \
	gpio_speed = GPIO_SPEED_FREQ_##__out__; \
	break;

	switch(speed) {
		mycase(LOW,       LOW);
		mycase(MEDIUM,    MEDIUM);
		mycase(HIGH,      HIGH);
		mycase(VERY_HIGH, VERY_HIGH);
	}
#undef mycase

	GPIO_InitTypeDef cfg = {
		.Pin   = pin,
		.Pull  = pull,
		.Mode  = gpio_mode,
		.Speed = gpio_speed,
	};

	HAL_GPIO_Init(__port, &cfg);
}

void gpio_hal_write(void *port, uint32_t pin, uint8_t level)
{
    GPIO_TypeDef * __port = (GPIO_TypeDef *)port;

    HAL_GPIO_WritePin(__port, pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t gpio_hal_read(void *port, uint32_t pin)
{
    GPIO_TypeDef * __port = (GPIO_TypeDef *)port;

    return HAL_GPIO_ReadPin(__port, pin) == GPIO_PIN_SET;
}

void gpio_hal_toggle(void *port, uint32_t pin)
{
    GPIO_TypeDef * __port = (GPIO_TypeDef *)port;

    HAL_GPIO_TogglePin(__port, pin);
}

static uint32_t exti_line_to_hal(uint32_t line)
{
#define mycase(__int) \
	case __int: \
		return EXTI_LINE_##__int;
	switch (line) {
		mycase(0)
		mycase(1)
		mycase(2)
		mycase(3)
		mycase(4)
		mycase(5)
		mycase(6)
		mycase(7)
		mycase(8)
		mycase(9)
		mycase(10)
		mycase(11)
		mycase(12)
		mycase(13)
		mycase(14)
		mycase(15)
	}
#undef mycase
    return EXTI_LINE_0;
}


void gpio_hal_irq_config(void *port,
		uint32_t pin,
		gpio_irq_edge_t edge,
		gpio_hal_irq_cb_t cb,
		void *ctx)
{
	GPIO_TypeDef * __port = (GPIO_TypeDef *)port;

	if (__port == NULL || pin == 0U || (pin & (pin - 1U)) != 0U) return;

	uint32_t line = __builtin_ctz(pin);

	hal_exti_slots[line].cb  = cb;
    hal_exti_slots[line].ctx = ctx;

	uint32_t mode;

#define mycase(__edge, __mode) \
	case GPIO_IRQ_EDGE_##__edge: \
	mode = GPIO_MODE_IT_##__mode; \
	break;
	switch(edge)
	{
		mycase(RISING,  RISING);
		mycase(FALLING, FALLING);
		mycase(BOTH,    RISING_FALLING);
	}
#undef mycase

    /* Preserve the pull config that was set by gpio_hal_init().
     * PUPDR encoding: 0=no-pull, 1=pull-up, 2=pull-down — identical to
     * GPIO_NOPULL / GPIO_PULLUP / GPIO_PULLDOWN HAL constants. */
    uint32_t pin_pos    = __builtin_ctz(pin);
    uint32_t saved_pull = (__port->PUPDR >> (2U * pin_pos)) & 0x3U;

    GPIO_InitTypeDef cfg = {
        .Pin   = pin,
        .Pull  = saved_pull,
        .Speed = GPIO_SPEED_FREQ_LOW,   /* irrelevant for digital input */
        .Mode  = mode,
    };

    HAL_GPIO_Init(__port, &cfg);

    HAL_EXTI_GetHandle(&hal_exti_slots[line].hexti,
            exti_line_to_hal(line));
}

static IRQn_Type exti_line_to_irq(uint32_t line)
{
#if defined(STM32L5) || defined(STM32U5)
    /* STM32L5 and STM32U5: EXTI0_IRQn ... EXTI15_IRQn are contiguous */
    return (IRQn_Type)(EXTI0_IRQn + line);
#else
#error "EXTI IRQ mapping not implemented for this STM32 family"
#endif
}


void gpio_hal_irq_enable(void *port, uint32_t pin)
{
    (void)port;

    if (pin == 0U || (pin & (pin - 1U)) != 0U) return;

    uint32_t line = __builtin_ctz(pin);
    IRQn_Type irq = exti_line_to_irq(line);

    HAL_NVIC_SetPriority(irq, 10, 0);
    HAL_NVIC_EnableIRQ(irq);
}

void gpio_hal_irq_disable(void *port, uint32_t pin)
{
    (void)port;

    if (pin == 0U || (pin & (pin - 1U)) != 0U) return;

    uint32_t line = __builtin_ctz(pin);
    HAL_NVIC_DisableIRQ(exti_line_to_irq(line));
}
