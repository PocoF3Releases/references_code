/*
 * ext4 filesystem defragmenter
 * based on external/e2fsprogs/misc/e4defrag.c
 * Copyright (C) 2009 NEC Software Tohoku, Ltd.
 *
 * Author: Akira Fujita    <a-fujita@rs.jp.nec.com>
 *     Takashi Sato    <t-sato@yk.jp.nec.com>
 */
#define LOG_TAG "FileDefrag"

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <dirent.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <limits.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#undef __bitwise
#include <ext2fs/ext2_types.h>
#include <ext2fs/ext2fs.h>
//#include <linux/fs.h>
#include <sys/ioctl.h>
#include <ext2fs/fiemap.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <log/log.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <pthread.h>

#include "utils.h"
#include "filedefrag.h"
#include "../log.h"

extern FileDefragLogger log;
int data_verify = 0;
int donor_fd;

/* A relatively new ioctl interface ... */
#ifndef EXT4_IOC_MOVE_EXT
#define EXT4_IOC_MOVE_EXT      _IOWR('f', 15, struct move_extent)
#endif

#define min(x, y) (((x) > (y)) ? (y) : (x))
/* Wrap up the free function */
#define FREE(tmp)                \
    do {                    \
        if ((tmp) != NULL)        \
            free(tmp);        \
    } while (0)                \
/* Insert list2 after list1 */
#define insert(list1, list2)            \
    do {                    \
        list2->next = list1->next;    \
        list1->next->prev = list2;    \
        list2->prev = list1;        \
        list1->next = list2;        \
    } while (0)

/* To delete unused warning */
#ifdef __GNUC__
#define EXT2FS_ATTR(x) __attribute__(x)
#else
#define EXT2FS_ATTR(x)
#endif

#define FTW_OPEN_FD       256
#define FS_EXT4           "ext4"

/* Magic number for ext4 */
#define EXT4_SUPER_MAGIC    0xEF53

#define BLOCK_SIZE          4096

/* Definition of flex_bg */
#define EXT4_FEATURE_INCOMPAT_FLEX_BG        0x0200

/* The following macro is used for ioctl FS_IOC_FIEMAP
 * EXTENT_MAX_COUNT:    the maximum number of extents for exchanging between
 *            kernel-space and user-space per ioctl
 */
#define EXTENT_MAX_COUNT    512

long g_defrag_stat[DEFRAG_STAT_MAX];
long g_donor_call_defrag_estat[DONOR_CALL_DEFRAG_ESTAT_MAX];
const char *extension_lists[] = {
    ".jpg",
    ".gif",
    ".png",
    ".avi",
    ".divx",
    ".m4a",
    ".m4v",
    ".m4p",
    ".mp4",
    ".mp3",
    ".3gp",
    ".wmv",
    ".wma",
    ".mpeg",
    ".mkv",
    ".mov",
    ".asx",
    ".asf",
    ".wmx",
    ".svi",
    ".wvx",
    ".wv",
    ".wm",
    ".mpg",
    ".mpe",
    ".rm",
    ".ogg",
    ".opus",
    ".flac",
    ".jpeg",
    ".video",
    ".apk",
    ".so",
    ".exe",
    ".db",
    ".db-shm",
    ".db-wal",
    ".db-journal",
    ".vdex",
    ".odex",
    ".art"
};
extern int only_defrag_extension;
extern unsigned int max_sectors_kb;

/* Data type for filesystem-wide blocks number */
typedef unsigned long long ext4_fsblk_t;

struct fiemap_extent_data {
    __u64 len;            /* blocks count */
    __u64 logical;        /* start logical block number */
    ext4_fsblk_t physical;        /* start physical block number */
};

struct fiemap_extent_list {
    struct fiemap_extent_list *prev;
    struct fiemap_extent_list *next;
    struct fiemap_extent_data data;    /* extent belong to file */
};

struct fiemap_extent_group {
    struct fiemap_extent_group *prev;
    struct fiemap_extent_group *next;
    __u64 len;    /* length of this continuous region */
    struct fiemap_extent_list *start;    /* start ext */
    struct fiemap_extent_list *end;        /* end ext */
};

struct move_extent {
    __s32 reserved;    /* original file descriptor */
    __u32 donor_fd;    /* donor file descriptor */
    __u64 orig_start;    /* logical start offset in block for orig */
    __u64 donor_start;    /* logical start offset in block for donor */
    __u64 len;    /* block length to be moved */
    __u64 moved_len;    /* moved block length */
};

int extents_before_defrag;
int extents_after_defrag;
unsigned int defraged_file_count;
__u8 log_groups_per_flex;
__u32 blocks_per_group;
__u32 feature_incompat;

#define HAVE_POSIX_FADVISE
#define HAVE_FALLOCATE64

/* Local definitions of some syscalls glibc may not yet have */
#ifndef HAVE_POSIX_FADVISE
#warning Using locally defined posix_fadvise interface.

#ifndef __NR_fadvise64_64
#error Your kernel headers dont define __NR_fadvise64_64
#endif

/*
 * fadvise() -        Give advice about file access.
 *
 * @fd:            defrag target file's descriptor.
 * @offset:        file offset.
 * @len:        area length.
 * @advise:        process flag.
 */
static int posix_fadvise(int fd, loff_t offset, size_t len, int advise)
{
    return syscall(__NR_fadvise64_64, fd, offset, len, advise);
}
#endif /* ! HAVE_FADVISE64_64 */

