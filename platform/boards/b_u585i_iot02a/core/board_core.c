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

typedef struct {
    gpio_pin_t    pin;
    gpio_irq_cb_t cb;
    void         *ctx;
} board_irq_slot_t;

static board_irq_slot_t board_irq_slots[16];

static void board_gpio_init(gpio_pin_t pin, gpio_mode_t mode, gpio_pull_t pp)
{
    GPIO_TypeDef *port    = port_table[gpio_get_port(pin)];
    uint16_t      pin_num = gpio_get_pin(pin);

    gpio_drv_mode_t __mode;
    gpio_drv_pull_t pull = GPIO_DRV_PULL_NONE;

#define mycase(__in__, __out__) \
    case GPIO_MODE_##__in__: \
    __mode = GPIO_DRV_MODE_##__out__; \
    break;
    switch (mode) {
        mycase(INPUT,  INPUT);
        mycase(OUTPUT, OUTPUT);
        mycase(AF,     AF);
        mycase(ANALOG, ANALOG);
    }
#undef mycase

#define mycase(__in__, __out__) \
    case GPIO_PULL_##__in__: \
    pull = GPIO_DRV_PULL_##__out__; \
    break;
    switch (pp) {
        mycase(NONE, NONE);
        mycase(UP,   UP);
        mycase(DOWN, DOWN);
    }
#undef mycase

    gpio_drv_init_pin((gpio_port_t)port, pin_num, __mode, pull);
}

static void board_gpio_wrtie(gpio_pin_t pin, gpio_level_t level)
{
    GPIO_TypeDef *port    = port_table[gpio_get_port(pin)];
    uint16_t      pin_num = gpio_get_pin(pin);

    gpio_drv_write((gpio_port_t)port, pin_num, level == GPIO_HIGH);
}

static gpio_level_t board_gpio_read(gpio_pin_t pin)
{
    GPIO_TypeDef *port    = port_table[gpio_get_port(pin)];
    uint16_t      pin_num = gpio_get_pin(pin);

    return gpio_drv_read((gpio_port_t)port, pin_num) == GPIO_PIN_SET;
}

static void board_gpio_toggle(gpio_pin_t pin)
{
    GPIO_TypeDef *port    = port_table[gpio_get_port(pin)];
    uint16_t      pin_num = gpio_get_pin(pin);

    gpio_drv_toggle((gpio_port_t)port, pin_num);
}

static void board_exti_irq_cb(gpio_port_t port, uint32_t pin, void *ctx)
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
    GPIO_TypeDef    *port     = port_table[gpio_get_port(pin)];
    uint16_t         pin_num  = gpio_get_pin(pin);
    gpio_drv_irq_edge_t drv_edge;

    uint32_t line = __builtin_ctz(pin_num);

    if (board_irq_slots[line].cb) return -1;

    board_irq_slots[line] = (board_irq_slot_t){
        .pin = pin,
        .cb  = cb,
        .ctx = ctx,
    };

#define mycase(__in, __out) \
    case GPIO_IRQ_EDGE_##__in: \
    drv_edge = GPIO_DRV_IRQ_EDGE_##__out; \
    break;
    switch (edge) {
        mycase(RISING,  RISING);
        mycase(FALLING, FALLING);
        mycase(BOTH,    BOTH);
    }
#undef mycase

    return gpio_drv_irq_register(
            port,
            pin_num,
            drv_edge,
            board_exti_irq_cb,
            (void *)(uintptr_t)line);
}

static void board_gpio_irq_enable(gpio_pin_t pin)
{
    GPIO_TypeDef *port    = port_table[gpio_get_port(pin)];
    uint16_t      pin_num = gpio_get_pin(pin);

    gpio_drv_irq_enable(port, pin_num);
}

static void board_gpio_irq_disable(gpio_pin_t pin)
{
    GPIO_TypeDef *port    = port_table[gpio_get_port(pin)];
    uint16_t      pin_num = gpio_get_pin(pin);

    gpio_drv_irq_disable(port, pin_num);
}

static const struct gpio_ops __gpio_ops = {
    .init         = board_gpio_init,
    .write        = board_gpio_wrtie,
    .read         = board_gpio_read,
    .toggle       = board_gpio_toggle,
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
    __HAL_RCC_GPIOI_CLK_ENABLE();

    gpio_register(&__gpio_ops);
}
