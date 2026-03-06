#include "gpio.h"
#include "board.h"
#include "gpio_driver.h"
#include "stm32l5xx_hal.h"

static GPIO_TypeDef *port_table[] = {
    GPIOA,
    GPIOB,
    GPIOC,
    GPIOD,
    GPIOE,
    GPIOF,
    GPIOG,
    GPIOH,
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
    gpio_pin_t pin;
    gpio_irq_cb_t cb;
    void *ctx;
}board_irq_slot_t;

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
    GPIO_TypeDef *port    = board_get_port(pin);
    uint16_t      pin_num = gpio_pin_mask(pin);

    if (port == NULL || pin_num == 0U) {
        return -1;
    }

    /*
     * "Count Trailing Zeros" in the binary representation of x.
     */
    uint32_t line = __builtin_ctz(pin_num);

    if(board_irq_slots[line].cb) return -1;

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
    .init = board_gpio_init,
    .write = board_gpio_write,
    .read = board_gpio_read,
    .toggle = board_gpio_toggle,
    .irq_register = board_gpio_irq_register,
    .irq_enable   = board_gpio_irq_enable,
    .irq_disable  = board_gpio_irq_disable,
};

void board_init(void)
{
    HAL_Init();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio_register(GPIO_CTRL_ONCHIP, &__gpio_ops);

}
