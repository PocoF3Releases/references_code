/*
 * storage_compact.cpp - ext4 filesystem block group compacter
 *
 * Copyright (C) Xiaomi Inc.
 *
 */
#define LOG_TAG "FragCompact"

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

//#include "config.h"
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
#include <sys/ioctl.h>
#include <ext2fs/fiemap.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/vfs.h>
#include <pthread.h>
#include <cutils/properties.h>

#include "e2fsprogs_modified/cache.h"
#include "e2fsprogs_modified/file_check.h"
#include "storage_compact.h"
#include "../utils.h"
#include "../log.h"

#include <string>
using namespace std;

extern CompactLogger log;

/* A relatively new ioctl interface ... */
#ifndef EXT4_IOC_MOVE_EXT
#define EXT4_IOC_MOVE_EXT      _IOWR('f', 15, struct move_extent)
#endif

#define FALLOC_FL_RESERVE_RANGE		0x80

/* Wrap up the free function */
#define FREE(tmp)                \
    do {                    \
        if ((tmp) != NULL)        \
            free(tmp);        \
    } while (0)                \

#define BLOCK_SIZE                     4096
#define COMPACT_LAST_GROUP             "persist.sys.mem_clast_group"
/*
 * Minimum free blocks required for block group defrag.
 */
#define MIN_GROUP_FREE_BLOCKS_THRESHOLD             256

/*
 * Minimum fragments required for block group defrag.
 */
#define MIN_GROUP_FRAGS_THRESHOLD                   4

/*
 * Minimum free space percentage required to perform defrag operation
 */
#define MIN_FREE_SPACE_PERCENTAGE_THRESHOLD         10

/*
 * If average free blocks size larger than 2M, don't touch it
 */
#define AVERAGE_FREE_BLOCKS_LENGTH                  512


struct move_extent {
    __s32 reserved;    /* original file descriptor */
    __u32 donor_fd;    /* donor file descriptor */
    __u64 orig_start;    /* logical start offset in block for orig */
    __u64 donor_start;    /* logical start offset in block for donor */
    __u64 len;    /* block length to be moved */
    __u64 moved_len;    /* moved block length */
};

struct defrag_range {
    __u64 free_start;
    __u64 free_end;
    __u64 used_start;
    __u64 used_end;
    unsigned long length;
    unsigned int group;
};

struct pa_address {
    int pa_group;
    int pa_offset;
};

struct fallocate_reserved_range {
    int mode;
    loff_t offset;
    loff_t len;
    struct pa_address pa_addr;
};

#define EXT4_IOC_DEFRAG_RANGE      _IOWR('f', 22, struct defrag_range)
#define EXT4_IOC_FALLOCATE_RESERVE _IOW('f', 23, struct fallocate_reserved_range)

#define HAVE_POSIX_FADVISE64
#define HAVE_FALLOCATE64

/*
 * We prefer posix_fadvise64 when available, as it allows 64bit offset on
 * 32bit systems
 */
#if defined(HAVE_POSIX_FADVISE64)
#define posix_fadvise    posix_fadvise64
#elif defined(HAVE_FADVISE64)
#define posix_fadvise    fadvise64
#elif !defined(HAVE_POSIX_FADVISE)
#error posix_fadvise not available!
#endif

#ifndef HAVE_FALLOCATE64
#error fallocate64 not available!
#endif /* ! HAVE_FALLOCATE64 */

int dry_run;
int data_verify;
char *data_path;
int donor_fd;
int     num_swap_extents;
struct swap_block_info *swap_blocks_info;

#ifndef HAVE_SYNC_FILE_RANGE
//#warning Using locally defined sync_file_range interface.

#ifndef __NR_sync_file_range
#ifndef __NR_sync_file_range2 /* ppc */
#error Your kernel headers dont define __NR_sync_file_range
#endif
#endif

