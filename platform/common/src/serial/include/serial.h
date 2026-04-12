#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Total fd slots: 0-2 = stdio, 3-(SERIAL_FD_TABLE_SIZE-1) = generic */
#define SERIAL_FD_TABLE_SIZE 8

typedef enum
{
    SERIAL_ROLE_STDIO,   /* occupies fds 0, 1, 2 (stdin/stdout/stderr) */
    SERIAL_ROLE_GENERIC, /* allocated from fd 3 upward */
} serial_role_t;

struct serial_ops
{
    int (*open)(void *ctx);
    int (*close)(void *ctx);
    int (*write)(void *ctx, const uint8_t *buf, size_t len);
    int (*read)(void *ctx, uint8_t *buf, size_t len);
    /*
     * read_available — returns the number of bytes waiting in the RX
     * buffer without consuming them.  Optional: NULL is treated as 0
     * (always blocking behaviour in syscalls._read spin-wait).
     */
    int (*read_available)(void *ctx);
};

struct serial_device
{
    const char             *name; /* e.g. "S0", "USB0" */
    const struct serial_ops *ops;
    void                   *ctx;  /* backend-specific context */
};

/* Registration / lookup -------------------------------------------- */

/*
 * serial_register — register a serial device.
 *   SERIAL_ROLE_STDIO: binds fds 0/1/2 (stdin/stdout/stderr); rejects
 *                      double-register.
 *   SERIAL_ROLE_GENERIC: allocates the next fd >= 3.
 *   Returns the assigned fd on success, -1 on error.
 */
int   serial_register(struct serial_device *dev, serial_role_t role);

/*
 * serial_open — look up a registered device by name.
 *   Scans generic slots only (fd >= 3); never returns a stdio fd.
 *   Returns the fd, or -1 if not found.
 */
int   serial_open(const char *name);

/* fdopen wrapper for use with stdio FILE * */
FILE *serial_fopen(const char *name, const char *mode);

int   serial_close(int fd);

/* I/O -------------------------------------------------------------- */

int   serial_write(int fd, const uint8_t *buf, size_t len);
int   serial_read(int fd, uint8_t *buf, size_t len);

/*
 * serial_read_available — bytes waiting in the RX buffer for fd.
 *   Returns 0 if ops->read_available is NULL (i.e. always-blocking
 *   backend).  Returns -1 for an invalid fd.
 */
int   serial_read_available(int fd);
