/*
 * icheck.cpp --- given a list of blocks, generate a list of inodes
 *
 * Copyright (C) Xiaomi Inc.
 *
 */
#define LOG_TAG "FragCompact"
//#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <sys/types.h>

#include "../../utils.h"
#include "../../log.h"
#include "cache.h"

extern CompactLogger log;

#undef __bitwise
#include <pthread.h>
#include "ext2fs.h"
#include "debugfs.h"

#include "file_check.h"

#define MAX_SPECIAL_INODE_INO  11

#define EXTENT_BLOCK_CNT       -1
#define ACL_BLOCK_CNT          -2

struct block_walk_struct {
    struct swap_block_info  *barray;
    e2_blkcnt_t             blocks_left;
    int                     ino_is_reg;
    int                     extents_num;
    blk64_t                 start_blk;
    blk64_t                 end_blk;
    ext2_ino_t              inode;
    dgrp_t                  group;
};

/* statistics result of icheck */
__u64 total_icheck_num;
__u64 icheck_hit_num;
__u64 icheck_left_num;
__u64 icheck_right_num;
__u64 icheck_cache_hit_num;

static int icheck_proc(ext2_filsys fs EXT2FS_ATTR((unused)),
               blk64_t    *block_nr,
               e2_blkcnt_t blockcnt,
               blk64_t ref_block,
               int ref_offset EXT2FS_ATTR((unused)),
               void *aa)
{
    struct block_walk_struct *bw = (struct block_walk_struct *) aa;

    if ((blockcnt > 0 && ((bw->start_blk <= *block_nr && *block_nr + blockcnt - 1 <= bw->end_blk)
            || (*block_nr < bw->start_blk && *block_nr + blockcnt - 1 >= bw->start_blk)
            || (*block_nr <= bw->end_blk && *block_nr + blockcnt -1 > bw->end_blk)))
            || ((blockcnt == EXTENT_BLOCK_CNT || blockcnt == ACL_BLOCK_CNT) && (bw->start_blk <= *block_nr && *block_nr <= bw->end_blk))) {
        if (bw->ino_is_reg == 0) {
            bw->barray[bw->extents_num].type = NON_REG_FILE_BLOCK;
            bw->barray[bw->extents_num].len = blockcnt;
            bw->barray[bw->extents_num].blk = *block_nr;
        } else if (blockcnt == EXTENT_BLOCK_CNT) {
            bw->barray[bw->extents_num].type = EXTENT_BLOCK;
            bw->barray[bw->extents_num].len = 1;
            bw->barray[bw->extents_num].blk = *block_nr;
        } else if (blockcnt == ACL_BLOCK_CNT) {
            bw->barray[bw->extents_num].type = ACL_BLOCK;
            bw->barray[bw->extents_num].blk = *block_nr;
        } else if (bw->inode < MAX_SPECIAL_INODE_INO) {
            bw->barray[bw->extents_num].type = SPECIAL_INODE_BLOCK;
            bw->barray[bw->extents_num].len = blockcnt;
            bw->barray[bw->extents_num].blk = *block_nr;
        } else {
            if (bw->start_blk <= *block_nr && *block_nr + blockcnt - 1 <= bw->end_blk) {
                bw->barray[bw->extents_num].blk = *block_nr;
                bw->barray[bw->extents_num].len = blockcnt;
                bw->barray[bw->extents_num].l_blk = ref_block;
            } else if (*block_nr < bw->start_blk && *block_nr + blockcnt - 1 >= bw->start_blk) {
                if (*block_nr + blockcnt - 1 <= bw->end_blk) {
                    bw->barray[bw->extents_num].len = *block_nr + blockcnt - bw->start_blk;
                } else {
                    bw->barray[bw->extents_num].len = bw->end_blk - bw->start_blk + 1;
                }

                bw->barray[bw->extents_num].blk = bw->start_blk;
                bw->barray[bw->extents_num].l_blk = ref_block + bw->start_blk - *block_nr;
            } else {
                bw->barray[bw->extents_num].blk = *block_nr;
                bw->barray[bw->extents_num].len = bw->end_blk - *block_nr + 1 ;
                bw->barray[bw->extents_num].l_blk = ref_block;
            }

            bw->barray[bw->extents_num].type = REG_FILE_DATA_BLOCK;
            bw->blocks_left -= bw->barray[bw->extents_num].len;
        }

        bw->barray[bw->extents_num].ino = bw->inode;
        bw->barray[bw->extents_num].ino_group = bw->group;

        LOGV("block %llu is found, blockcnt = %lld, its inode number is %u, inode is in group %u", *block_nr, blockcnt, bw->inode, bw->group);

        if (!bw->blocks_left || bw->extents_num > MAX_NUM_OF_BLOCK_INFO - 1
               || bw->barray[bw->extents_num].type != REG_FILE_DATA_BLOCK) {
            if(bw->blocks_left == 0) {
                bw->extents_num++;
            }

            return BLOCK_ABORT;
        }

        bw->extents_num++;
    }

    return 0;
}

