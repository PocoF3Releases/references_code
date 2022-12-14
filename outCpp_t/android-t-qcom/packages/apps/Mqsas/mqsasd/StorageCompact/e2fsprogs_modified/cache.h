/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _CACHE_H_
#define _CACHE_H_

#include "ext2fs.h"

extern struct ext2_block_group_cache *icheck_group_cache;
extern struct ext2_block_group_cache *ncheck_group_cache;

extern void icheck_cache_add(dgrp_t group);
extern void ncheck_cache_add(dgrp_t group);

extern void create_check_cache();
extern void release_check_cache();
extern void reset_check_cache();
extern void print_block_group_cache(struct ext2_block_group_cache *cache);
extern int icheck_is_group_cached(dgrp_t group);
extern int ncheck_is_group_cached(dgrp_t group);
#endif