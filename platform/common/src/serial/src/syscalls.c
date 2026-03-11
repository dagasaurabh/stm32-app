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
    return serial_read(fd, (uint8_t *)buf, len);
}

int
_close(int fd)
{
    (void)fd;
    return 0;
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