/*
 * Sort block walk array by pysical block number in ascending order
 */
static void icheck_sort(struct block_walk_struct *bw)
{
    int i, j, need_sort;
    struct swap_block_info binfo;

    for (i = 0; i < bw->extents_num - 1; i++) {
        need_sort = 0;
        for (j = 0; j < bw->extents_num - 1 - i; j++) {
            if (bw->barray[j].blk > bw->barray[j+1].blk) {
                need_sort = 1;
                binfo.blk = bw->barray[j].blk;
                binfo.len = bw->barray[j].len;
                binfo.ino = bw->barray[j].ino;
                binfo.type = bw->barray[j].type;
                binfo.l_blk = bw->barray[j].l_blk;
                binfo.ino_group = bw->barray[j].ino_group;

                bw->barray[j].blk = bw->barray[j+1].blk;
                bw->barray[j].len = bw->barray[j+1].len;
                bw->barray[j].ino = bw->barray[j+1].ino;
                bw->barray[j].type = bw->barray[j+1].type;
                bw->barray[j].l_blk = bw->barray[j+1].l_blk;
                bw->barray[j].ino_group = bw->barray[j+1].ino_group;

                bw->barray[j+1].blk = binfo.blk;
                bw->barray[j+1].len = binfo.len;
                bw->barray[j+1].ino = binfo.ino;
                bw->barray[j+1].type = binfo.type;
                bw->barray[j+1].l_blk = binfo.l_blk;
                bw->barray[j+1].ino_group = binfo.ino_group;
            }
        }

        if (need_sort == 0)
            break;
    }
}

/*
 * calculate statistics result of icheck
 */
static void icheck_calc_statistic(struct block_walk_struct *bw, dgrp_t group)
{
    int i;

    for (i = 0; i < bw->extents_num; i++) {
        total_icheck_num++;

        if (group == bw->barray[i].ino_group) {
            icheck_hit_num++;
        } else if (group < bw->barray[i].ino_group) {
            icheck_right_num++;
        } else {
            icheck_left_num++;
        }

        if (icheck_is_group_cached(bw->barray[i].ino_group))
            icheck_cache_hit_num++;
    }
}

static void icheck_add_block_groups(struct block_walk_struct *bw)
{
    int i;

    for (i = 0; i < bw->extents_num; i++) {
        icheck_cache_add(bw->barray[i].ino_group);
        /*
         * Add block groups where inodes of data blocks within same defrag range are found
         * into ncheck block group cache. This may help improve incoming ncheck performance
         */
        ncheck_cache_add(bw->barray[i].ino_group);
    }
}

/*
 * enhance_icheck - find inodes of data blocks in specified range
 *
 * return: number of extents on success, -1 on error
 */
