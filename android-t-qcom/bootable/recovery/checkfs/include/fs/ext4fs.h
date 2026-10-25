/**
 * Copyright (c) 2015 The CheckFs Open Source Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FS_EXT4FS_H_
#define _FS_EXT4FS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "ext4.h"
#include "io/io.h"

/* Flags for the ext4_fs structure */
#define EXT4_FLAG_RW       0x1
#define EXT4_FLAG_FORCE    0x2

/* How big superblock are supposed to be.
 * We define SUPERBLOCK_SIZE because the size of the superblock structure
 * is not necessarily trustworthy.
 */
#define EXT4_SUPERBLOCK_SIZE 1024

#define EXT4_SUPER_MAGIC 0xEF53

struct ext4_fs {
    int   magic;
    char *device_name;
    int   flags;

    struct ext4_super_block *super;

    struct_io_channel_ptr channel;
};
typedef struct ext4_fs *struct_ext4_fs_ptr;

int open_ext4fs(const char *device_name, int fs_flags,
                int superblock, unsigned int block_size,
                struct_io_manager_ptr manager,
                struct_ext4_fs_ptr *ret_fs);
void free_ext4fs(struct_ext4_fs_ptr fs);

void dump_ext4fs_super(struct ext4_super_block *s);
void dump_ext4fs_super_f(struct ext4_super_block *s, FILE *f);

int is_null_uuid(void *uuid);
const char *uuid2str(void *uuid);
const char *feature2str(int compat, unsigned int mask);
const char *default_mount_opts2str(unsigned int mask);
const char *os2str(int os_type);
const char *hash2str(int num);

void updater_print_fs_state(FILE *f, unsigned short state);
void updater_print_fs_errors(FILE *f, unsigned short errors);

#ifdef __cplusplus
}
#endif

#endif /* _FS_EXT4FS_H_ */
