#define LOG_TAG "FileDefrag"

#include <ext2fs/ext2_types.h>
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <log/log.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include "utils.h"
#include "filedefrag.h"
#include "../log.h"

/* The mode of defrag */
#define DETAIL             0x01
#define NUM_DEFRAG_DIRS    2
#define NUM_WEIXIN_USERS   10
#define USER_NAME_LEN      80

#define DEFRAG_DEFAULT     0
#define DEFRAG_FILE        1
#define DEFRAG_DIR         2

#define BLOCK_SIZE         4096
#define SYSBLOCK_NAME_MAX  20

FileDefragLogger log;
unsigned int max_sectors_kb = 0;
extern int data_verify;
extern int donor_fd;
int donor_dir_fd;

int only_defrag_extension;
static char defaultObjFiles[][PATH_MAX] = {
    "/data/data",
    "/data/user",
    "/data/user_de",
    "/data/media/0",
    "/data/vendor"
};
static char *optObjFiles[] = {0};

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

static int exec_e4defrag(int num, int defrag_obj) {
    int ii;

    // It will call markExitReason() in following code again if this
    // process exit abnormally.
    log.markExitReason("normal_exit");

    log.traceBegin();
    for (ii = 0; ii < num; ii++) {
        if (defrag_obj == DEFRAG_FILE)
            defrag_file(optObjFiles[ii]);
        else if (defrag_obj == DEFRAG_DIR)
            defrag_dir(optObjFiles[ii]);
        else if (defrag_obj == DEFRAG_DEFAULT)
            defrag_dir(defaultObjFiles[ii]);
    }

    log.traceEnd(extents_before_defrag, extents_after_defrag,
                 g_defrag_stat, g_donor_call_defrag_estat);
    return 0;
}

void thread_exit_handler(int sig) {
    LOGD("stop filedefrag and the signal is %d", sig);
    stop_defrag();
}

static void defragdata_init(ext2_filsys fs) {
    blocks_per_group = fs->super->s_blocks_per_group;
    feature_incompat = fs->super->s_feature_incompat;
    log_groups_per_flex = fs->super->s_log_groups_per_flex;

    defraged_file_count = 0;
}

static unsigned int get_block_transaction_limit(char *device_name) {
    unsigned int max_trans_sectors_kb = 0;
    unsigned int read_ahead_kb = 0;
    char sysfile_name[PATH_MAX];
    char sysblock_name[SYSBLOCK_NAME_MAX];
    char *block_device = NULL;
    int ret;

    block_device = strrchr(device_name, '/');
    if(block_device == NULL) {
        LOGE("block_device name error");
        return -1;
    }

    memset(sysblock_name, 0, SYSBLOCK_NAME_MAX);
    if (strncmp(block_device, "/dm", 3) == 0) {
        strcpy(sysblock_name, block_device);
    } else if (strncmp(block_device, "/sd", 3) == 0) {
        strncpy(sysblock_name, block_device, 4);
    } else if (strncmp(block_device, "/mmcblk0p", 9) == 0) {
        strncpy(sysblock_name, block_device, 8);
    }

    strcpy(sysfile_name, "/sys/block/");
    strcat(sysfile_name, sysblock_name);
    strcat(sysfile_name, "/queue/read_ahead_kb");

    ret = get_file_value(sysfile_name, &read_ahead_kb);
    if (ret) {
        LOGE("%s fail to get read_ahead value in %s", __func__, sysfile_name);
        read_ahead_kb = 0;
    }

    strcpy(sysfile_name, "/sys/block/");
    strcat(sysfile_name, sysblock_name);
    strcat(sysfile_name, "/queue/max_sectors_kb");

    ret = get_file_value(sysfile_name, &max_trans_sectors_kb);
    if (ret) {
        LOGE("%s fail to get max_sectors value in %s", __func__, sysfile_name);
        max_trans_sectors_kb = 0;
    }

    if(max_trans_sectors_kb > read_ahead_kb)
        max_trans_sectors_kb = read_ahead_kb;

    return max_trans_sectors_kb;
}

