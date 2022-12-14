/**
 * Copyright (c) 2015 The CheckFs Open Source Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _IO_IO_H_
#define _IO_IO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Flags for the io_channel structure */
#define IO_FLAG_RW         0x1
#define IO_FLAG_EXECLUSIVE 0x2

typedef struct io_manager *struct_io_manager_ptr;
typedef struct io_channel *struct_io_channel_ptr;

/* io_linux.c */
extern struct_io_manager_ptr linux_io_manager_ptr;

/*
 * fs_offset_t is defined here.
 */
#if defined(__GNUC__) || defined(HAS_LONG_LONG)
typedef long long fs_offset_t;
#else
typedef long      fs_offset_t;
#endif

struct io_channel {
    int   magic;

    struct_io_manager_ptr manager;

    char *device_name;
    int   block_size;
    int   ref_counts;
    int   flags;
    void *private_data;
    void *fs;
};

struct io_manager {
    int magic;
    const char *manager_name;

    int (*open)(const char *device_name, int io_flags,
                struct_io_channel_ptr *channel);
    int (*close)(struct_io_channel_ptr channel);
    int (*set_blksize)(int block_size, struct_io_channel_ptr channel);
    int (*read_block)(void *buf, unsigned long block, int count,
                      struct_io_channel_ptr channel);
    int (*read_block64)(void *buf, unsigned long long block, int count,
                        struct_io_channel_ptr channel);
};

struct io_stats {
    int                num_fields;
    unsigned long long bytes_read;
    unsigned long long bytes_written;
};

void free_io_channel(struct_io_channel_ptr channel);
fs_offset_t llseek(int dev, fs_offset_t offset, int whence);

/* Convenience functions */
#define io_channel_close(channel) ((channel)->manager->close((channel)))
#define io_channel_set_blksize(block_size, channel) \
    ((channel)->manager->set_blksize(block_size, (channel)))
#define io_channel_read_block(buf, block, count, channel) \
    ((channel)->manager->read_block(buf, block, count, (channel)))

#ifdef __cplusplus
}
#endif

#endif /* _IO_IO_H_ */