#ifndef HAVE_SYNC_FILE_RANGE
//#warning Using locally defined sync_file_range interface.

#ifndef __NR_sync_file_range
#ifndef __NR_sync_file_range2 /* ppc */
#error Your kernel headers dont define __NR_sync_file_range
#endif
#endif

/*
 * sync_file_range() -    Sync file region.
 *
 * @fd:            defrag target file's descriptor.
 * @offset:        file offset.
 * @length:        area length.
 * @flag:        process flag.
 */
int sync_file_range(int fd, loff_t offset, loff_t length, unsigned int flag)
{
#ifdef __NR_sync_file_range
    return syscall(__NR_sync_file_range, fd, offset, length, flag);
#else
    return syscall(__NR_sync_file_range2, fd, flag, offset, length);
#endif
}
#endif /* ! HAVE_SYNC_FILE_RANGE */

#ifndef HAVE_FALLOCATE64
#warning Using locally defined fallocate syscall interface.

#ifndef __NR_fallocate
#error Your kernel headers dont define __NR_fallocate
#endif

/*
 * fallocate64() -    Manipulate file space.
 *
 * @fd:            defrag target file's descriptor.
 * @mode:        process flag.
 * @offset:        file offset.
 * @len:        file size.
 */
static int fallocate64(int fd, int mode, loff_t offset, loff_t len)
{
    return syscall(__NR_fallocate, fd, mode, offset, len);
}
#endif /* ! HAVE_FALLOCATE */

/*
 * page_in_core() -    Get information on whether pages are in core.
 *
 * @fd:            defrag target file's descriptor.
 * @defrag_data:    data used for defrag.
 * @vec:        page state array.
 * @page_num:        page number.
 */
static int page_in_core(int fd, struct move_extent defrag_data,
            unsigned char **vec, unsigned int *page_num)
{
    long    pagesize;
    void    *page = NULL;
    loff_t    offset, end_offset, length;

    if (vec == NULL || *vec != NULL)
        return -1;

    pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize < 0)
        return -1;
    /* In mmap, offset should be a multiple of the page size */
    offset = (loff_t)defrag_data.orig_start * BLOCK_SIZE;
    length = (loff_t)defrag_data.len * BLOCK_SIZE;
    end_offset = offset + length;
    /* Round the offset down to the nearest multiple of pagesize */
    offset = (offset / pagesize) * pagesize;
    length = end_offset - offset;

    page = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, offset);
    if (page == MAP_FAILED)
        return -1;

    *page_num = 0;
    *page_num = (length + pagesize - 1) / pagesize;
    *vec = (unsigned char *)calloc(*page_num, 1);
    if (*vec == NULL) {
        munmap(page, length);
        return -1;
    }

    /* Get information on whether pages are in core */
    if (mincore(page, (size_t)length, *vec) == -1 ||
        munmap(page, length) == -1) {
        FREE(*vec);
        return -1;
    }

    return 0;
}

/*
 * defrag_fadvise() -    Predeclare an access pattern for file data.
 *
 * @fd:            defrag target file's descriptor.
 * @defrag_data:    data used for defrag.
 * @vec:        page state array.
 * @page_num:        page number.
 */
static int defrag_fadvise(int fd, struct move_extent defrag_data,
           unsigned char *vec, unsigned int page_num)
{
    int    flag = 1;
    long    pagesize = sysconf(_SC_PAGESIZE);
    int    fadvise_flag = POSIX_FADV_DONTNEED;
    int    sync_flag = SYNC_FILE_RANGE_WAIT_BEFORE |
            SYNC_FILE_RANGE_WRITE |
            SYNC_FILE_RANGE_WAIT_AFTER;
    unsigned int    i;
    loff_t    offset;

    if (pagesize < 1)
        return -1;

    offset = (loff_t)defrag_data.orig_start * BLOCK_SIZE;
    offset = (offset / pagesize) * pagesize;

    /* Sync file for fadvise process */
    if (sync_file_range(fd, offset,
        (loff_t)pagesize * page_num, sync_flag) < 0)
        return -1;

    /* Try to release buffer cache which this process used,
     * then other process can use the released buffer
     */
    for (i = 0; i < page_num; i++) {
        if ((vec[i] & 0x1) == 0) {
            offset += pagesize;
            continue;
        }
        if (posix_fadvise(fd, offset, pagesize, fadvise_flag) < 0) {
            if (flag) {
                LOGE("\tFailed to fadvise");
                flag = 0;
            }
        }
        offset += pagesize;
    }

    return 0;
}

/*
 * check_free_size() -    Check if there's enough disk space.
 *
 * @fd:            defrag target file's descriptor.
 * @file:        file name.
 * @blk_count:        file blocks.
 */
static int check_free_size(int fd, const char *file, ext4_fsblk_t blk_count)
{
    ext4_fsblk_t    free_blk_count;
    struct statfs64    fsbuf;

    if (fstatfs64(fd, &fsbuf) < 0) {
        LOGE("%s: Failed to get filesystem information", file);
        return -1;
    }

    free_blk_count = fsbuf.f_bfree;

    if (free_blk_count >= blk_count)
        return 0;

    return -ENOSPC;
}

/*
 * file_frag_count() -    Get file fragment count.
 *
 * @fd:            defrag target file's descriptor.
 */
