/*
 * Copyright (C) Xiaomi Inc.
 *
 */
#define LOG_TAG "FragCompact"

#include <dirent.h>
#include <ext2fs/ext2_types.h>
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <cutils/properties.h>

#include "e2fsprogs_modified/cache.h"
#include "storage_compact.h"
#include "../FragInfo.h"
#include "../utils.h"
#include "../log.h"

#define BLOCK_SIZE                     4096

CompactLogger log;

extern int dry_run;
extern int data_verify;
extern char *data_path;
extern int donor_fd;
int donor_dir_fd;

ext2_filsys current_fs = NULL;

static void stop_defrag() {
        pthread_mutex_lock(&stop_mutex);
        g_stop_flag = 1;
        pthread_mutex_unlock(&stop_mutex);
}

static void recover_stopflag() {
        pthread_mutex_lock(&stop_mutex);
        g_stop_flag = 0;
        pthread_mutex_unlock(&stop_mutex);
}

void thread_exit_handler(int sig) {
    LOGI("stop defrag and the signal is %d", sig);
    stop_defrag();
}

static void show_storage_compact_info()
{
    LOGI("COMPACT: Ver: %s (%s)", COMPACT_VERSION, COMPACT_DATE);
}

static void usage()
{
    LOGI("Usage: msc [options...] <partition>");
    LOGI("Options:");
    LOGI("  -A log to Android logging system instead of stderr");
    LOGI("  -d debug level [0:info 1:debug 2:verbose default:0]");
    LOGI("  -e end block group [default:last block group]");
    LOGI("  -h show this help text");
    LOGI("  -m number of block groups compacted each time");
    LOGI("  -o path to log root dir");
    LOGI("  -O override the generated log subdir name");
    LOGI("  -p block group select policy [0:greedy 1:linear default:0]");
    LOGI("  -s start block group [default:0]");
    LOGI("  -v enable checking file data consistent");
    LOGI("  -V show storage compact version");
    LOGI("  --dry-run do not really swap extents");
    exit(1);
}

int main(int argc, char *argv[]) {
    unsigned int first_group_no = 0, end_group_no = UINT_MAX, max_compact_groups = UINT_MAX, group_nr = 0;
    char donor_file[PATH_MAX];
    char device_name[PATH_MAX] = {0};
    int policy = GREEDY;
    int fd;
    int ret = 0;
    int option;
    int opt = 0;
    struct option long_opt[] = {
        {"dry-run", no_argument, 0, 1},
        {"frag-info", no_argument, 0, 2},
        {0, 0, 0, 0}
    };
    int open_flags = EXT2_FLAG_SOFTSUPP_FEATURES | EXT2_FLAG_64BITS;
    struct sigaction actions;
    DIR *dir_ptr = NULL;

    // Register SIGUSR1 handler
    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR1, &actions, NULL);

    while ((option = getopt_long(argc, argv, "Ad:e:hm:o:O:p:s:vV",
                    long_opt, &opt)) != EOF) {
        switch (option) {
        case 1:
            dry_run = 1;
            break;
        case 2:
            printFragInfo();
            return 0;
            break;
        case 'A':
            log.logToAndroidLoggingSystem(true);
            break;
        case 'd':
            log.setLogLevel(parseLogLevelArg(optarg));
            break;
        case 'e':
            end_group_no = atoi(optarg);
            break;
        case 'h':
            usage();
            break;
        case 'm':
            // Override max compact groups
            max_compact_groups = atoi(optarg);
            break;
        case 'o':
            log.setLogRootDirPath(string(optarg));
            break;
        case 'O':
            log.setLogDirPath(string(optarg));
            break;
        case 'p':
            // Override policy
            policy = atoi(optarg);
            break;
        case 's':
            first_group_no = atoi(optarg);
            break;
        case 'v':
            data_verify = 1;
            break;
        case 'V':
            show_storage_compact_info();
            exit(0);
        default:
            LOGE("Error: Unknown option");
            usage();
            break;
        }
    }

    if (optind >= argc) {
        LOGE("%s: option parse error.\n", argv[0]);
        usage();
    }

    show_storage_compact_info();

    recover_stopflag();

    data_path = argv[optind];

    if (get_block_device(data_path, device_name) < 0) {
        LOGE("get_block_device error");
        return -1;
    }

    ret = ext2fs_open(device_name, open_flags, 0, BLOCK_SIZE, unix_io_manager, &current_fs);
    if (ret) {
        LOGE("ext2fs_open error, ret = %d", ret);
        return -1;
    }

    if (end_group_no >= current_fs->group_desc_count)
        end_group_no = current_fs->group_desc_count - 1;

    group_nr = end_group_no - first_group_no + 1;
    if (max_compact_groups > group_nr)
        max_compact_groups = group_nr;

    //for new kernel encryption mode（v2）.v2 encryption mode get fscrypt context need dir_fd
    dir_ptr = opendir("/data");
    if (!dir_ptr) {
        LOGE("donor_dir dir_ptr is null");
        ext2fs_close(current_fs);
        return -1;
    }
    donor_dir_fd = dirfd(dir_ptr);

    strcpy(donor_file, data_path);
    strcat(donor_file, "/mqsas/.tmp_defrag");

    unlink(donor_file);
    donor_fd = open64(donor_file, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR);
    if (donor_fd < 0) {
        LOGE("create donor file error");
        closedir(dir_ptr);
        ext2fs_close(current_fs);
        return -1;
    }

    fd = open(data_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        LOGE("failed to open(%s)", data_path);
        closedir(dir_ptr);
        close(donor_fd);
        ext2fs_close(current_fs);
        return -1;
    }

    // Create block group cache
    create_check_cache();

    LOGI("compact path: %s, first group: %u, end group: %u, max compact groups: %u, policy: %s, dry run: %d, data verify: %d",
                data_path, first_group_no, end_group_no, max_compact_groups, (policy == 0) ? "greedy" : "linear", dry_run, data_verify);
    ret = storage_compact(current_fs, fd, first_group_no, end_group_no, max_compact_groups, policy);
    if (ret < 0) {
        LOGE("storage_compact on %s failed, ret is %d", data_path, ret);
    }

    release_check_cache();
    closedir(dir_ptr);
    close(fd);
    close(donor_fd);
    unlink(donor_file);
    ext2fs_close(current_fs);
    recover_stopflag();
    return 0;
}


