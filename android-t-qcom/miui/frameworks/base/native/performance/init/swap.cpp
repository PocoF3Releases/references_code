#include <errno.h>
#include <sys/ioctl.h>
#include <sys/statfs.h>
#include <android-base/file.h>
#include <string>

using std::string;

#include "init.h"

#define MKSWAP_BIN       "/system/bin/mkswap"

#define ZRAM_BLOCK_DEV   "/dev/block/zram0"
#define ZRAM_CONF_DEV    "/sys/block/zram0/disksize"
#define ZRAM_CONF_MCS    "/sys/block/zram0/max_comp_streams"
#define ZRAM_CONF_CALGO  "/sys/block/zram0/comp_algorithm"
#define ZRAM_BACK_DEV    "/sys/block/zram0/backing_dev"
#define ZRAM_INITSTATE   "/sys/block/zram0/initstate"
#define ZRAM_RESET       "/sys/block/zram0/reset"

#define PROC_VM_SWAPPINESS     "/proc/sys/vm/swappiness"
#define PROC_VM_PAGECLUSTER    "/proc/sys/vm/page-cluster"

/*
 * Minimum free space percentage required to setup zram backing device
 */
#define MIN_FREE_SPACE_PERCENTAGE_THRESHOLD         10
#define WAIT_DEVICE_TIMEOUT_MS                      500
#define DATA_PATH                                   "/data"

#ifndef F2FS_IOC_SET_PIN_FILE
#ifndef F2FS_IOCTL_MAGIC
#define F2FS_IOCTL_MAGIC 0xf5
#endif
#define F2FS_IOC_SET_PIN_FILE _IOW(F2FS_IOCTL_MAGIC, 13, __u32)
#define F2FS_IOC_GET_PIN_FILE _IOR(F2FS_IOCTL_MAGIC, 14, __u32)
#endif

#define LOOPDEV_SCHEDULER_IO    "bfq"

using android::base::WriteStringToFile;
using android::base::ReadFileToString;

static bool first_time_to_swapoff = true;

static struct zram_config* get_zram_config()
{
    struct zram_config *config = get_loaded_zram_config();

    ALOGD("**********************************************");
    ALOGD("          swap_on: %d", config->swap_on);
    ALOGD("         disksize: %d", config->disksize);
    ALOGD("   comp_algorithm: %s", config->comp_algorithm);
    ALOGD("       swappiness: %d", config->swappiness);
    ALOGD(" max_comp_streams: %d", config->max_comp_streams);
    ALOGD("     page_cluster: %d", config->page_cluster);

    ALOGD("          extm_on: %d", config->extm_on);
    ALOGD("        extm_size: %d", config->extm_size);
    ALOGD("        extm_file: %s", config->extm_file);
    ALOGD("         extm_dev: %s", config->extm_dev);
    ALOGD("**********************************************");

    return config;
}

int KillExistedZram() {
    if (!first_time_to_swapoff) {
        ALOGW("no need to swapoff again.");
        return -1;
    }

    std::string zram_initstate;
    if (!android::base::ReadFileToString(ZRAM_INITSTATE, &zram_initstate)) {
        ALOGE("Failed to read /sys/block/zram0/initstate");
        return -1;
    }
    zram_initstate.erase(zram_initstate.length() - 1);
    ALOGE("read /sys/block/zram0/initstate [%s]", zram_initstate.c_str());

    if (zram_initstate == "0") {
        ALOGI("Zram has not been swapped on");
        return 0;
    }

    // Waitting 50s for swapoff successful. This waitting is not an error.
    int operate_count = 0;
    while(swapoff(ZRAM_BLOCK_DEV) == -1) {
        if (operate_count > 100) {
            ALOGW("wait 50s to swapoff zram0 and failed.");
            return -1;
        }
        operate_count++;
        std::this_thread::sleep_for(500ms);
    }
    ALOGI("wait %d ms to swapoff zram0 and succeed.", 500 * operate_count);

    if (!WriteStringToFile("1", ZRAM_RESET)) {
        ALOGE("zram_backing_dev: reset  failed");
        return -1;
    }

    first_time_to_swapoff = false;
    return 0;
}

