/*
 * ncheck.cpp --- given a list of inodes, generate a list of names
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
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
extern int optind;
extern char *optarg;
#endif

#undef __bitwise
#include <pthread.h>
#include "ext2fs.h"
#include "debugfs.h"
#include "ext4_decrypt.h"

#include "../../utils.h"
#include "../../log.h"
#include "cache.h"
#include "file_check.h"

extern CompactLogger log;

extern errcode_t ext2fs_get_pathname_ex(ext2_filsys fs, ext2_ino_t dir, ext2_ino_t ino,
                  char **name);

struct inode_walk_struct {
    struct swap_block_info  *iarray;
    ext2_ino_t              dir;
    int                     inodes_left;
    int                     num_inodes;
    int                     position;
    char                    *parent;
    unsigned int            get_pathname_failed:1;
    unsigned int            check_dirent:1;
    dgrp_t                  group;
};

/* statistics result of ncheck */
__u64 total_ncheck_num;
__u64 ncheck_hit_num;
__u64 ncheck_left_num;
__u64 ncheck_right_num;
__u64 ncheck_cache_hit_num;

static int ncheck_proc(struct ext2_dir_entry *dirent,
               int    offset EXT2FS_ATTR((unused)),
               int    blocksize EXT2FS_ATTR((unused)),
               char    *buf EXT2FS_ATTR((unused)),
               void    *aa)
{
    struct inode_walk_struct *iw = (struct inode_walk_struct *) aa;
    errcode_t    retval;
    int        filetype = ext2fs_dirent_file_type(dirent);
    int        i, length;
    struct fscrypt_str iname;
    struct fscrypt_str oname;
    int ret;

    iw->position++;
    if (iw->position <= 2)
        return 0;
    for (i=0; i < iw->num_inodes; i++) {
        if (iw->iarray[i].ino == dirent->inode) {
            LOGV("inode %u is found, its dir ino is %u, corresponding group is %u",dirent->inode, iw->dir, iw->group);
            iw->inodes_left--;
            iw->iarray[i].dir_group = iw->group;
            if (!iw->parent && !iw->get_pathname_failed) {
                retval = ext2fs_get_pathname_ex(current_fs,
                                 iw->dir,
                                 0, &iw->parent);
                if (retval) {
                    LOGE("error while calling ext2fs_get_pathname_ex for inode #%u", iw->dir);
                    iw->get_pathname_failed = 1;
                }
            }

            length = ext2fs_dirent_name_len(dirent);
            if (ext2fs_is_dir_encrypted(iw->dir)) {
                memcpy(iname.fname, dirent->name, length);
                iname.len = length;

                ret = ext2fs_decrypt_fname(iw->dir, &iname, &oname);
                if (ret) {
                    LOGE("ext4_decrypt_fname failed, ret is %d", ret);
                    return DIRENT_ABORT;
                }

                LOGV("ncheck_proc: ext4_decrypt_fname return, encrypt filename is %s, len is %u, after decrypt filename is %s, len is %u",
                        dirent->name, length, oname.fname, oname.len);
            } else {
                LOGV("ncheck_proc: dir is not encrypted, filename is %s, len is %u", dirent->name, length);
                strncpy(oname.fname, dirent->name, length);
                oname.len = length;
            }

            if (iw->parent) {
                strcpy(iw->iarray[i].filename, iw->parent);
                strcat(iw->iarray[i].filename, "/");
                strncat(iw->iarray[i].filename, oname.fname, oname.len);
            } else {
                strncpy(iw->iarray[i].filename, oname.fname, oname.len);
            }

            if (iw->check_dirent && filetype) {
                LOGD("  <--- BAD FILETYPE");
            }
        }
    }
    if (!iw->inodes_left)
        return DIRENT_ABORT;

    return 0;
}

/*
 * calculate statistics result of ncheck
 */
static void ncheck_calc_statistic(struct inode_walk_struct *iw)
{
    int i;

    for (i = 0; i < iw->num_inodes; i++) {
        total_ncheck_num++;

        if (iw->iarray[i].ino_group == iw->iarray[i].dir_group) {
            ncheck_hit_num++;
        } else if (iw->iarray[i].ino_group < iw->iarray[i].dir_group) {
            ncheck_right_num++;
        } else {
            ncheck_left_num++;
        }

        if (ncheck_is_group_cached(iw->iarray[i].dir_group))
            ncheck_cache_hit_num++;
    }
}

static void ncheck_add_block_groups(struct inode_walk_struct *iw)
{
    int i;

    for (i = 0; i < iw->num_inodes; i++)
        ncheck_cache_add(iw->iarray[i].dir_group);
}

int enhance_ncheck(struct swap_block_info *ino_array, int num_inodes)
{
    struct inode_walk_struct iw;
    ext2_inode_scan        scan = 0;
    ext2_ino_t        ino;
    struct ext2_inode    inode;
    errcode_t        retval;
    dgrp_t    dir_group;
    int ret = -1;

    LOGV("enhance_ncheck enter");

    iw.check_dirent = 0;
    iw.iarray = ino_array;
    iw.num_inodes = iw.inodes_left = num_inodes;

    print_block_group_cache(ncheck_group_cache);

    retval = ext2fs_open_inode_scan_ex(current_fs, 0, 0, ncheck_group_cache, &scan);
    if (retval) {
        LOGE("error while opening inode scan, retval is %ld", retval);
        goto error_out;
    }

    do {
        retval = ext2fs_get_next_inode_ex(scan, &ino, &inode, &dir_group);
    } while (retval == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE);
    if (retval) {
        LOGE("error while starting inode scan, retval is %ld", retval);
        goto error_out;
    }

    while (ino) {
        if (!inode.i_links_count)
            goto next;
        /*
         * To handle filesystems touched by 0.3c extfs; can be
         * removed later.
         */
        if (inode.i_dtime)
            goto next;
        /* Ignore anything that isn't a directory */
        if (!LINUX_S_ISDIR(inode.i_mode))
            goto next;

        iw.position = 0;
        iw.parent = 0;
        iw.dir = ino;
        iw.get_pathname_failed = 0;
        iw.group = dir_group;

        retval = ext2fs_dir_iterate(current_fs, ino, 0, 0,
                        ncheck_proc, &iw);
        ext2fs_free_mem(&iw.parent);
        if (retval) {
            LOGE("error while calling ext2_dir_iterate, retval is %ld", retval);
            goto next;
        }

        if (iw.inodes_left == 0)
            break;

        pthread_mutex_lock(&stop_mutex);
        if (g_stop_flag == 1) {
            pthread_mutex_unlock(&stop_mutex);
            goto error_out;
        }
        pthread_mutex_unlock(&stop_mutex);

    next:
        do {
            retval = ext2fs_get_next_inode_ex(scan, &ino, &inode, &dir_group);
        } while (retval == EXT2_ET_BAD_BLOCK_IN_INODE_TABLE);

        if (retval) {
            LOGE("error while doing inode scan, retval is %ld", retval);
            goto error_out;
        }
    }

    if (iw.inodes_left != 0) {
        LOGE("some inode is not found");
        goto error_out;
    }

    /* Calculate statistics status of ncheck */
    ncheck_calc_statistic(&iw);

    /* Add all block groups into ncheck block group cache */
    ncheck_add_block_groups(&iw);

    ret = 0;
error_out:
    if (scan)
        ext2fs_close_inode_scan(scan);
    return ret;
}



