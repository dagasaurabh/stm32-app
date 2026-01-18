#include "gpio_driver.h"
#include "gpio_hal_if.h"
#include <assert.h>

typedef struct {
    gpio_port_t port;
    uint32_t pin;
    gpio_drv_irq_cb_t cb;
    void *ctx;
} gpio_irq_slot_t;

static gpio_irq_slot_t irq_slots[16];

void gpio_drv_init_pin(gpio_port_t port,
		uint32_t pin,
		gpio_drv_mode_t mode,
		gpio_drv_pull_t pull)
{
	assert(port != NULL);
	assert(pin != 0);
	gpio_hal_mode_t hal_mode;
	gpio_hal_pull_t hal_pull;

#define mycase(__in, __out) \
	case GPIO_DRV_MODE_##__in: \
	hal_mode = GPIO_HAL_MODE_##__out; \
	break;
	switch(mode) {
		mycase(INPUT, INPUT);
		mycase(OUTPUT, OUTPUT);
		mycase(AF, AF);
		mycase(ANALOG, ANALOG);
	}
#undef mycase

#define mycase(__in, __out) \
	case GPIO_DRV_PULL_##__in: \
	hal_pull = GPIO_HAL_PULL_##__out; \
	break;
	switch(pull) {
		mycase(NONE, NONE);
		mycase(UP, UP);
		mycase(DOWN, DOWN);
	}
#undef mycase

	gpio_hal_init(port, pin, hal_mode, hal_pull);
}

void gpio_drv_write(gpio_port_t port, uint32_t pin, uint8_t level)
{
	assert(port != NULL);
	assert(pin != 0);
	gpio_hal_write(port, pin, level);
}

uint8_t gpio_drv_read(gpio_port_t port, uint32_t pin)
{
	assert(port != NULL);
	assert(pin != 0);
	return gpio_hal_read(port, pin);
}

void gpio_drv_toggle(gpio_port_t port, uint32_t pin)
{
	assert(port != NULL);
	assert(pin != 0);
	gpio_hal_toggle(port, pin);
}

static void gpio_drv_irq_cb(void *ctx)
{
    gpio_irq_slot_t *slot = ctx;

    if(slot->cb) slot->cb(slot->port, slot->pin, slot->ctx);
}

int gpio_drv_irq_register(gpio_port_t port,
		uint32_t pin,
		gpio_drv_irq_edge_t edge,
		gpio_drv_irq_cb_t cb,
		void *ctx)
{
	assert(port != NULL);
	assert(pin != 0);

    uint32_t line = __builtin_ctz(pin);

    if(irq_slots[line].cb) return -1;

    irq_slots[line] = (gpio_irq_slot_t){
        .port = port,
        .pin  = pin,
        .cb   = cb,
        .ctx  = ctx,
    };

	gpio_hal_irq_edge_t hal_edge;

#define mycase(__in, __out) \
	case GPIO_DRV_IRQ_EDGE_##__in: \
	hal_edge = GPIO_HAL_IRQ_EDGE_##__out; \
	break;
	switch(edge) {
		mycase(RISING, RISING);
		mycase(FALLING, FALLING);
		mycase(BOTH, BOTH);
	}
#undef mycase

    gpio_hal_irq_config(port,
			pin,
			hal_edge,
			gpio_drv_irq_cb,
			&irq_slots[line]);
    return 0;
}

void gpio_drv_irq_enable(gpio_port_t port, uint32_t pin)
{
	assert(port != NULL);
	assert(pin != 0);

    gpio_hal_irq_enable(port, pin);
}

void gpio_drv_irq_disable(gpio_port_t port, uint32_t pin)
{
	assert(port != NULL);
	assert(pin != 0);

    gpio_hal_irq_disable(port, pin);
}