static int file_frag_count(int fd)
{
    int    ret;
    struct fiemap    fiemap_buf;

    /* When fm_extent_count is 0,
     * ioctl just get file fragment count.
     */
    memset(&fiemap_buf, 0, sizeof(struct fiemap));
    fiemap_buf.fm_start = 0;
    fiemap_buf.fm_length = FIEMAP_MAX_OFFSET;
    fiemap_buf.fm_flags |= FIEMAP_FLAG_SYNC;

    ret = ioctl(fd, FS_IOC_FIEMAP, &fiemap_buf);
    if (ret < 0)
        return ret;

    return fiemap_buf.fm_mapped_extents;
}

/*
 * file_check() -    Check file's attributes.
 *
 * @fd:            defrag target file's descriptor.
 * @buf:        a pointer of the struct stat64.
 * @file:        file name.
 * @extents:        file extents.
 * @blk_count:        file blocks.
 */
static int file_check(int fd, const char *file,
        int extents, ext4_fsblk_t blk_count)
{
    int    ret;
    struct flock    lock;

    /* Write-lock check is more reliable */
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;

    /* Free space */
    ret = check_free_size(fd, file, blk_count);
    if (ret < 0) {
        if (ret == -ENOSPC) {
            LOGE("[%u] \"%s\"\t\t"
                "  extents: %d -> %d\n", defraged_file_count,
                file, extents, extents);
            LOGE("Defrag size is larger than filesystem's free space");
        }
        return -1;
    }

    /* Lock status */
    if (fcntl(fd, F_GETLK, &lock) < 0) {
        LOGE("Failed to get %s lock information", file);
        return -1;
    } else if (lock.l_type != F_UNLCK) {
        LOGE("File %s has been locked", file);
        return -1;
    }

    return 0;
}

/*
 * insert_extent_by_logical() -    Sequentially insert extent by logical.
 *
 * @ext_list_head:    the head of logical extent list.
 * @ext:        the extent element which will be inserted.
 */
static int insert_extent_by_logical(struct fiemap_extent_list **ext_list_head,
            struct fiemap_extent_list *ext)
{
    struct fiemap_extent_list    *ext_list_tmp = *ext_list_head;

    if (ext == NULL)
        goto out;

    /* First element */
    if (*ext_list_head == NULL) {
        (*ext_list_head) = ext;
        (*ext_list_head)->prev = *ext_list_head;
        (*ext_list_head)->next = *ext_list_head;
        return 0;
    }

    if (ext->data.logical <= ext_list_tmp->data.logical) {
        /* Insert before head */
        if (ext_list_tmp->data.logical <
            ext->data.logical + ext->data.len)
            /* Overlap */
            goto out;
        /* Adjust head */
        *ext_list_head = ext;
    } else {
        /* Insert into the middle or last of the list */
        do {
            if (ext->data.logical < ext_list_tmp->data.logical)
                break;
            ext_list_tmp = ext_list_tmp->next;
        } while (ext_list_tmp != (*ext_list_head));
        if (ext->data.logical <
            ext_list_tmp->prev->data.logical +
            ext_list_tmp->prev->data.len)
            /* Overlap */
            goto out;

        if (ext_list_tmp != *ext_list_head &&
            ext_list_tmp->data.logical <
            ext->data.logical + ext->data.len)
            /* Overlap */
            goto out;
    }
    ext_list_tmp = ext_list_tmp->prev;
    /* Insert "ext" after "ext_list_tmp" */
    insert(ext_list_tmp, ext);
    return 0;
out:
    errno = EINVAL;
    return -1;
}

/*
 * insert_extent_by_physical() -    Sequentially insert extent by physical.
 *
 * @ext_list_head:    the head of physical extent list.
 * @ext:        the extent element which will be inserted.
 */
static int insert_extent_by_physical(struct fiemap_extent_list **ext_list_head,
            struct fiemap_extent_list *ext)
{
    struct fiemap_extent_list    *ext_list_tmp = *ext_list_head;

    if (ext == NULL)
        goto out;

    /* First element */
    if (*ext_list_head == NULL) {
        (*ext_list_head) = ext;
        (*ext_list_head)->prev = *ext_list_head;
        (*ext_list_head)->next = *ext_list_head;
        return 0;
    }

    if (ext->data.physical <= ext_list_tmp->data.physical) {
        /* Insert before head */
        if (ext_list_tmp->data.physical <
                    ext->data.physical + ext->data.len)
            /* Overlap */
            goto out;
        /* Adjust head */
        *ext_list_head = ext;
    } else {
        /* Insert into the middle or last of the list */
        do {
            if (ext->data.physical < ext_list_tmp->data.physical)
                break;
            ext_list_tmp = ext_list_tmp->next;
        } while (ext_list_tmp != (*ext_list_head));
        if (ext->data.physical <
            ext_list_tmp->prev->data.physical +
                ext_list_tmp->prev->data.len)
            /* Overlap */
            goto out;

        if (ext_list_tmp != *ext_list_head &&
            ext_list_tmp->data.physical <
                ext->data.physical + ext->data.len)
            /* Overlap */
            goto out;
    }
    ext_list_tmp = ext_list_tmp->prev;
    /* Insert "ext" after "ext_list_tmp" */
    insert(ext_list_tmp, ext);
    return 0;
out:
    errno = EINVAL;
    return -1;
}