/*
 * sync_file_range() -        Sync file region.
 *
 * @fd:                        defrag target file's descriptor.
 * @offset:                file offset.
 * @length:                area length.
 * @flag:                process flag.
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
        return -2;
    // In mmap, offset should be a multiple of the page size
    offset = (loff_t)defrag_data.orig_start * BLOCK_SIZE;
    length = (loff_t)defrag_data.len * BLOCK_SIZE;
    end_offset = offset + length;
    // Round the offset down to the nearest multiple of pagesize
    offset = (offset / pagesize) * pagesize;
    length = end_offset - offset;

    page = mmap(NULL, length, PROT_READ, MAP_SHARED, fd, offset);
    if (page == MAP_FAILED)
        return -3;

    *page_num = 0;
    *page_num = (length + pagesize - 1) / pagesize;
    *vec = (unsigned char *)calloc(*page_num, 1);
    if (*vec == NULL) {
        munmap(page, length);
        return -4;
    }

    // Get information on whether pages are in core
    if (mincore(page, (size_t)length, *vec) == -1 ||
        munmap(page, length) == -1) {
        FREE(*vec);
        return -5;
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

    // Sync file for fadvise process
    if (sync_file_range(fd, offset, (loff_t)pagesize * page_num, sync_flag) < 0) {
        return -2;
    }

    for (i = 0; i < page_num; i++) {
        if ((vec[i] & 0x1) == 0) {
            offset += pagesize;
            continue;
        }

        if (posix_fadvise(fd, offset, pagesize, fadvise_flag) < 0) {
            perror("\tFailed to fadvise");
            flag = 0;
        }
        offset += pagesize;
    }

    return 0;
}

/*
 * call_defrag() -    Execute the defrag program.
 *
 * @fd:            target file descriptor.
 * @donor_fd:        donor file descriptor.
 * @file:            target file name.
 * @buf:            pointer of the struct stat64.
 * @ext_list_head:    head of the extent list.
 */
static int call_defrag(int fd, int donor_fd, int o_offset, int d_offset, int len)
{
    unsigned int    page_num;
    unsigned char    *vec = NULL;
    int    defraged_ret = 0;
    int    ret;
    struct move_extent    move_data;

    memset(&move_data, 0, sizeof(struct move_extent));
    move_data.donor_fd = donor_fd;

    //LOGD("call_defrag: o_offset is %d, d_offset is %d, len is %d", o_offset, d_offset, len);
    move_data.orig_start = o_offset;
    /* Logical offset of orig and donor should be same */
    move_data.donor_start = d_offset;
    move_data.len = len;
    move_data.moved_len = 0;

    ret = page_in_core(fd, move_data, &vec, &page_num);
    if (ret < 0) {
        LOGE("Failed to get file map,ret %d", ret);
        return -3;
    }

    /* EXT4_IOC_MOVE_EXT */
    defraged_ret = ioctl(fd, EXT4_IOC_MOVE_EXT, &move_data);

    /* Free pages */
    ret = defrag_fadvise(fd, move_data, vec, page_num);
    if (vec) {
        free(vec);
        vec = NULL;
    }

    if (ret < 0) {
        LOGE("Failed to free page ret %d", ret);
        return -1;
    }

    if (defraged_ret < 0) {
        LOGE("Failed to defrag with "
                "EXT4_IOC_MOVE_EXT ioctl ret %d, %s, move_data: orig_start %llu, donor_start %llu, len %llu, moved_len %llu", 
               defraged_ret, strerror(errno), move_data.orig_start, move_data.donor_start, move_data.len, move_data.moved_len);
        return -2;
    }

    if (((__u64)len) != move_data.moved_len) {
        LOGE("move_data.moved_len is %llu", move_data.moved_len);
        return -4;
    }

    return 0;
}

