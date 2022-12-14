/**
 * Copyright (c) 2015 The CheckFs Open Source Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ERR_CODE_H_
#define _ERR_CODE_H_

#include <limits.h>

/* For checking structure magic number */
#define CHECKFS_check_magic(_struct, _magic)            \
    if ((_struct)->magic != (_magic)) return _magic;    \

#define CHECKFS_BASE                   (INT_MAX)

#define CHECKFS_MAGIC_EXT4FS           (CHECKFS_BASE - 1)
#define CHECKFS_MAGIC_IO_MANAGER       (CHECKFS_MAGIC_EXT4FS - 1)
#define CHECKFS_MAGIC_IO_CHANNEL       (CHECKFS_MAGIC_IO_MANAGER - 1)
#define CHECKFS_MAGIC_LINUX_PRIV_DATA  (CHECKFS_MAGIC_IO_CHANNEL - 1)
#define CHECKFS_MAGIC_END              (CHECKFS_MAGIC_LINUX_PRIV_DATA)

#define CHECKFS_ERR_NO_MEMORY          (CHECKFS_MAGIC_END - 1)
#define CHECKFS_ERR_BAD_SUPER_MAGIC    (CHECKFS_ERR_NO_MEMORY - 1)
#define CHECKFS_ERR_INVALID_ARGUMENT   (CHECKFS_ERR_BAD_SUPER_MAGIC - 1)
#define CHECKFS_ERR_CORRUPT_SUPERBLOCK (CHECKFS_ERR_INVALID_ARGUMENT - 1)
#define CHECKFS_ERR_BAD_DEVICE_NAME    (CHECKFS_ERR_CORRUPT_SUPERBLOCK - 1)
#define CHECKFS_ERR_LLSEEK             (CHECKFS_ERR_BAD_DEVICE_NAME - 1)
#define CHECKFS_ERR_SHORT_READ         (CHECKFS_ERR_LLSEEK - 1)

#endif /* _ERR_CODE_H_ */