/*
 * insert_exts_group() -    Insert a exts_group.
 *
 * @ext_group_head:        the head of a exts_group list.
 * @exts_group:            the exts_group element which will be inserted.
 */
static int insert_exts_group(struct fiemap_extent_group **ext_group_head,
                struct fiemap_extent_group *exts_group)
{
    struct fiemap_extent_group    *ext_group_tmp = NULL;

    if (exts_group == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Initialize list */
    if (*ext_group_head == NULL) {
        (*ext_group_head) = exts_group;
        (*ext_group_head)->prev = *ext_group_head;
        (*ext_group_head)->next = *ext_group_head;
        return 0;
    }

    ext_group_tmp = (*ext_group_head)->prev;
    insert(ext_group_tmp, exts_group);

    return 0;
}

/*
 * join_extents() -        Find continuous region(exts_group).
 *
 * @ext_list_head:        the head of the extent list.
 * @ext_group_head:        the head of the target exts_group list.
 */
static int join_extents(struct fiemap_extent_list *ext_list_head,
        struct fiemap_extent_group **ext_group_head)
{
    __u64    len = ext_list_head->data.len;
    struct fiemap_extent_list *ext_list_start = ext_list_head;
    struct fiemap_extent_list *ext_list_tmp = ext_list_head->next;

    do {
        struct fiemap_extent_group    *ext_group_tmp = NULL;

        /* This extent and previous extent are not continuous,
         * so, all previous extents are treated as an extent group.
         */
        if ((ext_list_tmp->prev->data.logical +
            ext_list_tmp->prev->data.len)
                != ext_list_tmp->data.logical) {
            ext_group_tmp =
                (struct fiemap_extent_group *)malloc(sizeof(struct fiemap_extent_group));
            if (ext_group_tmp == NULL)
                return -1;

            memset(ext_group_tmp, 0,
                sizeof(struct fiemap_extent_group));
            ext_group_tmp->len = len;
            ext_group_tmp->start = ext_list_start;
            ext_group_tmp->end = ext_list_tmp->prev;

            if (insert_exts_group(ext_group_head,
                ext_group_tmp) < 0) {
                FREE(ext_group_tmp);
                return -1;
            }
            ext_list_start = ext_list_tmp;
            len = ext_list_tmp->data.len;
            ext_list_tmp = ext_list_tmp->next;
            continue;
        }

        /* This extent and previous extent are continuous,
         * so, they belong to the same extent group, and we check
         * if the next extent belongs to the same extent group.
         */
        len += ext_list_tmp->data.len;
        ext_list_tmp = ext_list_tmp->next;
    } while (ext_list_tmp != ext_list_head->next);

    return 0;
}

/*
 * get_file_extents() -    Get file's extent list.
 *
 * @fd:            defrag target file's descriptor.
 * @ext_list_head:    the head of the extent list.
 */
static int get_file_extents(int fd, struct fiemap_extent_list **ext_list_head)
{
    __u32    i;
    int    ret;
    int    ext_buf_size, fie_buf_size;
    __u64    pos = 0;
    struct fiemap    *fiemap_buf = NULL;
    struct fiemap_extent    *ext_buf = NULL;
    struct fiemap_extent_list    *ext_list = NULL;

    /* Convert units, in bytes.
     * Be careful : now, physical block number in extent is 48bit,
     * and the maximum blocksize for ext4 is 4K(12bit),
     * so there is no overflow, but in future it may be changed.
     */

    /* Alloc space for fiemap */
    ext_buf_size = EXTENT_MAX_COUNT * sizeof(struct fiemap_extent);
    fie_buf_size = sizeof(struct fiemap) + ext_buf_size;

    fiemap_buf = (struct fiemap *)malloc(fie_buf_size);
    if (fiemap_buf == NULL)
        return -1;

    ext_buf = fiemap_buf->fm_extents;
    memset(fiemap_buf, 0, fie_buf_size);
    fiemap_buf->fm_length = FIEMAP_MAX_OFFSET;
    fiemap_buf->fm_flags |= FIEMAP_FLAG_SYNC;
    fiemap_buf->fm_extent_count = EXTENT_MAX_COUNT;

    do {
        fiemap_buf->fm_start = pos;
        memset(ext_buf, 0, ext_buf_size);
        ret = ioctl(fd, FS_IOC_FIEMAP, fiemap_buf);
        if (ret < 0 || fiemap_buf->fm_mapped_extents == 0)
            goto out;
        for (i = 0; i < fiemap_buf->fm_mapped_extents; i++) {
            ext_list = NULL;
            ext_list = (struct fiemap_extent_list *)malloc(sizeof(struct fiemap_extent_list));
            if (ext_list == NULL)
                goto out;

            ext_list->data.physical = ext_buf[i].fe_physical
                        / BLOCK_SIZE;
            ext_list->data.logical = ext_buf[i].fe_logical
                        / BLOCK_SIZE;
            ext_list->data.len = ext_buf[i].fe_length
                        / BLOCK_SIZE;

            ret = insert_extent_by_physical(
                    ext_list_head, ext_list);
            if (ret < 0) {
                FREE(ext_list);
                goto out;
            }
        }
        /* Record file's logical offset this time */
        pos = ext_buf[EXTENT_MAX_COUNT-1].fe_logical +
            ext_buf[EXTENT_MAX_COUNT-1].fe_length;
        /*
         * If fm_extents array has been filled and
         * there are extents left, continue to cycle.
         */
    } while (fiemap_buf->fm_mapped_extents
                    == EXTENT_MAX_COUNT &&
        !(ext_buf[EXTENT_MAX_COUNT-1].fe_flags
                    & FIEMAP_EXTENT_LAST));

    FREE(fiemap_buf);
    return 0;
out:
    FREE(fiemap_buf);
    return -1;
}

/*
 * get_logical_count() -    Get the file logical extents count.
 *
 * @logical_list_head:    the head of the logical extent list.
 */
static int get_logical_count(struct fiemap_extent_list *logical_list_head)
{
    int ret = 0;
    struct fiemap_extent_list *ext_list_tmp  = logical_list_head;

    do {
        ret++;
        ext_list_tmp = ext_list_tmp->next;
    } while (ext_list_tmp != logical_list_head);

    return ret;
}

/*
 * get_physical_count() -    Get the file physical extents count.
 *
 * @physical_list_head:    the head of the physical extent list.
 */
static int get_physical_count(struct fiemap_extent_list *physical_list_head)
{
    int ret = 0;
    struct fiemap_extent_list *ext_list_tmp = physical_list_head;

    do {
        if ((ext_list_tmp->data.physical + ext_list_tmp->data.len)
                != ext_list_tmp->next->data.physical) {
            /* This extent and next extent are not continuous. */
            ret++;
        }

        ext_list_tmp = ext_list_tmp->next;
    } while (ext_list_tmp != physical_list_head);

    return ret;
}

/*
 * change_physical_to_logical() -    Change list from physical to logical.
 *
 * @physical_list_head:    the head of physical extent list.
 * @logical_list_head:    the head of logical extent list.
 */
static int change_physical_to_logical(
            struct fiemap_extent_list **physical_list_head,
            struct fiemap_extent_list **logical_list_head)
{
    int ret;
    struct fiemap_extent_list *ext_list_tmp = *physical_list_head;
    struct fiemap_extent_list *ext_list_next = ext_list_tmp->next;

