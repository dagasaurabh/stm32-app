#include "gpio_hal_if.h"  
#include "stm32l5xx_hal.h"

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

void gpio_hal_init(void * port, uint32_t pin, gpio_hal_mode_t mode, gpio_hal_pull_t pull)
{
	GPIO_TypeDef * __port = (GPIO_TypeDef *)port;
	GPIO_InitTypeDef cfg = {
		.Pin = pin,
		.Pull = pull,
		.Mode = mode,
		.Speed = GPIO_SPEED_FREQ_LOW,
	};

	HAL_GPIO_Init(__port, &cfg);
}

void gpio_hal_write(void * port, uint32_t pin, uint8_t level)
{
	GPIO_TypeDef * __port = (GPIO_TypeDef *)port;

	HAL_GPIO_WritePin(__port, pin, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint8_t gpio_hal_read(void *port, uint32_t pin)
{
	GPIO_TypeDef * __port = (GPIO_TypeDef *)port;

	return HAL_GPIO_ReadPin(__port, pin) == GPIO_PIN_SET;
}

void gpio_hal_toggle(void * port, uint32_t pin)
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
}


void gpio_hal_irq_config(void * port,
		uint32_t pin,
		gpio_hal_irq_edge_t edge,
		gpio_hal_irq_cb_t cb,
		void *ctx)
{
	GPIO_TypeDef * __port = (GPIO_TypeDef *)port;
	uint32_t line = __builtin_ctz(pin);

	hal_exti_slots[line].cb  = cb;
    hal_exti_slots[line].ctx = ctx;

	uint32_t mode;
	EXTI_CallbackIDTypeDef cbIdType;

#define mycase(__edge, __mode) \
	case GPIO_HAL_IRQ_EDGE_##__edge: \
	mode = GPIO_MODE_IT_##__mode; \
	break;
	switch(edge)
	{
		mycase(RISING, RISING);
		mycase(FALLING, FALLING);
		mycase(BOTH, RISING_FALLING);
	}
#undef mycase

#define mycase(__edge, __mode) \
	case GPIO_HAL_IRQ_EDGE_##__edge: \
	cbIdType = HAL_EXTI_##__mode##_CB_ID; \
	break;
	
	switch(edge)
	{
		mycase(RISING, FALLING);
		mycase(FALLING, FALLING);
		mycase(BOTH, COMMON);
	}
#undef mycase

    GPIO_InitTypeDef cfg = {
        .Pin  = pin,
        .Pull = GPIO_NOPULL,
		.Speed = GPIO_SPEED_FREQ_HIGH,
        .Mode = mode,
    };

    HAL_GPIO_Init(__port, &cfg);

	/* Get EXTI handle */
    HAL_EXTI_GetHandle(&hal_exti_slots[line].hexti,
			exti_line_to_hal(line));

    /* Register EXTI callback */
    HAL_EXTI_RegisterCallback(&hal_exti_slots[line].hexti, cbIdType, NULL);
}

static IRQn_Type exti_line_to_irq(uint32_t line)
{
#if defined(STM32L5)
    /* STM32L5: EXTI0_IRQn ... EXTI15_IRQn are contiguous */
    return (IRQn_Type)(EXTI0_IRQn + line);
#else
#error "EXTI IRQ mapping not implemented for this STM32 family"
#endif
}


void gpio_hal_irq_enable(void *port, uint32_t pin)
{
	(void)port;
	uint32_t line = __builtin_ctz(pin);
	IRQn_Type irq = exti_line_to_irq(line);

	HAL_NVIC_SetPriority(irq, 10, 0);
	HAL_NVIC_EnableIRQ(irq);
}

void gpio_hal_irq_disable(void *port, uint32_t pin)
{
	uint32_t line = __builtin_ctz(pin);
	HAL_NVIC_DisableIRQ(exti_line_to_irq(line));
}
