#pragma once

#include <stdint.h>
#include <stddef.h>

#define SPI_MAX_DEVICES 4

struct spi_bus_ops {
    int (*open)(void *ctx);
    int (*close)(void *ctx);
    int (*write)(void *ctx, const uint8_t *buf, uint16_t len);
    int (*read)(void *ctx, uint8_t *buf, uint16_t len);
    int (*transfer)(void *ctx, const uint8_t *tx, uint8_t *rx, uint16_t len);
};

struct spi_device {
    const char              *name;
    const struct spi_bus_ops *ops;
    void                    *ctx;
};

int spi_register(struct spi_device *dev);

int spi_open(const char *name);
int spi_close(int fd);
int spi_write(int fd, const uint8_t *buf, uint16_t len);
int spi_read(int fd, uint8_t *buf, uint16_t len);
int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, uint16_t len);