    while (1) {
        if (ext_list_tmp == ext_list_next) {
            ret = insert_extent_by_logical(
                logical_list_head, ext_list_tmp);
            if (ret < 0)
                return -1;

            *physical_list_head = NULL;
            break;
        }

        ext_list_tmp->prev->next = ext_list_tmp->next;
        ext_list_tmp->next->prev = ext_list_tmp->prev;
        *physical_list_head = ext_list_next;

        ret = insert_extent_by_logical(
            logical_list_head, ext_list_tmp);
        if (ret < 0) {
            FREE(ext_list_tmp);
            return -1;
        }
        ext_list_tmp = ext_list_next;
        ext_list_next = ext_list_next->next;
    }

    return 0;
}

/* get_file_blocks() -  Get total file blocks.
 *
 * @ext_list_head:    the extent list head of the target file
 */
static ext4_fsblk_t get_file_blocks(struct fiemap_extent_list *ext_list_head)
{
    ext4_fsblk_t blk_count = 0;
    struct fiemap_extent_list *ext_list_tmp = ext_list_head;

    do {
        blk_count += ext_list_tmp->data.len;
        ext_list_tmp = ext_list_tmp->next;
    } while (ext_list_tmp != ext_list_head);

    return blk_count;
}

/*
 * free_ext() -        Free the extent list.
 *
 * @ext_list_head:    the extent list head of which will be free.
 */
static void free_ext(struct fiemap_extent_list *ext_list_head)
{
    struct fiemap_extent_list    *ext_list_tmp = NULL;

    if (ext_list_head == NULL)
        return;

    while (ext_list_head->next != ext_list_head) {
        ext_list_tmp = ext_list_head;
        ext_list_head->prev->next = ext_list_head->next;
        ext_list_head->next->prev = ext_list_head->prev;
        ext_list_head = ext_list_head->next;
        free(ext_list_tmp);
    }
    free(ext_list_head);
}

/*
 * free_exts_group() -        Free the exts_group.
 *
 * @*ext_group_head:    the exts_group list head which will be free.
 */
static void free_exts_group(struct fiemap_extent_group *ext_group_head)
{
    struct fiemap_extent_group    *ext_group_tmp = NULL;

    if (ext_group_head == NULL)
        return;

    while (ext_group_head->next != ext_group_head) {
        ext_group_tmp = ext_group_head;
        ext_group_head->prev->next = ext_group_head->next;
        ext_group_head->next->prev = ext_group_head->prev;
        ext_group_head = ext_group_head->next;
        free(ext_group_tmp);
    }
    free(ext_group_head);
}

/*
 * get_best_count() -    Get the file best extents count.
 *
 * @block_count:        the file's physical block count.
 */
static int get_best_count(ext4_fsblk_t block_count)
{
    int ret;
    unsigned int flex_bg_num;

    /* Calcuate best extents count */
    if (feature_incompat & EXT4_FEATURE_INCOMPAT_FLEX_BG) {
        flex_bg_num = 1 << log_groups_per_flex;
        ret = ((block_count - 1) /
            ((ext4_fsblk_t)blocks_per_group *
                flex_bg_num)) + 1;
    } else
        ret = ((block_count - 1) / blocks_per_group) + 1;

    return ret;
}

/*
 * print_progress -    Print defrag progress
 *
 * @file:        file name.
 * @start:        logical offset for defrag target file
 * @file_size:        defrag target filesize

static void print_progress(const char *file, loff_t start, loff_t file_size)
{
    int percent = (start * 100) / file_size;
    LOGD("[%u]%s:\t%3d%%", defraged_file_count, file, min(percent, 100));
    return;
}
*/

/*
 * call_defrag() -    Execute the defrag program.
 *
 * @fd:            target file descriptor.
 * @donor_fd:        donor file descriptor.
 * @file:            target file name.
 * @buf:            pointer of the struct stat64.
 * @ext_list_head:    head of the extent list.
 */
static int call_defrag(int fd, int donor_fd, const char *file,
    const struct stat64 *buf, struct fiemap_extent_list *ext_list_head)
{
    loff_t    start = 0;
    unsigned int    page_num;
    unsigned char    *vec = NULL;
    int    defraged_ret = 0;
    int    ret;
    struct move_extent    move_data;
    struct fiemap_extent_list    *ext_list_tmp = NULL;

    memset(&move_data, 0, sizeof(struct move_extent));
    move_data.donor_fd = donor_fd;

    /* Print defrag progress */
    //print_progress(file, start, buf->st_size);

    ext_list_tmp = ext_list_head;
    do {
        move_data.orig_start = ext_list_tmp->data.logical;
        /* Logical offset of orig and donor should be same */
        move_data.donor_start = move_data.orig_start;
        move_data.len = ext_list_tmp->data.len;
        move_data.moved_len = 0;

        ret = page_in_core(fd, move_data, &vec, &page_num);
        if (ret < 0) {
            LOGE("Failed to get file %s map, %s", file, strerror(errno));
            g_donor_call_defrag_estat[DFILE_PAGE_IN_CORE]++;
            return -1;
        }

        /* EXT4_IOC_MOVE_EXT */
        defraged_ret =
            ioctl(fd, EXT4_IOC_MOVE_EXT, &move_data);

        /* Free pages */
        ret = defrag_fadvise(fd, move_data, vec, page_num);
        if (vec) {
            free(vec);
            vec = NULL;
        }
        if (ret < 0) {
            LOGE("%s: Failed to free page, %s", file, strerror(errno));
            g_donor_call_defrag_estat[DFILE_FREE_PAGES]++;
            return -2;
        }

        if (defraged_ret < 0) {
            LOGE("%s: Failed to defrag with EXT4_IOC_MOVE_EXT ioctl, %s", file, strerror(errno));
            LOGE("%s: orig_start=%llu,len=%llu,moved_len=%llu,move_data.donor_start=%llu\n",
                   file, move_data.orig_start, move_data.len, move_data.moved_len, move_data.donor_start);
            if (errno == ENOTTY)
                LOGE("\tAt least 2.6.31-rc1 of vanilla kernel is required\n");
            g_donor_call_defrag_estat[DFILE_IOC_MOVE_EXT]++;
            return -3;
        }

        pthread_mutex_lock(&stop_mutex);
        if (g_stop_flag == 1) {
            pthread_mutex_unlock(&stop_mutex);
            log.markExitReason("stop_3");
            LOGI("user cancel event happen when move extents");
            return -4;
        }
        pthread_mutex_unlock(&stop_mutex);

        /* Adjust logical offset for next ioctl */
        move_data.orig_start += move_data.moved_len;
        move_data.donor_start = move_data.orig_start;

        start = move_data.orig_start * buf->st_blksize;

        /* Print defrag progress */
        //print_progress(file, start, buf->st_size);

        /* End of file */
        if (start >= buf->st_size)
            break;

        ext_list_tmp = ext_list_tmp->next;
    } while (ext_list_tmp != ext_list_head);

    return 0;
}

static long getTime() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return(tv.tv_sec*1000 + tv.tv_usec/1000);  //ms
}


