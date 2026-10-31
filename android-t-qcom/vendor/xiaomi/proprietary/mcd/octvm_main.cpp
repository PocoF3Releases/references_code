#define LOG_TAG "octvm"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <errno.h>

#include <cutils/properties.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "octvm.h"
#include "drv/memsw_state.h"
#include "drv/memctrl_drv.h"
#include "octvm/OctVmService.h"
#include "octvm/SystemKloProxy.h"
#include "octvm/PowerStateService.h"

extern "C" {
#include "mc_sec.h"
#include "mc_plugintools.h"
#include "klo_internal.h"
#include "debug_utils.h"
}

using namespace android;

#define OCTVM_START_COMPLETE 1990300

static bool platform_init_stage1(void)
{
    //check cgroup support: give a chance to try mount again
    if(!cgroup_supported())
        //cgroup_try_mount_cgroup();
    if(!cgroup_supported()) {
        ERROR("cgroup not support\n");
    }
    if (!cgroup_freezer_supported()) {
        cgroup_try_mount_cgroup_freezer();
    }
    if (cgroup_freezer_supported()) {
        cgroup_basic_freezer_config();
        //freezeCGPoolinit();
    }
    //check if zram support
    if (supported_zram_devices() <= 0) {
        ERROR("not find zram device\n");
        return false;
    }
    //check memsw_state device
    if (memsw_state_device_init() != 0) {
        ERROR("memsw_state device not found or permission denied\n");
    }
    //all init and check pass return true
    return true;
}

static bool platform_init_stage2(struct sysinfo *si)
{
    int32_t zram_dev_num = supported_zram_devices();
    if (zram_dev_num >= gVMMemoryParams->zram_device_num) {
        zram_dev_num = gVMMemoryParams->zram_device_num;
    }
    int total_size_MB = gVMMemoryParams->zram_total_size;
    if (gVMMemoryParams->zram_device_num <= 0 || total_size_MB <= 0) {
        return false;
    }
    //try config zram disksize, not exceed half total memory
    if (total_size_MB >= si->totalram/1024) {
        total_size_MB = (si->totalram/1024)/2;
    }
    if (zram_try_config_devices(total_size_MB, zram_dev_num) < 0) {
        return false;
    }
    //extra sysctl
    zram_swap_extra_sysctl(gVMMemoryParams->global_swappiness);
    //swapon the zram device
    char device_name[32];
    for(uint32_t dev_no = 0; dev_no < zram_dev_num; dev_no++) {
        memset(device_name, 0, sizeof(device_name));
        snprintf(device_name, sizeof(device_name)-1, ZRAM_DEVICE_PREFIX"%d", dev_no);
        if (zram_swap_activate(device_name) != 0)
            return false;
    }

    return true;
}

#define DENSITY_LOW        120
#define DENSITY_MEDIUM     160
#define DENSITY_HIGH       240
#define DENSITY_XHIGH      320
#define DENSITY_XXHIGH     480
#define BUILD_DENSITY_PROP "ro.sf.lcd_density"

//unit in KB
#define MEMORY_SIZE_1GB   (1*1024*1024)
#define MEMORY_SIZE_1_5GB (1.5*1024*1024)
#define MEMORY_SIZE_2GB   (2*1024*1024)
#define MEMORY_SIZE_3GB   (3*1024*1024)

static bool octvm_memctrl_feature_auto(struct sysinfo *si)
{
    int density = 0;
    char property[PROPERTY_VALUE_MAX];
    if (property_get(BUILD_DENSITY_PROP, property, NULL) > 0) {
        density = atoi(property);
    }

    //auto on case1: 1G memory device
    if (si->totalram/1024 <= MEMORY_SIZE_1GB)
        return true;
    //auto on case2: 1GB~1.5GB memory device with xhdpi density
    if ((si->totalram/1024 <= MEMORY_SIZE_1_5GB) && (density >= DENSITY_XHIGH))
        return true;
    //auto on case3: 2G memory with xxhdpi density
    if ((si->totalram/1024 <= MEMORY_SIZE_2GB) && (density >= DENSITY_XXHIGH))
        return true;
    //auto on case4: 64bit system
    if (strcmp(TARGET_ARCH,"arm64") == 0 || strcmp(MTK_K64_SUPPORT, "yes") == 0)
        return true;

    //default
    return false;
}

/*
 * main for octvm basic init, running as root, start from init rc script
 */
