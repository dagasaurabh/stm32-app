#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "gpio.h"
#include "pinmap.h"
#include "board_api.h"
#include "systime.h"

static volatile bool button_pressed;
static volatile uint64_t last_press_ms;

#define DEBOUNCE_MS 50

/* ISR callback (runs in interrupt context) */
static void button_irq_cb(gpio_pin_t pin, void *ctx)
{
    /* Minimal ISR work*/
	uint64_t now = time_monotonic_ms();

	if ((now - last_press_ms) < DEBOUNCE_MS) return;
	last_press_ms = now;
	button_pressed = true;
}

int main(void)
{
	board_init();

	gpio_init(LED_PIN, GPIO_MODE_OUTPUT, GPIO_PULL_NONE, GPIO_SPEED_LOW);
	gpio_write(LED_PIN, GPIO_LOW);

	/* Button input */
	gpio_init(BUTTON_PIN, GPIO_MODE_INPUT, GPIO_PULL_NONE, GPIO_SPEED_LOW);

	/* Register EXTI interrupt */
	gpio_irq_register(
			BUTTON_PIN,
			GPIO_IRQ_EDGE_RISING,
			button_irq_cb,
			NULL
			);

	gpio_irq_enable(BUTTON_PIN);

	while (1) {
		if(button_pressed) {
			button_pressed = false;
			gpio_toggle(LED_PIN);
		}
		__asm volatile("nop");
	}
}