int swap_extent(int donor_fd, int ii, unsigned long swap_block_nums)
{
    char o_filename[PATH_MAX] = {0};
    int ret, fd;
    unsigned int cksum_before, cksum_after;

    strcpy(o_filename, data_path);
    strcat(o_filename, swap_blocks_info[ii].filename);
    LOGV("get file is %s", o_filename);

    if (dry_run)
        return 0;

    if (data_verify) {
        ret = cksum_generate(o_filename, &cksum_before);
        if (ret < 0) {
            LOGE("cksum_generate befor defrag error");
            return -4;
        } else {
            LOGV("before defrag checksum is %u", cksum_before);
        }
    }

    fd = open64(o_filename, O_RDWR);
    if (fd < 0) {
        if (errno != EACCES) {
            LOGE("open error to orginal file %s: %s", o_filename, strerror(errno));
        } else {
            LOGV("open error to orginal file %s: %s", o_filename, strerror(errno));
        }
        return -1;
    }

    ret = call_defrag(fd, donor_fd, (int)swap_blocks_info[ii].l_blk, swap_block_nums, swap_blocks_info[ii].len);
    if (ret < 0) {
        close(fd);
        LOGE("call_defrag error, ret %d", ret);
        return -2;
    }

    close(fd);

    if (data_verify) {
        ret = cksum_generate(o_filename, &cksum_after);
        if (ret < 0) {
            LOGE("cksum_generate after defrag error");
            return -4;
        } else {
            LOGV("after defrag checksum is %u", cksum_after);
        }

        if(cksum_before != cksum_after) {
            LOGE("%s checksum before and after defrag is inconsistent, cksum_before %u, cksum_after %u", o_filename, cksum_before, cksum_after);
            return -3;
        }
    }

    return 0;
}

static int swap_blocks(int donor_fd, unsigned long *num_moved_blocks)
{
    int ret = 0;
    unsigned long swap_block_nums = 0;
    int ii, first_index, last_index;

    ret = enhance_ncheck(swap_blocks_info, num_swap_extents);
    if (ret < 0) {
        LOGE("enhance_ncheck error, ret:%d", ret);
        return -1;
    }

    first_index = 0;
    last_index = num_swap_extents - 1;

    for (ii = first_index, swap_block_nums = 0; ii <= last_index; ii++) {
        ret = swap_extent(donor_fd, ii, swap_block_nums);
        if (ret < 0) {
            break;
        }

        swap_block_nums += swap_blocks_info[ii].len;
    }

    *num_moved_blocks += swap_block_nums;
    return ret;
}

static int get_group_info(char *block, unsigned int cur_group_no, char *frags_info)
{
    char *p;
    char device_name[64];
    int bypass_first_line = true;
    char group_info_name[PATH_MAX] = {0};
    int ret;

    strcpy(device_name, (p = strrchr(block, '/')) ? p + 1 : block);

    strcpy(group_info_name, "/proc/fs/ext4/");
    strcat(group_info_name, device_name);
    strcat(group_info_name, "/mb_groups");

    LOGV("group_info_name: %s\n", group_info_name);
    FILE *fp = fopen(group_info_name, "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            unsigned int group_no;

            if (bypass_first_line == true) {
                bypass_first_line = false;
                continue;
            }

            strcpy(frags_info, line);
            ret = sscanf(line, "#%5u", &group_no);

            if (ret > 0) {
                if (cur_group_no == group_no) {
                    fclose(fp);
                    return 0;
                }
            }
        }
         fclose(fp);
    }

    return -1;
}

static void show_block_group_status(char *device_name, unsigned int group)
{
    char frags_info[1024];

    get_group_info(device_name, group, frags_info);
    LOGD("#group: free  frags first [ 2^0   2^1   2^2   2^3   2^4   2^5   2^6   2^7   2^8   2^9   2^10  2^11  2^12  2^13  ]");
    LOGD("%s", frags_info);
}

static bool has_not_enough_free_space(ext2_filsys fs)
{
    blk64_t cur_free_blocks, total_blocks;

    cur_free_blocks = ext2fs_free_blocks_count(fs->super);
    total_blocks = ext2fs_blocks_count(fs->super);

    if ((100 * cur_free_blocks / total_blocks) <= MIN_FREE_SPACE_PERCENTAGE_THRESHOLD)
        return true;

    return false;
}

static unsigned long get_block_group_defrag_benefit(ext2_filsys fs, unsigned int free_blocks, unsigned int fragments)
{
    unsigned int percentage = (100 * free_blocks) / fs->super->s_blocks_per_group;
    return (unsigned long)fragments * percentage;
}

/*
 * Get block group free space defragment information and return candidate block group to defrag
 */