static void usage(const char *prog)
{
    LOGD("Usage: %s [options...] <path>", prog);
    LOGI("Options:");
    LOGI("  -a defrag all type file");
    LOGI("  -A log to Android logging system instead of stderr");
    LOGI("  -d debug level [0:info 1:debug 2:verbose default:0]");
    LOGI("  -f defrag file");
    LOGI("  -h show this help text");
    LOGI("  -r defrag dir");
    LOGI("  -o path to log root dir");
    LOGI("  -O override the generated log subdir name");
    LOGI("  -v enable checking file data consistent");
    exit(1);
}

int main(int argc, char *argv[]) {
    int c, ret, defrag_obj = DEFRAG_DEFAULT, num;
    char data_path[] = "/data";
    char device_name[PATH_MAX];
    char donor_file[PATH_MAX];
    ext2_filsys fs = NULL;
    struct sigaction actions;
    DIR *dir_ptr = NULL;

    only_defrag_extension = 1;

    recover_stopflag();

    memset(&actions, 0, sizeof(actions));
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = thread_exit_handler;
    sigaction(SIGUSR1, &actions, NULL);

    while ((c = getopt(argc, argv, "d:afrho:O:Av")) != EOF) {
        switch (c) {
        case 'd':
            log.setLogLevel(parseLogLevelArg(optarg));
            break;
        case 'a':
            only_defrag_extension = 0;
            break;
        case 'f':
            defrag_obj = DEFRAG_FILE;
            break;
        case 'r':
            defrag_obj = DEFRAG_DIR;
            break;
        case 'o':
            log.setLogRootDirPath(string(optarg));
            break;
        case 'O':
            log.setLogDirPath(string(optarg));
            break;
        case 'A':
            log.logToAndroidLoggingSystem(true);
            break;
        case 'v':
            data_verify = 1;
            break;
        case 'h':
        default:
            usage(argv[0]);
            break;
        }
    }

    if (defrag_obj != DEFRAG_DEFAULT) {
        if (optind >= argc) {
            LOGE("%s: option parse error.lack of file lists\n", argv[0]);
            usage(argv[0]);
        } else {
            for(num = 0; (num + optind) < argc; num++)
               optObjFiles[num] = argv[num + optind];
        }
    }

    if (get_block_device(data_path, device_name) < 0) {
        LOGE("get_block_device error");
        return -1;
    }

    max_sectors_kb = get_block_transaction_limit(device_name);
    LOGD("max_sectors_kb is %u", max_sectors_kb);

    /* Get super block info */
    ret = ext2fs_open(device_name, 0, 0, BLOCK_SIZE,
          unix_io_manager, &fs);
    if (ret) {
        LOGE("%s fail to ext2fs_open",__func__);
        return -1;
    }

    //for new kernel encryption mode（v2）.v2 encryption mode get fscrypt context need dir_fd
    dir_ptr = opendir("/data");
    if (!dir_ptr) {
        LOGE("donor_dir dir_ptr is null");
        ext2fs_close(fs);
        return -1;
    }
    donor_dir_fd = dirfd(dir_ptr);

    /* Create donor inode */
    memset(donor_file, 0, PATH_MAX);
    strcpy(donor_file, "/data/mqsas/.tmp_filedefrag");
    unlink(donor_file);

    donor_fd = open64(donor_file, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR);
    if (donor_fd < 0) {
        LOGE("create donor file error");
        closedir(dir_ptr);
        ext2fs_close(fs);
        return -1;
    }

    defragdata_init(fs);
    if (defrag_obj == DEFRAG_DEFAULT)
        exec_e4defrag(sizeof(defaultObjFiles) / sizeof(defaultObjFiles[0]), defrag_obj);
    else
        exec_e4defrag(num, defrag_obj);

    log.save();
    closedir(dir_ptr);
    close(donor_fd);
    unlink(donor_file);
    ext2fs_close(fs);
    recover_stopflag();

    LOGD("filedefrag have been finished");
    return 0;
}


