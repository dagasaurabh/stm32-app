#include "gpio.h"
#include "board.h"
#include "gpio_driver.h"
#include "stm32u5xx_hal.h"

static GPIO_TypeDef *port_table[] = {
    GPIOA,
    GPIOB,
    GPIOC,
    GPIOD,
    GPIOE,
    GPIOF,
    GPIOG,
    GPIOH,
    GPIOI,
};

#define BOARD_PORT_COUNT (sizeof(port_table) / sizeof(port_table[0]))

static GPIO_TypeDef *board_get_port(gpio_pin_t pin)
{
    uint8_t port_idx = gpio_get_bank(pin);
    if (port_idx >= BOARD_PORT_COUNT) {
        return NULL;
    }

    return port_table[port_idx];
}

typedef struct {
    gpio_pin_t    pin;
    gpio_irq_cb_t cb;
    void         *ctx;
} board_irq_slot_t;

static board_irq_slot_t board_irq_slots[16];

static void board_gpio_init(gpio_pin_t pin, gpio_mode_t mode, gpio_pull_t pp, gpio_speed_t speed)
{
    GPIO_TypeDef *port    = board_get_port(pin);
    uint16_t      pin_num = gpio_pin_mask(pin);

    if (port == NULL || pin_num == 0U) {
        return;
    }

    gpio_drv_init_pin((gpio_port_t)port, pin_num, mode, pp, speed);
}

static void board_gpio_write(gpio_pin_t pin, gpio_level_t level)
{
    GPIO_TypeDef *port    = board_get_port(pin);
    uint16_t      pin_num = gpio_pin_mask(pin);

    if (port == NULL || pin_num == 0U) {
        return;
    }

    gpio_drv_write((gpio_port_t)port, pin_num, level == GPIO_HIGH);
}

static gpio_level_t board_gpio_read(gpio_pin_t pin)
{
    GPIO_TypeDef *port    = board_get_port(pin);
    uint16_t      pin_num = gpio_pin_mask(pin);

    if (port == NULL || pin_num == 0U) {
        return GPIO_LOW;
    }

    return gpio_drv_read((gpio_port_t)port, pin_num) ? GPIO_HIGH : GPIO_LOW;
}

static void board_gpio_toggle(gpio_pin_t pin)
{
    GPIO_TypeDef *port    = board_get_port(pin);
    uint16_t      pin_num = gpio_pin_mask(pin);

    if (port == NULL || pin_num == 0U) {
        return;
    }

    gpio_drv_toggle((gpio_port_t)port, pin_num);
}

static void board_exti_irq_cb(void *ctx)
{
    uint32_t line = (uint32_t)(uintptr_t)ctx;

    if (board_irq_slots[line].cb) {
        board_irq_slots[line].cb(board_irq_slots[line].pin, board_irq_slots[line].ctx);
    }
}

static int board_gpio_irq_register(gpio_pin_t pin,
        gpio_irq_edge_t edge,
        gpio_irq_cb_t cb,
        void *ctx)
{
    GPIO_TypeDef    *port    = board_get_port(pin);
    uint16_t         pin_num = gpio_pin_mask(pin);

    if (port == NULL || pin_num == 0U) {
        return -1;
    }

    uint32_t line = __builtin_ctz(pin_num);

    if (board_irq_slots[line].cb) return -1;

    board_irq_slots[line] = (board_irq_slot_t){
        .pin = pin,
        .cb  = cb,
        .ctx = ctx,
    };

    return gpio_drv_irq_register(
            port,
            pin_num,
            edge,
            board_exti_irq_cb,
            (void *)(uintptr_t)line);
}

static void board_gpio_irq_enable(gpio_pin_t pin)
{
    GPIO_TypeDef *port    = board_get_port(pin);
    uint16_t      pin_num = gpio_pin_mask(pin);

    if (port == NULL || pin_num == 0U) {
        return;
    }

    gpio_drv_irq_enable(port, pin_num);
}

static void board_gpio_irq_disable(gpio_pin_t pin)
{
    GPIO_TypeDef *port    = board_get_port(pin);
    uint16_t      pin_num = gpio_pin_mask(pin);

    if (port == NULL || pin_num == 0U) {
        return;
    }

    gpio_drv_irq_disable(port, pin_num);
}

static const struct gpio_ops __gpio_ops = {
    .init         = board_gpio_init,
    .write        = board_gpio_write,
    .read         = board_gpio_read,
    .toggle       = board_gpio_toggle,
    .irq_register = board_gpio_irq_register,
    .irq_enable   = board_gpio_irq_enable,
    .irq_disable  = board_gpio_irq_disable,
};

/*
 * SystemClock_Config — bring SYSCLK to 160 MHz via PLL.
 *
 * Must be called once from board_init(), immediately after HAL_Init().
 * Without it the MCU runs at the 4 MHz MSI default.
 *
 * Why this matters for peripherals:
 *   - I2C : TIMINGR is a hardcoded constant calculated for 160 MHz PCLK.
 *           At 4 MHz those values produce a completely wrong SCL period
 *           and all transfers fail.
 *   - SPI : self-synchronous (master drives SCK), so it tolerates any
 *           clock but runs much slower than intended.
 *   - UART: HAL_UART_Init() queries the actual PCLK at runtime and
 *           auto-computes BRR, so baud rate stays correct regardless.
 *
 * This function is intentionally NOT in hal_msp.c. hal_msp.c is reserved
 * for HAL_XXX_MspInit() peripheral callbacks invoked from within HAL init
 * functions. SystemClock_Config() is system-level startup code, not a
 * peripheral callback.
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Voltage scaling required for 160 MHz operation */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
        while (1) {}

    /* MSI 4 MHz -> PLL1 (x80 /2) -> 160 MHz SYSCLK */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_4;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLMBOOST       = RCC_PLLMBOOST_DIV1;
    RCC_OscInitStruct.PLL.PLLM            = 1;
    RCC_OscInitStruct.PLL.PLLN            = 80;
    RCC_OscInitStruct.PLL.PLLP            = 2;
    RCC_OscInitStruct.PLL.PLLQ            = 2;
    RCC_OscInitStruct.PLL.PLLR            = 2;
    RCC_OscInitStruct.PLL.PLLRGE          = RCC_PLLVCIRANGE_0;
    RCC_OscInitStruct.PLL.PLLFRACN        = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        while (1) {}

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2  |
                                       RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        while (1) {}
}

void board_init(void)
{
    HAL_Init();
    SystemClock_Config();

    /* Enable instruction cache (required for reliable operation at 160 MHz) */
    HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY);
    HAL_ICACHE_Enable();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio_register(GPIO_CTRL_ONCHIP, &__gpio_ops);
}