/*
static int write_loopdev_scheduler(int loopNum)
{
    int ret = 0;
    char schedulerPath[1024] = {0};

    if(snprintf(schedulerPath, 1024, "/sys/block/loop%d/queue/scheduler", loopNum) < 0) {
        ALOGE("Constuct str loop%d's schedulerPath failed.", loopNum);
        ret = -1;
        return ret;
    }
    ret = write_fmt_data_to_file(schedulerPath, "%s\n", LOOPDEV_SCHEDULER_IO);
    return ret;
}
*/

#if PLATFORM_SDK_VERSION > 28 /* support from Android Q */
static bool backing_dev_has_enough_free_space(off64_t bdsize)
{
    struct statfs stat;
    int ret;
    long long size_pages = bdsize / 4096;
    long long size;

    ret = statfs(DATA_PATH, &stat);
    if (ret) {
        ALOGE("Statfs failed, errno = %d.", errno);
        return false;
    }

    size = stat.f_bavail - size_pages;
    ALOGE("stat.f_bavail - size_pages = %lld.", size);

    if (size < 0 || (100 * size / stat.f_blocks) < MIN_FREE_SPACE_PERCENTAGE_THRESHOLD) {
        ALOGE("Filesystem has not enough free space.");
        return false;
    }

    return true;
}

static int prepare_zram_backing_dev(char* loop, off64_t size /* byte */, char* bdev) {
    int ret = 0;
    int device_fd = 0;
    char device[1024] = {0};
    int target_fd = 0;
    int num = 0;
    int loop_ctl_fd = 0;
    __u32 val = 1;

    if (get_int_prop_config_item("persist.miui.extm.enable", 0) == 0) {
        ALOGE("The extm needs not to set.");
        ret = -1;
        goto end;
    }

    if (access(ZRAM_BACK_DEV, F_OK)) {
        ALOGE("This device don't support extm.");
        ret = -1;
        goto end;
    }

    if (loop == NULL && bdev == NULL) {
        ALOGE("Loop file and backing dev can't be null at sametime.");
        ret = -1;
        goto end;
    }

    // Use block dev as backup if exist
    if (bdev != NULL && strlen(bdev) > 0) {
        ret = write_fmt_data_to_file(ZRAM_BACK_DEV, "%s\n", bdev);
        goto end;
    }

    // Get free loopback
    loop_ctl_fd = TEMP_FAILURE_RETRY(open("/dev/loop-control", O_RDWR | O_CLOEXEC));
    if (loop_ctl_fd == -1) {
        ALOGE("Cannot open loop-control");
        ret = -1;
        goto end;
    }

    num = ioctl(loop_ctl_fd, LOOP_CTL_GET_FREE);
    if (num == -1) {
        ALOGE("Cannot get free loop slot");
        ret = -1;
        goto close_loop_ctl_fd;
    }

    // Prepare target path
    target_fd = TEMP_FAILURE_RETRY(open(loop, O_RDWR | O_CREAT | O_CLOEXEC, 0600));
    if (target_fd == -1) {
        ALOGE("Cannot open target path: %s", loop);
        ret = -1;
        goto close_loop_ctl_fd;
    }
    // If user needs to open, the space will not be checked here.
    if(!backing_dev_has_enough_free_space(size)) {
        ALOGE("Backing device has not enough free space");
        set_int_prop_config_item_forcing("persist.miui.extm.enable", 0);
        set_int_prop_config_item_forcing("miui.extm.low_data", 1);
        ret = -1;
        goto close_target_fd;
    }

    ret = ioctl(target_fd, F2FS_IOC_SET_PIN_FILE, &val);
    if (ret != 0) {
        ALOGW("Cannot pin target path: %s", loop);
    }

    if (fallocate(target_fd, 0, 0, size) < 0) {
        ALOGE("Cannot truncate target path: %s", loop);
        ret = -1;
        goto close_target_fd;
    }

    // Connect loopback (device_fd) to target path (target_fd)
    if(snprintf(device, 1024, "/dev/block/loop%d", num) < 0) {
        ALOGE("Constuct str /dev/block/loop%d failed.", num);
        ret = -1;
        goto close_target_fd;
    }

    // wait for /dev/block/loopxx creating successfully within timeout WAIT_DEVICE_TIMEOUT_MS
    if (wait_for_file(device, std::chrono::milliseconds(WAIT_DEVICE_TIMEOUT_MS))) {
        ALOGW("Wait for %s time out and took %dms.", device, WAIT_DEVICE_TIMEOUT_MS);
        ret = -1;
        goto close_target_fd;
    }
    device_fd = TEMP_FAILURE_RETRY(open(device, O_RDWR | O_CLOEXEC));
    if (device_fd == -1) {
        ALOGE("Cannot open /dev/block/loop%d", num);
        ret = -1;
        goto close_target_fd;
    }

    if (ioctl(device_fd, LOOP_SET_FD, target_fd)) {
        ALOGE("Cannot set loopback to target path");
        ret = -1;
        goto close_device_fd;
    }

    // set block size & direct IO
    if (ioctl(device_fd, LOOP_SET_BLOCK_SIZE, 4096)) {
        ALOGD("Cannot set 4KB blocksize to /dev/block/loop%d", num);
    }
    if (ioctl(device_fd, LOOP_SET_DIRECT_IO, 1)) {
        ALOGD("Cannot set direct_io to /dev/block/loop%d", num);
    }

    // Specfic: if device need to open extm, swapoff should be done before config backing_dev.
    KillExistedZram();

    ret = write_fmt_data_to_file(ZRAM_BACK_DEV, "%s\n", device);
    // ret = write_loopdev_scheduler(num);

close_device_fd:
    close(device_fd);
close_target_fd:
    close(target_fd);
close_loop_ctl_fd:
    close(loop_ctl_fd);
end:
    return ret;
}
#endif

