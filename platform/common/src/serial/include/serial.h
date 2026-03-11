#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SERIAL_MAX_DEVICES 8

typedef enum
{
    SERIAL_ROLE_STDIO,   /* S0 */
    SERIAL_ROLE_GENERIC, /* S1, S2, ... */
} serial_role_t;

struct serial_ops
{
    int (*open)(void *ctx);
    int (*close)(void *ctx);
    int (*write)(void *ctx, const uint8_t *buf, size_t len);
    int (*read)(void *ctx, uint8_t *buf, size_t len);
};

struct serial_device
{
    const char *name; /* "S0", "S1", ... */
    const struct serial_ops *ops;
    void *ctx; /* backend-specific context */
};

int
serial_register(struct serial_device *dev, serial_role_t role);
int
serial_open(const char *name);
FILE *
serial_fopen(const char *name, const char *mode);
int
serial_close(int fd);

int
serial_write(int fd, const uint8_t *buf, size_t len);
int
serial_read(int fd, uint8_t *buf, size_t len);
