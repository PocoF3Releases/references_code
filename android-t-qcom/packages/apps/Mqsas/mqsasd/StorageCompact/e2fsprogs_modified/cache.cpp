/*
 * cache.cpp --- implement block groups cache
 *
 * Copyright (C) Xiaomi Inc.
 *
 */
#define LOG_TAG "FragCompact"

#include "ext2fs.h"
#include "../../log.h"

extern CompactLogger log;

#define MAX_ICHECK_CACHED_GROUP_NUM 16
#define MAX_NCHECK_CACHED_GROUP_NUM 8

struct ext2_block_group_cache *icheck_group_cache = NULL;
struct ext2_block_group_cache *ncheck_group_cache = NULL;

void icheck_cache_add(dgrp_t group)
{
    if (icheck_group_cache != NULL)
        ext2fs_block_group_reference(icheck_group_cache, group);
}

void ncheck_cache_add(dgrp_t group)
{
    if (ncheck_group_cache != NULL)
        ext2fs_block_group_reference(ncheck_group_cache, group);
}

void reset_check_cache()
{
    if (icheck_group_cache != NULL)
        ext2fs_empty_block_group_cache(icheck_group_cache);

    if (ncheck_group_cache != NULL)
        ext2fs_empty_block_group_cache(ncheck_group_cache);
}

void print_block_group_cache(struct ext2_block_group_cache *cache)
{
    if (cache) {
        struct ext2_block_group_cache_ent *tmp;

        LOGV("block group cache: count is %d, cached groups are:", cache->count);
        tmp = cache->head;
        while (tmp != NULL) {
            LOGV("group: %u", tmp->group);
            tmp = tmp->next;
        }
    }
}

int icheck_is_group_cached(dgrp_t group)
{
    if (icheck_group_cache != NULL)
        return ext2fs_is_block_group_cached(icheck_group_cache, group);

    return 0;
}

int ncheck_is_group_cached(dgrp_t group)
{
    if (ncheck_group_cache != NULL)
        return ext2fs_is_block_group_cached(ncheck_group_cache, group);

    return 0;
}

void create_check_cache()
{
    icheck_group_cache = ext2fs_create_block_group_cache(MAX_ICHECK_CACHED_GROUP_NUM);
    if (icheck_group_cache == NULL) {
        LOGE("failed to create icheck block group cache");
        return;
    }

    ncheck_group_cache = ext2fs_create_block_group_cache(MAX_NCHECK_CACHED_GROUP_NUM);
    if (ncheck_group_cache == NULL) {
        LOGE("failed to create ncheck block group cache");
        return;
    }
}

void release_check_cache()
{
    if (icheck_group_cache != NULL) {
        ext2fs_release_block_group_cache(icheck_group_cache);
        icheck_group_cache = NULL;
    }

    if (ncheck_group_cache != NULL) {
        ext2fs_release_block_group_cache(ncheck_group_cache);
        ncheck_group_cache = NULL;
    }
}