static int get_next_block_group_greedy(ext2_filsys fs, unsigned int start, unsigned int end)
{
    char *p;
    char device[64];
    int bypass_first_line = true;
    char group_info_name[PATH_MAX] = {0};
    int ret;
    unsigned int group = -1;
    unsigned long max_benefit = 0;
    unsigned long benefit;
    bool find = false;
    /* Record block groups have been scanned, maximum 2048 block groups = 256GB */
    static char scan_groups_bitmap[256] = {0};

    strcpy(device, (p = strrchr(fs->device_name, '/')) ? p + 1 : fs->device_name);
    strcpy(group_info_name, "/proc/fs/ext4/");
    strcat(group_info_name, device);
    strcat(group_info_name, "/mb_groups");

    //LOGV("get_next_block_group enter: group_info_name is %s", group_info_name);
    FILE *fp = fopen(group_info_name, "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            unsigned int cur_group, cur_free_blocks, cur_free_fragments;

            if (bypass_first_line == true) {
                bypass_first_line = false;
                continue;
            }

            ret = sscanf(line, "#%u%*[ ]: %u%*[ ]%u", &cur_group, &cur_free_blocks, &cur_free_fragments);
            if (ret < 3) {
                LOGE("sscanf error, ret is %d", ret);
                fclose(fp);
                return -1;
            }

            //LOGV("current group is %u, current free blocks is %u, current free fragments is %u",
            //            cur_group, cur_free_blocks, cur_free_fragments);
            if (cur_group < start)
                continue;

            if (cur_group > end)
                break;

            if (cur_free_blocks <= MIN_GROUP_FREE_BLOCKS_THRESHOLD
                    || cur_free_fragments <= MIN_GROUP_FRAGS_THRESHOLD
                    || (cur_free_blocks / cur_free_fragments) >= AVERAGE_FREE_BLOCKS_LENGTH)
                continue;

            /* If block group has been defragged, ignore it */
            if (scan_groups_bitmap[cur_group >> 3] & (1 << (cur_group % 8)))
                continue;

            benefit = get_block_group_defrag_benefit(fs, cur_free_blocks, cur_free_fragments);
            if (max_benefit < benefit) {
                //LOGV("group is %u, benefit is %lu, max_benefit is %lu", group, benefit, max_benefit);
                group = cur_group;
                max_benefit = benefit;
                find = true;
            }
        }
        fclose(fp);
    } else {
        LOGE("fopen error, group_info_name is %s, errno is %d", group_info_name, errno);
        return -1;
    }

    if (find) {
        /* Add block group to bitmap */
        scan_groups_bitmap[group >> 3] |= 1 << (group % 8);
    }

    //LOGV("get_next_block_group exit: group is %u, max_benefit is %lu", group, max_benefit);
    return find ? group : -1;
}

static int get_next_block_group_linear(unsigned int start, unsigned int end)
{
    char last_group[PROPERTY_VALUE_MAX] = {0};
    unsigned int last_group_no = 0;
    unsigned int group;

    /* Continue block group defrag process from where we stopped last time */
    property_get(COMPACT_LAST_GROUP, last_group, "0");
    last_group_no = atoi(last_group);
    if (last_group_no < start || last_group_no > end) {
        last_group_no = start;
    }

    group = last_group_no++;

    /* Remember block group next to defrag */
    sprintf(last_group, "%u", last_group_no);
    property_set(COMPACT_LAST_GROUP, last_group);

    return group;
}

/*
 * Get block group to defrag
 */
static int get_next_block_group(ext2_filsys fs, unsigned int start, unsigned int end, int policy)
{
    int ret;

    if (has_not_enough_free_space(fs)) {
        LOGI("low free space");
        return -1;
    }

    if (policy == LINEAR) {
        ret = get_next_block_group_linear(start, end);
    } else {
        ret = get_next_block_group_greedy(fs, start, end);
    }

    return ret;
}

static void show_icheck_ncheck_status()
{
    LOGD("======================================");
    LOGD("icheck statistic status:");
    LOGD("total icheck numbers: %llu", total_icheck_num);
    LOGD("group hit ratio: %llu%% (%llu / %llu)", icheck_hit_num * 100 / total_icheck_num, icheck_hit_num, total_icheck_num);
    LOGD("inode is on the left side of data block: %llu", icheck_left_num);
    LOGD("inode is on the right side of data block: %llu", icheck_right_num);
    LOGD("block groups cache hit ratio: %llu%% (%llu / %llu)", icheck_cache_hit_num * 100 / total_icheck_num, icheck_cache_hit_num, total_icheck_num);
    LOGD("======================================");
    LOGD("ncheck statistic status:");
    LOGD("total ncheck numbers: %llu", total_ncheck_num);
    LOGD("group hit ratio: %llu%% (%llu / %llu)", ncheck_hit_num * 100 / total_ncheck_num, ncheck_hit_num, total_ncheck_num);
    LOGD("dir inode is on the left side of file inode: %llu", ncheck_left_num);
    LOGD("dir inode is on the right side of file inode: %llu", ncheck_right_num);
    LOGD("block groups cache hit ratio: %llu%% (%llu / %llu)", ncheck_cache_hit_num * 100 / total_ncheck_num, ncheck_cache_hit_num, total_ncheck_num);
    LOGD("======================================");
}

