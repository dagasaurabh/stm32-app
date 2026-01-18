#pragma once

#include "gpio.h"

/* User LED (LD2) */
#define LED_PIN			GPIO_PIN_ENCODE(1, 7)	// port B, pin 7

/* User Button (B1) */
#define BUTTON_PIN		GPIO_PIN_ENCODE(2, 13)	// port C, pin 13
