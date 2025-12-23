#pragma once

#include "stm32l5xx_hal.h"

/* User LED (LD2) */
#define LED_GPIO_PORT   GPIOB
#define LED_GPIO_PIN    GPIO_PIN_7

/* User Button (B1) */
#define BUTTON_GPIO_PORT GPIOC
#define BUTTON_GPIO_PIN  GPIO_PIN_13