/*
 * storage_compact() - Ext4 block group compact.
 */
int storage_compact(ext2_filsys current_fs, int fd, unsigned int first_group_no,
                        unsigned int end_group_no, unsigned int max_compact_groups, int policy)
{
    int ret = 0;
    unsigned int cur_group = 0;
    __u64 free_start, used_start, free_end, used_end;
    __u64 start_data_blk;
    unsigned long length;
    struct defrag_range range;
    struct fallocate_reserved_range reserve_range;
    unsigned long num_frag_compacted = 0, num_defraged_group = 0, num_moved_blocks = 0;
    int iteration = 0;

    log.traceBegin();
    LOGI("storage_compact enter:");

    pthread_mutex_lock(&stop_mutex);
    if (g_stop_flag == 1) {
        pthread_mutex_unlock(&stop_mutex);
        LOGI("user cancel storage compact");
        log.markExitReason("user_cancel");
        goto tracing_end;
    }
    pthread_mutex_unlock(&stop_mutex);

    swap_blocks_info = (struct swap_block_info *)malloc(sizeof(struct swap_block_info) * MAX_NUM_OF_BLOCK_INFO);
    if (!swap_blocks_info) {
        LOGE("malloc swap_blocks_info failed");
        ret = -1;
        log.markExitReason("alloc_swap_blocks_info_error");
        goto exit;
    }

    ret = get_next_block_group(current_fs, first_group_no, end_group_no, policy);
    if (ret < 0) {
        LOGD("no block group need to defrag");
        ret = 0;
        log.markExitReason("no_group_need_be_defrag_0");
        goto free_swap_block_info;
    }

    cur_group = ret;
    // Add current group into block group cache
    icheck_cache_add(cur_group);
    ncheck_cache_add(cur_group);

    // Show current block group extents info before compact operation
    LOGD("#### group %u before compact ####", cur_group);
    log.traceGroupBegin(cur_group);
    show_block_group_status(current_fs->device_name, cur_group);

    memset(&range, 0, sizeof(struct defrag_range));

compact_next_frag:
    pthread_mutex_lock(&stop_mutex);
    if (g_stop_flag == 1) {
        pthread_mutex_unlock(&stop_mutex);
        LOGI("user cancel storage compact");
        log.markExitReason("stop");
        goto free_swap_block_info;
    }
    pthread_mutex_unlock(&stop_mutex);

    memset(swap_blocks_info, 0, sizeof(struct swap_block_info) * MAX_NUM_OF_BLOCK_INFO);
    num_swap_extents = 0;
    iteration++;

    // Step1: get defrag range from kernel
    LOGV("[group#%u][iteration#%d][step#1]: get defrag range", cur_group, iteration);
    range.group = cur_group;
    ret = ioctl(fd, EXT4_IOC_DEFRAG_RANGE, &range);
    if (ret < 0) {
        // Show current block group extents info after compact operation
        LOGD("#### [group %u] after compact ####", cur_group);
        log.traceGroupEnd(cur_group, num_moved_blocks);
        show_block_group_status(current_fs->device_name, cur_group);
        LOGD("group %u: total %lu blocks moved", cur_group, num_moved_blocks);

        // If policy is greedy, count it only when block group has actually been defragged
        if (num_moved_blocks || policy != GREEDY)
            num_defraged_group++;

        if (num_defraged_group >= max_compact_groups) {
            ret = 0;
            log.markExitReason("normal_exit");
            goto free_swap_block_info;
        }

        ret = get_next_block_group(current_fs, first_group_no, end_group_no, policy);
        if (ret < 0) {
            LOGD("no block group need to defrag");
            ret = 0;
            log.markExitReason("no_group_need_be_defrag_1");
            goto free_swap_block_info;
        }

        cur_group = ret;
        /*
         * Reset icheck and ncheck block group cache, and add current block group into cache
         */
        reset_check_cache();
        icheck_cache_add(cur_group);
        ncheck_cache_add(cur_group);

        num_moved_blocks = 0;
        // Show next block group extents info before compact operation
        LOGD("#### [group %u] before compact ####", cur_group);
        log.traceGroupBegin(cur_group);
        show_block_group_status(current_fs->device_name, cur_group);

        memset(&range, 0, sizeof(struct defrag_range));
        goto compact_next_frag;
    }

    start_data_blk = current_fs->super->s_first_data_block + cur_group * (current_fs->super->s_blocks_per_group);
    free_start = range.free_start;
    used_start = range.used_start;
    free_end = range.free_end;
    used_end = range.used_end;
    length = range.length;

    // Step2: find all owner files according to defrag range returned by kernel
    LOGV("[group#%d][iteration#%d][step#2]: returned defrag range info: free_start:%llu, free_end:%llu, used_start:%llu, used_end:%llu, length: %lu",
                cur_group, iteration, start_data_blk + free_start, start_data_blk + free_end, start_data_blk + used_start, start_data_blk + used_end, length);

    ret = enhance_icheck(cur_group, used_start + start_data_blk, used_end + start_data_blk);
    if (ret < 0) {
        if (ret == -2) {
            memset(&range, 0, sizeof(struct defrag_range));
            goto compact_next_frag;
        } else {
            LOGE("enhance_icheck error, ret %d", ret);
            if (ret != -3)
                log.markExitReason("enhance_icheck_err(ret<0&&ret!=-2&&ret!=-3)");
            goto free_swap_block_info;
        }
    }
    num_swap_extents = ret;

    // Step3: generate the donor file to use the free subranges of defrag range
    LOGV("[group#%d][iteration#%d][step#3]: generate donor file", cur_group, iteration);
    reserve_range.mode = FALLOC_FL_RESERVE_RANGE;
    reserve_range.offset = 0;
    reserve_range.len = length * BLOCK_SIZE;
    reserve_range.pa_addr.pa_group = cur_group;
    reserve_range.pa_addr.pa_offset = free_start;

    ret = ioctl(donor_fd, EXT4_IOC_FALLOCATE_RESERVE, &reserve_range);
    if (ret < 0) {
        LOGE("fallocate donor file error, ret %d, errno %d", ret, errno);
        log.markExitReason("fallocate_donor_file_err");
        goto truncate_donor_file;
    }

    // Step4: do the move extents work
    LOGV("[group#%d][iteration#%d][step#4]: swap donor file and data file", cur_group, iteration);
    ret = swap_blocks(donor_fd, &num_moved_blocks);
    if (ret < 0) {
        if (errno != EACCES) {
            LOGE("swap_blocks error, ret %d, errno %d", ret, errno);
        } else {
            LOGV("swap_blocks error, ret %d, errno %d", ret, errno);
        }
    }

truncate_donor_file:
    // Step5: truncate donor file
    LOGV("[group#%d][iteration#%d][step#5]: finish extent swap operation, truncate donor file now", cur_group, iteration);
    if (ftruncate(donor_fd, 0) < 0) {
        LOGE("ftruncate donor file error");
        log.markExitReason("ftruncate_donor_file_err");
        goto free_swap_block_info;
    }

    num_frag_compacted++;
    goto compact_next_frag;

free_swap_block_info:
    free(swap_blocks_info);
exit:
    LOGI("storage_compact exit: num_frag_compacted = %lu, num_defraged_group = %lu, g_stop_flag = %d",
                num_frag_compacted, num_defraged_group, g_stop_flag);
    show_icheck_ncheck_status();

tracing_end:
    log.traceEnd(
            total_icheck_num, icheck_hit_num,
            icheck_left_num, icheck_right_num,
            icheck_cache_hit_num,
            total_ncheck_num, ncheck_hit_num,
            ncheck_left_num, ncheck_right_num,
            ncheck_cache_hit_num
    );

    log.save();

    return ret;
}