static inline int need_defrag(const char *file)
{
    int i, suffix_len, name_len;
    int length;

    length = sizeof(extension_lists) / sizeof(extension_lists[0]);
    name_len = strlen(file);

    for (i = 0; i < length; i++) {
        suffix_len = strlen(extension_lists[i]);

        if(name_len <= suffix_len)
            continue;

        if (strcmp(file + strlen(file) - suffix_len, extension_lists[i]) == 0)
            return 1;
    }

    return 0;
}

/*
 * file_defrag() -        Check file attributes and call ioctl to defrag.
 *
 * @file:        the file's name.
 * @buf:        the pointer of the struct stat64.
 * @flag:        file type.
 * @ftwbuf:        the pointer of a struct FTW.
 */
static int file_defrag(const char *file, const struct stat64 *buf,
            int flag EXT2FS_ATTR((unused)),
            struct FTW *ftwbuf EXT2FS_ATTR((unused)))
{
    int fd;
    int ret;
    int best;
    int file_frags_start = -1, file_frags_end = -1;
    int orig_physical_cnt, donor_physical_cnt = 0;
    ext4_fsblk_t        blk_count = 0;
    struct fiemap_extent_list *orig_list_physical = NULL;
    struct fiemap_extent_list *orig_list_logical = NULL;
    struct fiemap_extent_list *donor_list_physical = NULL;
    struct fiemap_extent_list *donor_list_logical = NULL;
    struct fiemap_extent_group *orig_group_head = NULL;
    struct fiemap_extent_group *orig_group_tmp = NULL;
    long startTime = 0, endTime = 0, delay = 0;
    unsigned int cksum_before, cksum_after;
    char ck_file[PATH_MAX] = {0};