int config_zram(struct zram_config* conf) {
    KillExistedZram();

    if (conf->disksize > 0) {
        if (conf->comp_algorithm != NULL) {
            write_fmt_data_to_file(ZRAM_CONF_CALGO, "%s\n", conf->comp_algorithm);
        }
        if (conf->max_comp_streams > 0) {
            write_fmt_data_to_file(ZRAM_CONF_MCS, "%d\n", conf->max_comp_streams);
        }
        if (conf->swappiness > 0) {
            write_fmt_data_to_file(PROC_VM_SWAPPINESS, "%d\n", conf->swappiness);
        }
        if (conf->page_cluster >= 0) {
            write_fmt_data_to_file(PROC_VM_PAGECLUSTER, "%d\n", conf->page_cluster);
        }
        long long size_bytes = conf->disksize;
        size_bytes = size_bytes * 1024 * 1024;
        write_fmt_data_to_file(ZRAM_CONF_DEV, "%lu\n", size_bytes);
    }

    // Initialize the swap area.
    int err = 0;
    int status;
    const char* mkswap_argv[2] = {
            MKSWAP_BIN,
            ZRAM_BLOCK_DEV,
    };

    ALOGE("try to mkswap %s", ZRAM_BLOCK_DEV);
    err = logwrap_fork_execvp(ARRAY_SIZE(mkswap_argv), mkswap_argv,
                                  &status, true, LOG_ALOG, false, NULL);
    if (err) {
        ALOGE("mkswap failed for %s", ZRAM_BLOCK_DEV);
        return -1;
    }

    ALOGE("mkswap success, try to swapon %s", ZRAM_BLOCK_DEV);
    err = swapon(ZRAM_BLOCK_DEV, 0);
    if (err == -1) {
        ALOGE("swapon failed for %s, err: %s", ZRAM_BLOCK_DEV, strerror(errno));
        return -1;
    }

    return 0;
}

int zram_init()
{
    if (!loaded_config_available()) {
        ALOGE("loaded config not available");
        return -1;
    }
    struct zram_config *config = get_zram_config();

#if PLATFORM_SDK_VERSION > SDK_VERSION_28 /* support from Android Q */
    long long size_bytes = get_long_long_prop_config_item("persist.miui.extm.bdsize", -1);
    if (size_bytes <= 0) {
        size_bytes = config->extm_size;
        set_int_prop_config_item_forcing("persist.miui.extm.bdsize",(int)size_bytes);
    }
    size_bytes = size_bytes * 1024 * 1024;

    if (config->extm_on > 0 && size_bytes > 0 && prepare_zram_backing_dev(
            config->extm_file, size_bytes, NULL)) {
        ALOGE("Skipping losetup for extendable memory.");
    }
#endif
    // config zram disksize & comp_algo
    if (config->swap_on > 0 && config->disksize > 0 && config_zram(config)) {
        ALOGE("Zram configuration failed.");
    }

    return 0;
}

