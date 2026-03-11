#pragma once

#include "gpio.h"

/* User LED RED (LED6) — PH6 */
#define USER_LED_PIN_1 GPIO_PIN_ENCODE(7, 6)

/* User LED GREEN (LED7) — PH7 */
#define USER_LED_PIN_2 GPIO_PIN_ENCODE(7, 7)

/* User Button (B1) — PC13 */
#define BUTTON_PIN GPIO_PIN_ENCODE(2, 13)

#define RED_LED USER_LED_PIN_1
#define GREEN_LED USER_LED_PIN_2
#define LED_PIN GREEN_LED

/* USART1 pins (ST-LINK VCP) */
/* TX = PA9 (AF7), RX = PA10 (AF7) — configured in HAL_UART_MspInit */

/* SPI1 pins */
/* SCK = PA5 (AF5), MISO = PA6 (AF5), MOSI = PA7 (AF5) — configured in HAL_SPI_MspInit */
#define SPI1_CS_PIN GPIO_PIN_ENCODE(0, 4) /* PA4 */

/* I2C1 pins (on-board sensors) */
/* SCL = PB8 (AF4), SDA = PB9 (AF4) — configured in HAL_I2C_MspInit */
