/**
 * Copyright (c) 2015 The CheckFs Open Source Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _CHECKFS_H_
#define _CHECKFS_H_

#ifdef __cplusplus
extern "C" {
#endif

void dump_ext4fs_linux(const char *device_name);

const char *my_error_message(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* _CHECKFS_H_ */
