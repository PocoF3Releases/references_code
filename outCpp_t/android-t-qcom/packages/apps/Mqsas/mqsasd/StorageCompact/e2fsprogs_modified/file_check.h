/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _FILE_CHECK_H_
#define _FILE_CHECK_H_

#include <ext2fs/ext2fs.h>

/* the maxmuim defrag range returned by kernel will not larger than 130 */
#define MAX_NUM_OF_BLOCK_INFO  130

enum block_type {
    REG_FILE_DATA_BLOCK,
    EXTENT_BLOCK,
    NON_REG_FILE_BLOCK,
    ACL_BLOCK,
    SPECIAL_INODE_BLOCK,
};

struct swap_block_info {
    blk64_t         blk;
    blk64_t         l_blk;
    int             len;
    ext2_ino_t      ino;
    enum block_type type;
    dgrp_t          ino_group;
    dgrp_t          dir_group;
    char            filename[PATH_MAX];
};

/* statistics result of icheck */
extern __u64 total_icheck_num;
/* inode is in the same block group as data block */
extern __u64 icheck_hit_num;
/* inode is on the left side of data block */
extern __u64 icheck_left_num;
/* inode is on the right side of data block */
extern __u64 icheck_right_num;
/* block group is in the block groups cache */
extern __u64 icheck_cache_hit_num;

/* statistics result of ncheck */
extern __u64 total_ncheck_num;
/* dir inode is in the same block group as file inode */
extern __u64 ncheck_hit_num;
/* dir inode is on the left side of the file inode */
extern __u64 ncheck_left_num;
/* dir inode is on the right side of the file inode */
extern __u64 ncheck_right_num;
/* block group is in the block groups cache */
extern __u64 ncheck_cache_hit_num;

extern struct swap_block_info *swap_blocks_info;

extern ext2_filsys current_fs;

/* icheck.c */
extern int enhance_icheck(dgrp_t group, blk64_t start_blk, blk64_t end_blk);

/* ncheck.c */
extern int enhance_ncheck(struct swap_block_info *ino_array, int num_inodes);

#endif /* _FILE_CHECK_H_ */
