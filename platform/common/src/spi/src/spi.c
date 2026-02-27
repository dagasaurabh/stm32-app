#include "spi.h"
#include <string.h>

static struct spi_device *fd_table[SPI_MAX_DEVICES];

int spi_register(struct spi_device *dev)
{
    if (!dev || !dev->ops) return -1;

    for (int i = 0; i < SPI_MAX_DEVICES; i++) {
        if (!fd_table[i]) {
            fd_table[i] = dev;
            if (dev->ops->open) dev->ops->open(dev->ctx);
            return i;
        }
    }
    return -1;
}

int spi_open(const char *name)
{
    if (!name) return -1;

    for (int i = 0; i < SPI_MAX_DEVICES; i++) {
        if (fd_table[i] && strcmp(fd_table[i]->name, name) == 0) return i;
    }

    return -1;
}

int spi_close(int fd)
{
    if (fd < 0 || fd >= SPI_MAX_DEVICES || !fd_table[fd]) return -1;

    if (fd_table[fd]->ops->close) fd_table[fd]->ops->close(fd_table[fd]->ctx);

    fd_table[fd] = NULL;
    return 0;
}

int spi_write(int fd, const uint8_t *buf, uint16_t len)
{
    if (fd < 0 || fd >= SPI_MAX_DEVICES || !fd_table[fd]) return -1;

    if (!fd_table[fd]->ops->write) return -1;

    return fd_table[fd]->ops->write(fd_table[fd]->ctx, buf, len);
}

int spi_read(int fd, uint8_t *buf, uint16_t len)
{
    if (fd < 0 || fd >= SPI_MAX_DEVICES || !fd_table[fd]) return -1;

    if (!fd_table[fd]->ops->read) return -1;

    return fd_table[fd]->ops->read(fd_table[fd]->ctx, buf, len);
}

int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    if (fd < 0 || fd >= SPI_MAX_DEVICES || !fd_table[fd]) return -1;
	
    if (!fd_table[fd]->ops->transfer) return -1;

    return fd_table[fd]->ops->transfer(fd_table[fd]->ctx, tx, rx, len);
}