    if (!S_ISREG(buf->st_mode)) {
        LOGD("File %s is not regular file", file);
        return 0;
    }

    if (only_defrag_extension == 1 && !need_defrag(file)) {
        LOGD("File %s need not do defrag because it isn't in defrag file extension lists", file);
        return 0;
    }

    defraged_file_count++;
    LOGD("[%u]:%s", defraged_file_count, file);

    g_defrag_stat[FILE_REG]++;

    /* Empty file */
    if (buf->st_size == 0) {
        LOGD("File %s size is 0 or above 1GB", file);
        g_defrag_stat[FILE_SIZE_ZERO]++;
        return 0;
    }

    /* Has no blocks */
    if (buf->st_blocks == 0) {
        LOGD("File %s has no blocks", file);
        g_defrag_stat[FILE_BLOCKS_ZERO]++;
        return 0;
    }

    startTime = getTime();
    if (data_verify) {
        strcpy(ck_file, file);
        ret = cksum_generate(ck_file, &cksum_before);
        if (ret < 0) {
            LOGE("cksum_generate befor defrag error");
            return 0;
        } else {
            LOGV("before defrag checksum is %u", cksum_before);
        }
    }

    fd = open64(file, O_RDWR);
    if (fd < 0) {
        LOGI("Failed to open %s", file);
        g_defrag_stat[FILE_OPEN_FAIL]++;
        return 0;
    }

    log.traceFileBegin(string(file));

    /* Get file's extents */
    ret = get_file_extents(fd, &orig_list_physical);
    if (ret < 0) {
        LOGE("Failed to get file %s extents", file);
        g_defrag_stat[FILE_GET_EXT]++;
        goto out;
    }

    /* Get the count of file's continuous physical region */
    orig_physical_cnt = get_physical_count(orig_list_physical);

    if((max_sectors_kb != 0) && (orig_physical_cnt < (buf->st_size / (max_sectors_kb * 1024)))) {
        LOGE("file %s need not do defrag because its fragmentation is not serious", file);
        g_defrag_stat[DONOR_NO_DEFRAG_OK]++;
        /* in uploaded csv file, -2 means the reason why this file don't do defrag
           is that its fragmentation is not serious
        */
        file_frags_end = -2;
        goto out;
    }

    /* Change list from physical to logical */
    ret = change_physical_to_logical(&orig_list_physical,
                &orig_list_logical);
    if (ret < 0) {
        LOGE("Failed to change file %s extents", file);
        g_defrag_stat[FILE_CHG_PHY_TO_LOG]++;
        goto out;
    }

    /* Count file fragments before defrag */
    file_frags_start = get_logical_count(orig_list_logical);

    blk_count = get_file_blocks(orig_list_logical);
    if (file_check(fd, file, file_frags_start, blk_count) < 0) {
        g_defrag_stat[FILE_CHECK]++;
        goto out;
    }

    if (fsync(fd) < 0) {
        LOGE("Failed to sync(fsync) %s", file);
        g_defrag_stat[FILE_SYNC]++;
        goto out;
    }

    best = get_best_count(blk_count);

    if (file_frags_start <= best) {
        LOGE("need not do defrag because that file_frags_start:%d, best:%d",
                file_frags_start, best);
        goto check_improvement;
    }
    /* Combine extents to group */
    ret = join_extents(orig_list_logical, &orig_group_head);
    if (ret < 0) {
        LOGE("Failed to join file %s extents", file);
        g_defrag_stat[DONOR_FILE_JOIN_EXT]++;
        goto out;
    }

    /* Allocate space for donor inode */
    orig_group_tmp = orig_group_head;
    do {
        ret = fallocate64(donor_fd, 0,
          (loff_t)orig_group_tmp->start->data.logical * BLOCK_SIZE,
          (loff_t)orig_group_tmp->len * BLOCK_SIZE);
        if (ret < 0) {
            LOGE("Failed to fallocate");
            g_defrag_stat[DONOR_FILE_FALLOCATE]++;
            goto out;
        }

        orig_group_tmp = orig_group_tmp->next;
    } while (orig_group_tmp != orig_group_head);

    /* Get donor inode's extents */
    ret = get_file_extents(donor_fd, &donor_list_physical);
    if (ret < 0) {
        LOGE("Failed to get file extents");
        g_defrag_stat[DONOR_FILE_GET_EXT]++;
        goto out;
    }

    /* Calcuate donor inode's continuous physical region */
    donor_physical_cnt = get_physical_count(donor_list_physical);

    /* Change donor extent list from physical to logical */
    ret = change_physical_to_logical(&donor_list_physical,
            &donor_list_logical);
    if (ret < 0) {
        LOGE("Failed to change file extents");
        g_defrag_stat[DONOR_FILE_CHG_PHY_TO_LOG]++;
        goto out;
    }

check_improvement:
    extents_before_defrag += file_frags_start;

    /* make critical decision if do below defrag work or not */
    if (file_frags_start <= best ||
            (orig_physical_cnt / 2) <= donor_physical_cnt) {
        g_defrag_stat[DONOR_NO_DEFRAG_OK]++;

        if ((orig_physical_cnt / 2) <= donor_physical_cnt) {
            LOGD("need not do defrag because that orig_physical_cnt:%d"
                 " donor_physical_cnt:%d",
                 orig_physical_cnt / 2, donor_physical_cnt);
        }

        extents_after_defrag += file_frags_start;
        goto out;
    }

