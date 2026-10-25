/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <pthread.h>
#include <functional>
#include <stdlib.h>

extern int get_block_device(char *, char *);
extern int get_file_value(char *path, unsigned int *value);
extern int cksum_generate(char *path, unsigned int *value);

extern volatile int g_stop_flag;
extern pthread_mutex_t stop_mutex;

extern int create_dir_if_needed(const char* pathname, mode_t mode);

extern int delete_dir_contents(const char* pathname, bool delete_dir,
        std::function<bool(const char*, bool)> exclusion_predicate = nullptr);

extern int delete_dir_contents_and_dir(const char* pathname);

extern int visit_dir_contents(const char* pathname, bool recursive,
        std::function<void(const char*, bool)> visitor);

extern int copy_file(const char *fromPath, const char *toPath);
#endif /* _UTILS_H_ */
