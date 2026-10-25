#ifndef MIUI_ASHMEM_H
#define MIUI_ASHMEM_H
#include <linux/ashmem.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

static inline int ashmem_create_region(const char* name, size_t size) {
    int fd, ret;

    fd = open(ASHMEM_NAME_DEF, 0, O_RDWR);

    if (fd < 0)
        return fd;

    if (name) {
        char buf[ASHMEM_NAME_LEN];
        strlcpy(buf, name, sizeof(buf));
        ret = ioctl(fd, ASHMEM_SET_NAME, buf);
        if (ret < 0)
            goto error;
    }
    ret = ioctl(fd, ASHMEM_SET_SIZE, size);
    if (ret < 0)
        goto error;

    return fd;
error:
    close(fd);
    return ret;
}


#endif