    /* Defrag the file */
    ret = call_defrag(fd, donor_fd, file, buf, donor_list_logical);

    /* Count file fragments after defrag and print extents info */
    file_frags_end = file_frag_count(fd);
    if (file_frags_end < 0) {
        LOGE("Failed to get file %s information", file);
        goto out;
    }

    extents_after_defrag += file_frags_end;

    if (ret < 0) {
        g_defrag_stat[DONOR_DEFRAG_FAIL]++;
        goto out;
    }

    //call fsync
    if (fsync(fd) < 0) {
        LOGE("Failed to sync(fsync) after defrag");
        g_defrag_stat[FILE_SYNC]++;
        goto out;
    }

    endTime = getTime();
    delay = endTime - startTime;
    LOGD("after call_defrag file  extents: %d -> %d, file size: %lld, time cost: %ldms",
            file_frags_start, file_frags_end, buf->st_size, delay);

    LOGD("\t[ OK ]\n");
    g_defrag_stat[DONOR_DEFRAG_OK]++;

out:
    if (ftruncate(donor_fd, 0) < 0) {
        LOGE("ftruncate donor file error");
        log.markExitReason("ftruncate_donor_file_err");
        ret = -1;
        goto stop_whole_defrag;
    }

    pthread_mutex_lock(&stop_mutex);
    if (g_stop_flag == 1) {
        pthread_mutex_unlock(&stop_mutex);
        LOGI("user cancel event happen when file defrag");
        log.markExitReason("stop_0");
        ret = -1;
        goto stop_whole_defrag;
    }
    pthread_mutex_unlock(&stop_mutex);

    ret = 0;  //arrive at out ,means suceess.

stop_whole_defrag:

    close(fd);
    if (data_verify) {
        if (cksum_generate(ck_file, &cksum_after) < 0) {
            LOGE("cksum_generate after defrag error");
        } else {
            LOGV("after defrag checksum is %u", cksum_after);
        }

        if(cksum_before != cksum_after) {
            LOGE("%s checksum before and after defrag is inconsistent, cksum_before %u, cksum_after %u", ck_file, cksum_before, cksum_after);
        }
    }
    free_ext(orig_list_physical);
    free_ext(orig_list_logical);
    free_ext(donor_list_physical);
    free_exts_group(orig_group_head);

    log.traceFileEnd(file_frags_start, file_frags_end);

    return ret;
}

void defrag_file(char *file) {
    struct stat64 buf;

    pthread_mutex_lock(&stop_mutex);
    if (g_stop_flag == 1) {
        pthread_mutex_unlock(&stop_mutex);
        LOGI("user cancel dir defrag");
        log.markExitReason("stop_1");
        return;
    }
    pthread_mutex_unlock(&stop_mutex);

    if (lstat64(file, &buf) < 0) {
        LOGI("fail to lstat file %s, %s", file, strerror(errno));
        log.markExitReason("defrag_file_lstat_err");
        return;
    }

    /* only defrag reg file */
    if (S_ISREG(buf.st_mode)) {
        /* File */
        if (access(file, R_OK) < 0) {
            LOGE("%s fail to access %s, %s",__func__, file, strerror(errno));
            log.markExitReason("defrag_file_access_err");
            return;
        }
    } else {
        LOGI("%s:not reg file", file);
        log.markExitReason("defrag_file_non_reg_file");
        return;
    }

    file_defrag((const char *)file, (const struct stat64 *)(&buf), 0, NULL);
}

void defrag_dir(char *dir) {
    char real_dir[PATH_MAX];
    struct stat64 buf;
    int flags = FTW_PHYS | FTW_MOUNT;

    pthread_mutex_lock(&stop_mutex);
    if (g_stop_flag == 1) {
        pthread_mutex_unlock(&stop_mutex);
        LOGI("user cancel dir defrag");
        log.markExitReason("stop_2");
        return;
    }
    pthread_mutex_unlock(&stop_mutex);

    memset(real_dir, 0, PATH_MAX);

    if (lstat64(dir, &buf) < 0) {
        LOGI("fail to lstat dir %s, %s", dir, strerror(errno));
        log.markExitReason("defrag_dir_lstat_err");
        return;
    }

    /* Handle i.e. lvm device symlinks */
    if (S_ISLNK(buf.st_mode)) {
        struct stat64 buf2;

        if (stat64(dir, &buf2) == 0 &&
            S_ISBLK(buf2.st_mode))
            buf = buf2;
    }

    /* only defrag dir file */
    if (S_ISDIR(buf.st_mode)) {
        /* Directory */
        if (access(dir, R_OK) < 0) {
            LOGE("%s fail to access %s, %s",__func__, dir, strerror(errno));
            log.markExitReason("defrag_dir_access_err");
            return;
        }
    } else {
        LOGI("%s:not dir file", dir);
        log.markExitReason("defrag_dir_non_dir_file");
        return;
    }

    /* For device case,
     * filesystem type checked in get_mount_point()
     */
    if (realpath(dir, real_dir) == NULL) {
        LOGE("%s:Couldn't get full path", dir);
        log.markExitReason("defrag_dir_realpath_err");
        return;
    }

    /* File tree walk */
    nftw64(real_dir, file_defrag, FTW_OPEN_FD, flags);

    return;

}
