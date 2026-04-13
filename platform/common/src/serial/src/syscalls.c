#include <unistd.h>
#include <sys/stat.h>
#include "serial.h"

int
_write(int fd, const char *buf, int len)
{
    return serial_write(fd, (const uint8_t *)buf, len);
}

int
_read(int fd, char *buf, int len)
{
    /*
     * Use read_available() when implemented. If unsupported, fall back
     * directly to the backend's blocking read() path.
     */
    int avail = serial_read_available(fd);
    if (avail == SERIAL_READ_AVAIL_UNSUPPORTED) return serial_read(fd, (uint8_t *)buf, len);
    if (avail < 0) return -1;

    while (avail == 0) avail = serial_read_available(fd);
    if (avail < 0) return -1;

    return serial_read(fd, (uint8_t *)buf, len);
}

int
_close(int fd)
{
    return serial_close(fd);
}

int
_lseek(int fd, int ptr, int dir)
{
    (void)fd;
    (void)ptr;
    (void)dir;
    return 0;
}

int
_fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int
_isatty(int fd)
{
    return (fd >= 0 && fd <= 2);
}
