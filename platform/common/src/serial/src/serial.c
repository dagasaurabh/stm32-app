#include "serial.h"
#include <string.h>

struct fd_entry {
    struct serial_device * dev;
};

static struct fd_entry fd_table[SERIAL_MAX_DEVICES];
static int next_fd = 3; /* 0,1,2 reserved */

static int stdio_fd = -1;

int serial_register(struct serial_device *dev, serial_role_t role)
{
    int fd;

    if (!dev || !dev->ops->write || !dev->ops->read) return -1;

    if (role == SERIAL_ROLE_STDIO && stdio_fd == -1) {
        fd = 1; /* stdout */
        stdio_fd = fd;
        fd_table[0].dev = dev; /* stdin */
        fd_table[1].dev = dev; /* stdout */
        fd_table[2].dev = dev; /* stderr */
    } else {
        if (next_fd >= SERIAL_MAX_DEVICES) return -1;

        fd = next_fd++;
        fd_table[fd].dev = dev;
    }

    if (dev->ops->open) dev->ops->open(dev->ctx);

    return fd;
}

int serial_open(const char *name)
{
    for (int i = 0; i < SERIAL_MAX_DEVICES; i++) {
        if (fd_table[i].dev && 
                strcmp(fd_table[i].dev->name, name) == 0) return i;
    }
    return -1;
}

FILE *serial_fopen(const char *name, const char *mode)
{
    int fd = serial_open(name);

    if (fd < 0) return NULL;

    return fdopen(fd, mode);
}

int serial_write(int fd, const uint8_t *buf, size_t len)
{
    if (fd < 0 || fd >= SERIAL_MAX_DEVICES || !fd_table[fd].dev) return -1;

    return fd_table[fd].dev->ops->write(fd_table[fd].dev->ctx, buf, len);
}

int serial_read(int fd, uint8_t *buf, size_t len)
{
    if (fd < 0 || fd >= SERIAL_MAX_DEVICES || !fd_table[fd].dev) return -1;

    return fd_table[fd].dev->ops->read(fd_table[fd].dev->ctx, buf, len);
}

int serial_close(int fd)
{
    if (fd < 0 || fd >= SERIAL_MAX_DEVICES || !fd_table[fd].dev) return -1;

    if (fd >= 0 && fd <= 2) return -1;

    fd_table[fd].dev = NULL;

    return 0;
}
