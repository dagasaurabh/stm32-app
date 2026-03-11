#pragma once

#include "gpio.h"

/* User LED (LD1) */
#define USER_LED_PIN_1 GPIO_PIN_ENCODE(2, 7) // Port AC, pin 7

/* User LED (LD2) */
#define USER_LED_PIN_2 GPIO_PIN_ENCODE(1, 7) // port B, pin 7

/* User LED (LD3) */
#define USER_LED_PIN_3 GPIO_PIN_ENCODE(0, 9) // Port A, pin 9

/* User Button (B1) */
#define BUTTON_PIN GPIO_PIN_ENCODE(2, 13) // port C, pin 13

#define GREEN_LED USER_LED_PIN_1
#define BLUE_LED USER_LED_PIN_2
#define RED_LED USER_LED_PIN_3

#define LED_PIN BLUE_LED

/* I2C1 pins */
/* SCL = PB8 (AF4), SDA = PB9 (AF4) — configured in HAL_I2C_MspInit */

/* SPI1 pins */
/* SCK = PA5 (AF5), MISO = PA6 (AF5), MOSI = PA7 (AF5) — configured in HAL_SPI_MspInit */
#define SPI1_CS_PIN GPIO_PIN_ENCODE(0, 4) // port A, pin 4 (PA4)