int enhance_icheck(dgrp_t group, blk64_t start_blk, blk64_t end_blk)
{
    int ret = -1;
    ext2_inode_scan        scan = 0;
    ext2_ino_t        ino;
    struct ext2_inode    inode;
    errcode_t        retval;
    char            *block_buf;
    dgrp_t    ino_group = 0;
    struct block_walk_struct bw;

    LOGV("enhance_icheck enter, group is %u, start block is %llu, end block is %llu", group, start_blk, end_blk);

    block_buf = (char *)malloc(current_fs->blocksize * 3);
    if (!block_buf) {
        LOGE("error while allocating block buffer");
        return -1;
    }

    bw.start_blk = start_blk;
    bw.end_blk = end_blk;
    bw.extents_num = 0;
    bw.blocks_left = bw.end_blk - bw.start_blk + 1;
    bw.barray = swap_blocks_info;

    print_block_group_cache(icheck_group_cache);

    retval = ext2fs_open_inode_scan_ex(current_fs, 0, group, icheck_group_cache, &scan);
    if (retval) {
        LOGE("error while opening inode scan, retval is %ld", retval);
        free(block_buf);
        return -1;
    }

    do {
        retval = ext2fs_get_next_inode_ex(scan, &ino, &inode, &ino_group);
    } while (retval == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE);
    if (retval) {
        LOGE("error while starting inode scan, retval is %ld", retval);
        goto exit;
    }

    while (ino) {
        blk64_t blk;

        if (!inode.i_links_count)
            goto next;

        /* Inode don't use extents */
        if ((inode.i_flags & EXT4_EXTENTS_FL) != EXT4_EXTENTS_FL)
            goto next;

        bw.inode = ino;
        bw.group = ino_group;
        if (LINUX_S_ISREG(inode.i_mode))
            bw.ino_is_reg = 1;
        else
            bw.ino_is_reg = 0;

        blk = ext2fs_file_acl_block(current_fs, &inode);
        if (blk) {
            icheck_proc(current_fs, &blk, ACL_BLOCK_CNT, 0, 0, &bw);
            if (bw.blocks_left == 0
                    || bw.extents_num > MAX_NUM_OF_BLOCK_INFO - 1
                    || bw.barray[bw.extents_num].type != REG_FILE_DATA_BLOCK)
                break;
            ext2fs_file_acl_block_set(current_fs, &inode, blk);
        }

        if (!ext2fs_inode_has_valid_blocks2(current_fs, &inode))
            goto next;
        /*
         * To handle filesystems touched by 0.3c extfs; can be
         * removed later.
         */
        if (inode.i_dtime)
            goto next;

        retval = ext2fs_block_iterate3(current_fs, ino,
                           BLOCK_FLAG_READ_ONLY | BLOCK_FLAG_EXTENT_WAY, block_buf,
                           icheck_proc, &bw);
        if (retval) {
            LOGE("error while calling ext2fs_block_iterate, retval is %ld", retval);
            goto next;
        }

        if (bw.blocks_left == 0
                || bw.extents_num > MAX_NUM_OF_BLOCK_INFO - 1
                || bw.barray[bw.extents_num].type != REG_FILE_DATA_BLOCK)
            break;

        pthread_mutex_lock(&stop_mutex);
        if (g_stop_flag == 1) {
            pthread_mutex_unlock(&stop_mutex);
            ret = -3;
            LOGI("user cancel storage compact");
            log.markExitReason("user_cancel");
            goto exit;
        }
        pthread_mutex_unlock(&stop_mutex);

    next:
        do {
            retval = ext2fs_get_next_inode_ex(scan, &ino, &inode, &ino_group);
        } while (retval == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE);
        if (retval) {
            LOGE("error while doing inode scan, retval is %ld", retval);
            goto exit;
        }
    }

    if (bw.blocks_left != 0) {
        LOGD("some blocks in range %llu - %llu can not find corresponding inode or can not be moved", bw.start_blk, bw.end_blk);
        ret = -2;
        goto exit;
    }

    /* Sort g_bw by pysical block number in ascending order */
    icheck_sort(&bw);

    /* Calculate statistics result of icheck */
    icheck_calc_statistic(&bw, group);

    /* Add all block groups into block group cache */
    icheck_add_block_groups(&bw);

    /* Return number of extents */
    ret = bw.extents_num;
exit:
    free(block_buf);
    if (scan)
        ext2fs_close_inode_scan(scan);
    return ret;
}