static void octvm_init_basic(int argc, char **argv)
{
    struct sysinfo si;
    sysinfo(&si);

    //init and check low level support status
    if (!platform_init_stage1()) {
        OctVmService::setFeatureStatus("not support");
        return;
    }
    OctVmService::setFeatureStatus("supported");

    //swap on autoconfig: low ram device
    bool swap_on = octvm_memctrl_feature_auto(&si);
    //if product: ido/land/ysl/cereus/cactus, swap on
    if (strcmp(TARGET_PRODUCT, "ido") == 0
        || strcmp(TARGET_PRODUCT, "land") == 0
        || strcmp(TARGET_PRODUCT, "cactus") == 0
        || strcmp(TARGET_PRODUCT, "cereus") == 0
        ||strcmp(TARGET_PRODUCT, "ysl") == 0) {
        swap_on = true;
    }
    INFO("config swap %s\n", swap_on ? "on" : "off");

    if (swap_on == false){
        OctVmService::setFeatureStatus("off");
    }

    if (swap_on && gVMMemoryParams->zram_total_size > 0) {
        if (0 == strcmp("MI PAD", gModel)) {
            sleep(20);
        }
        if (si.totalswap > 0) {
            ERROR("seems swap already on, you may need change the init.rc file\n");
        }
        else if (!platform_init_stage2(&si)) {
            //platform supported, but error occur when init zram swap
            ERROR("error happen when config zram size or swapon zram\n");
            OctVmService::setFeatureStatus("error");
            return;
        }
        //extra permission change on cgroup
        cgroup_basic_config();
        //zram swap switch on success
        OctVmService::setFeatureStatus("on");
        //create and init default cgroups
        OctVmService::createAllSystemCGroups();
    }

    return;
}

extern bool global_config_init();
extern bool command_parse_and_execute(int argc, char** argv);

int main(int argc, char **argv)
{
#if PLATFORM_SDK_VERSION >= 24
    /* defined in system/core/include/private/android_filesystem_config.h */
    gid_t groups[] = { AID_INET, AID_NET_RAW, AID_NET_ADMIN, AID_READPROC, AID_WAKELOCK};
    if (setgroups(sizeof(groups) / sizeof(groups[0]), groups) == -1) {
        ALOGE("setgroups to readproc failed, %s.\n", strerror(errno));
        //return -1;
    }
#endif
    // record boot complete info
    int self_ppid = getppid();
    if(argc < 2 && self_ppid == 1) {
        record_boot_complete_info();
    }

    if(global_config_init() == false) {
        ERROR("config init error, mcd will exit.\n");
        return -1;
    }

    int arg_idx = argc;
    char *arg_ptrs[MAX_MCD_PARAMS_N+1];
    memcpy(arg_ptrs, argv, (argc * sizeof(char *)));  //copy args

    char extra_params[PROPERTY_VALUE_MAX];
    memset(extra_params, 0, PROPERTY_VALUE_MAX);
    property_get(MCD_EXTRA_PARAMS, extra_params, "");
    if (strlen(extra_params) != 0) {
        char *tmp = strdup(extra_params);
        char *next = tmp, *bword;
        while((bword = strsep(&next, " "))) {
            arg_ptrs[arg_idx++] = bword;
            if (arg_idx == MAX_MCD_PARAMS_N)
                break;
        }
        arg_ptrs[arg_idx] = NULL;
    }

    int num = arg_idx;
    String8 user_cmd;
    while(--num) user_cmd.appendFormat("%s ", arg_ptrs[arg_idx-num]);
    if (checkDebug()) {
        INFO("[%d][arg_idx %d]: %s %s\n", getpid(), arg_idx, arg_ptrs[0], user_cmd.string());
        ALOGI("[%d][arg_idx %d]: %s %s", getpid(), arg_idx, arg_ptrs[0], user_cmd.string());
    }

    // init.miui.rc:
    // service mcd_init /system/bin/mcd init
    if ((arg_idx >= 2 && strcmp(arg_ptrs[1], "init") == 0) || self_ppid != 1) {
        // basic init for the first time
        if(self_ppid == 1 && (arg_idx == 3) && atoi(arg_ptrs[2]) > 0) {
            octvm_init_basic(argc, argv);
        } else {
            int rc = command_parse_and_execute(arg_idx, arg_ptrs);
            if (checkDebug()) {
                ALOGI("mcd command execute completed, exit with %d", rc);
            }
        }
        //reset params property
        property_set(MCD_EXTRA_PARAMS, "");
        return 0;
    }

    //ensure shell command not run through here
    if (self_ppid != 1) {
        ALOGD("invalid calling to mcd");
        return 0;
    }

    //add task to cgroup as config
    if(OctVmService::isFeatureOn()) {
        OctVmService::initAllCGroupTasks();
    }

    //ignore sigpipe signal
    signal(SIGPIPE, SIG_IGN);

    //switch to system user
    //mc_print_my_cap();
    mc_switch_system_user();
    //mc_print_my_cap();

    //start plugin when start
    mc_plugintool_load_all();

    //octvm main services start
    sp<IServiceManager> sm = defaultServiceManager();
    ALOGI("ServiceManager: %p", sm.get());

    //check and start services
    if(!OctVmService::isServiceStarted()
            || !SystemKloProxy::isServiceStarted()
            || !PowerStateService::isServiceStarted()) {
        //instantiate service
        OctVmService::instantiate();
        SystemKloProxy::instantiate();
        PowerStateService::instantiate();

        //log the mcd_service start complete event
        LOG_EVENT_INT(OCTVM_START_COMPLETE, getpid());

        ProcessState::self()->startThreadPool();
        IPCThreadState::self()->joinThreadPool();
    }
    return 0;
}
