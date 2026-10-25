#define LOG_TAG "octvm"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fcntl.h>

#include <utils/String8.h>
#include <utils/String16.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <private/android_filesystem_config.h>

#include "octvm.h"
extern "C" {
#include "mc_sec.h"
#include "memcontrol.h"
}
#include "network.h"
#include "debug_utils.h"
#include "anonymous_id.h"
#include "drv/memctrl_drv.h"
#include "drv/platform_power.h"

#include "octvm/ISystemKloService.h"

#define MAX_ARGUMENT_LENGTH  128
#define MAX_LINE 1024
#define MAX_SUPPORT_ARGUMENTS 10

using namespace android;

typedef void (*command_func)(int argc, char **argv);

struct octvm_command {
    int argc;
    char* argv[MAX_SUPPORT_ARGUMENTS];
    command_func command;
};

static void octvm_powermode_config(int argc, char **argv);
static void octvm_memory_config(int argc, char **argv);
static void octvm_perf_config(int argc, char **argv);
static void octvm_test_client(int argc, char ** argv);
static void octvm_mount_ramdump(int argc, char ** argv);
static void octvm_debug_policy(int argc, char ** argv);
static void octvm_sudebug(int argc, char ** argv);
static void octvm_sudebug_camera(int argc, char ** argv);
static void octvm_ssl_client(int argc, char ** argv);
static void octvm_ssl_client2(int argc, char ** argv);
static void change_node(FILE* fd, const char* filename);

static octvm_command commands[] = {
    /* execute by init, commands should execute complete immediately */
    {3, {"init", "powermode", "*"/*<mode>*/}, octvm_powermode_config},
    {4, {"init", "powermode", "custom_profile", "*"/*<profile>*/}, octvm_powermode_config},
    {4, {"init", "perfconfig", "config_file", "*"/*<path>*/}, octvm_perf_config},
    {5, {"init", "perfconfig", "cpuset_by_pid", "*"/*cgroug_name*/,"*"/*<pid>*/}, octvm_perf_config},
    {4, {"init", "memory", "swappiness", "*"/*<swappiness>*/}, octvm_memory_config},
    {3, {"init", "ramdump", "mount"}, octvm_mount_ramdump},
    {3, {"init", "ramdump", "rmflag"}, octvm_mount_ramdump},
    {5, {"init", "debug_policy", "flash", "*"/*<src_path>*/, "*"/*<dst_path>*/}, octvm_debug_policy},
    {4, {"init", "debug_policy", "clean", "*"/*<file_path>*/}, octvm_debug_policy},
    {4, {"init", "sudebug", "kill_process_by_pid", "*"/*<process_pid>*/}, octvm_sudebug},
    {4, {"init", "sudebug", "kill_process_by_name", "*"/*<process_cmd>*/}, octvm_sudebug},
    {4, {"init", "sudebug", "get_iptables", "*"/*<file_path>*/}, octvm_sudebug},
    {4, {"init", "sudebug", "trace_on", "*"/*<0 or 1>*/}, octvm_sudebug},
    {4, {"init", "sudebug", "trigger_subsys", "*"/*<0 or 1>*/}, octvm_sudebug},
    /* NOT use sudebug sched command in new version */
    {4, {"init", "sudebug", "sched", "*"/*<path>*/}, octvm_sudebug},
    {4, {"init", "sudebug", "atrace", "*"/*0 or 1*/}, octvm_sudebug},
    {5, {"init", "sudebug", "top", "*"/*<path>*/, "*"/*<param_type>*/}, octvm_sudebug},
    {4, {"init", "sudebug", "dmesg", "*"/*<path>*/}, octvm_sudebug},
    {5, {"init", "sudebug", "set_chain_state", "*"/*<chain_name>*/, "*"/*<enable/disable>*/}, octvm_sudebug},
    {6, {"init", "sudebug", "init_gms_iptable_chain", "*"/*<chain_name>*/, "*"/*<uid>*/, "*"/*<rule>*/}, octvm_sudebug},
    {4, {"init", "sudebug_camera", "kill_camera_process_by_pid", "*"/*<process_pid>*/}, octvm_sudebug_camera},
    /* execute by shell or other, no execute time limit */
    {3, {"test", "cust_console", "*"/*<test_lib_path>*/}, octvm_test_client},
    {2, {"test", "wake_lock"}, octvm_test_client},
    {2, {"test", "wake_unlock"}, octvm_test_client},
    {2, {"test", "get_maxbrightness"}, octvm_test_client},
    {3, {"test", "set_brightness", "*"/*<brightness>*/}, octvm_test_client},
    {2, {"ssl_client", "update_mid"}, octvm_ssl_client},
    {2, {"ssl_client", "get_mid"}, octvm_ssl_client},
    {2, {"ssl_client2", "get_mid"}, octvm_ssl_client2}
};

char *not_compare_list[] = {
    "/sys/class/thermal/thermal_message/boost"
};

int not_compare_list_size = sizeof(not_compare_list)/sizeof(not_compare_list[0]);

bool command_parse_and_execute(int argc, char** argv)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("Permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    int i;
    int commands_num = sizeof(commands)/sizeof(octvm_command);
    for(i = 0; i < commands_num; i++) {
        if (argc != (commands[i].argc + 1)) {
            continue;
        }
        //skip argv[0] = "mcd"
        int j = 1;
        while ( j < argc) {
            char *arg = commands[i].argv[j-1];
            if (strcmp(arg, "*") != 0 && strcmp(arg, argv[j]) != 0) {
                break;
            }
            j++;
        }
        //command line all matched
        if (j == argc) break;
    }
    if (i < commands_num) {
        struct octvm_command cmd = commands[i];
        if (cmd.command != NULL) {
            //pass args to command_func
            cmd.command(cmd.argc, ++argv);
        }
        return true;
    }
    return false;
}

static void octvm_powermode_config(int argc, char **argv)
{
    if (argc == 3) {
        platform_set_cpu_powermode(argv[2]);
    }
    if (argc == 4 && strcmp(argv[2], "custom_profile") == 0) {
        platform_set_cpu_custprofile(argv[3]);
    }
}

static void octvm_memory_config(int argc, char **argv)
{
    if (argc == 4) {
        zram_swap_set_global_swappiness(atoi(argv[3]));
    }
}

static void octvm_perf_config(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[2], "config_file") == 0) {
        set_sysconfig_custprofile(argv[3]);
    }
    if (argc == 4 && strcmp(argv[2], "cpuset_by_pid") == 0) {
        char cgroup_path[MAX_ARGUMENT_LENGTH];
        sprintf(cgroup_path, "/dev/cpuset/%s/cgroup.procs", argv[3]);
        FILE *fp_cset = fopen(cgroup_path, "a");
        if (fp_cset == NULL) {
            fprintf(stderr, "open %s for write error, %s\n", cgroup_path, strerror(errno));
            return ;
        }
        //write pid to cgroup.procs
        char* pch = NULL;
        while((pch = strtok(argv[4], "#")) != NULL) {
            int pid = atoi(pch);
            fprintf(fp_cset, "%d\n", pid);
        }
        fclose(fp_cset);
    }
}

static void octvm_debug_policy(int argc, char ** argv)
{
    char debugcmd[MAX_ARGUMENT_LENGTH];
    memset(debugcmd, 0, sizeof(debugcmd));
    int ret = 0;
    if (argc == 5 && strcmp(argv[2], "flash") == 0) {
        sprintf(debugcmd, "dd if=%s of=%s", argv[3], argv[4]);
        printf("debugpolicy command: %s\n", debugcmd);
        ret = system(debugcmd);
    }
    else if (argc == 4 && strcmp(argv[2], "clean") == 0) {
        FILE *f = fopen(argv[3],"rb");
        fseek(f,0,SEEK_END);
        unsigned long long len = ftell(f);
        printf("debugpolicy clean device %s, size %llu\n", argv[3], len);
        fseek(f,0,SEEK_SET);
        sprintf(debugcmd, "dd if=/dev/zero of=%s bs=%llu count=1", argv[3], len);
        printf("debugpolicy command: %s\n", debugcmd);
        ret = system(debugcmd);
        fclose(f);
    }
    ALOGI("octvm_debug_policy return %d", ret);
}

static void octvm_mount_ramdump(int argc, char ** argv)
{
    printf("octvm_mount_ramdump, argc = %d\n", argc);
    if (argc == 3 && strcmp(argv[2], "mount") == 0) {
        struct stat st;
        int exist = stat(RAMDUMP_PATH, &st);
        if (exist != 0 && errno == ENOENT) {
            if (mkdir(RAMDUMP_PATH, 0664) < 0) {
                fprintf(stderr, "create ramdump dir failed\n");
                return;
            }
            else {
                chown(RAMDUMP_PATH, AID_SYSTEM, AID_SYSTEM);
                chmod(RAMDUMP_PATH, 0664);
            }
        }
        //mount -t vfat /dev/block/bootdevice/by-name/ramdump /mnt/ramdump
        if (!mount("/dev/block/bootdevice/by-name/ramdump", RAMDUMP_PATH, "vfat", 0, "umask=0022,uid=1000,gid=1000")) {
            printf("mount ramdump successful\n");
        }
        else {
            fprintf(stderr, "mount ramdump dir failed, %s\n", strerror(errno));
        }
    }
    else if (argc == 3 && strcmp(argv[2], "rmflag") == 0) {
        char debugcmd[PROPERTY_VALUE_MAX];
        memset(debugcmd, 0, sizeof(debugcmd));
        sprintf(debugcmd, "rm -f %s", RAMDUMP_FLAG);
        if (system(debugcmd) < 0) {
            fprintf(stderr, "octvm_mount_ramdump command %s failed, %s\n", debugcmd, strerror(errno));
        }
        else {
            printf("octvm_mount_ramdump delete %s success\n", debugcmd);
        }
    }
}

static void trigger_modem_log(void) {
    FILE *fp = fopen("/d/msm_subsys/modem", "w");
    if (fp == NULL) {
        ALOGD("open sys file %s for write error, %s\n", "/d/msm_subsys/modem", strerror(errno));
    } else {
        fprintf(fp, "%s", "restart");
        fclose(fp);
        ALOGD("write restart to /d/msm_subsys success\n");
    }
    return;
}

static void trigger_sched_perf(const char* file_name)
{
    FILE* fp = NULL;
    int ret = chown(file_name, 0, 0);
    if ((fp = fopen(file_name, "r")) != NULL) {
        change_node(fp, file_name);
    }
    return;
}

static void del_tail(char *str) {
    int len = strlen(str);
    if(!len){
        return;
    }
    int i=len-1;

    while (str[i] == '\r' || str[i] == '\n') {
        str[i] = '\0';
        i--;
        if(i < 0){
            return;
        }
    }
}

static bool is_need_compare(const char *node) {
    for (int i=0; i<not_compare_list_size; i++) {
        if (strcmp(node, not_compare_list[i]) == 0)
            return false;
    }
    return true;
}

static void change_node(FILE* fp, const char* filename) {
    char line[MAX_LINE];
    size_t len = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *l = strtok(line, ";");
        char *node = NULL;
        char *value = NULL;
        while (l) {
            char *sharp = strstr(l, "#");
            if (sharp) {
                l[sharp-l] = '\0';
                node = l;
                value = sharp+1;
                del_tail(value);
            } else {
                node = value = NULL;
            }
            l = strtok(NULL, ";");
            if (value != NULL && node != NULL) {
                if((strcmp(value, "ctl.start") == 0) || (strcmp(value,"ctl.stop") == 0)) {
                    property_set(value,node);
                    ALOGD("setprop %s %s\n", value,node);
                    continue;
                }
                FILE* fnode = fopen(node, "rw");
                if (fnode == NULL) {
                    if (checkDebug()) {
                        ALOGE("open perf file %s for write error, %s, still write.\n",
                                strerror(errno), node);
                    } else {
                        ALOGE("open file error: %s, still write.\n", strerror(errno));
                    }
                    char cmd[MAX_ARGUMENT_LENGTH];
                    memset(cmd, 0, sizeof(cmd));
                    sprintf(cmd, "echo %s > %s", value, node);
                    system(cmd);
                    if (checkDebug()) {
                        ALOGD("write %s to %s success!\n", value, node);
                    }
                } else {
                    char cur_value[MAX_ARGUMENT_LENGTH];
                    memset(cur_value, 0, sizeof(cur_value));
                    fgets(cur_value, 100, fnode);
                    // filter \r\n
                    del_tail(cur_value);
                    bool need_compare = is_need_compare(node);
                    if (need_compare && strcmp(value, cur_value) == 0) {
                        fclose(fnode);
                        fnode = NULL;
                        continue;
                    }
                    fclose(fnode);
                    fnode = NULL;
                    char cmd[MAX_ARGUMENT_LENGTH];
                    memset(cmd, 0, sizeof(cmd));
                    sprintf(cmd, "echo %s > %s", value, node);
                    system(cmd);
                    if (checkDebug()) {
                        ALOGD("write %s to %s success!\n", value, node);
                    }
                }
            }
        }
        //ALOGD("node path is: %s, value is: %s", node, value);
    }
    fclose(fp);
    fp = NULL;
    char rmcmd[MAX_ARGUMENT_LENGTH];
    memset(rmcmd, 0, sizeof(rmcmd));
    sprintf(rmcmd, "rm %s", filename);
    system(rmcmd);
}

//only system uid can use this command
static void octvm_sudebug_camera(int argc, char ** argv)
{
    char supercmd[MAX_ARGUMENT_LENGTH];
    memset(supercmd, 0, sizeof(supercmd));
    if (argc == 4 && strcmp(argv[2], "kill_camera_process_by_pid") == 0) {
        char *c = argv[3];
        while (*c != '\0') {
            if (*c == '#') *c = ' '; //replace # with space
            c++;
        }
        sprintf(supercmd, "kill -6 %s", argv[3]);
        printf("run command: %s\n", supercmd);
        if (system(supercmd) < 0) {
            fprintf(stderr, "run command %s failed, %s\n", supercmd, strerror(errno));
        }
    }
}

//only system uid can use this command
static void octvm_sudebug(int argc, char ** argv)
{
    char supercmd[MAX_ARGUMENT_LENGTH];
    memset(supercmd, 0, sizeof(supercmd));

    if (argc == 4 && strcmp(argv[2], "kill_process_by_pid") == 0) {
        char *c = argv[3];
        while (*c != '\0') {
            if (*c == '#') *c = ' '; //replace # with space
            c++;
        }
        sprintf(supercmd, "kill -9 %s", argv[3]);
        printf("run command: %s\n", supercmd);
        if (system(supercmd) < 0) {
            fprintf(stderr, "run command %s failed, %s\n", supercmd, strerror(errno));
        }
    }
    else if (argc == 4 && strcmp(argv[2], "kill_process_by_name") == 0) {
        sprintf(supercmd, "busybox pkill -9 %s", argv[3]);
        printf("run command: %s\n", supercmd);
        if (system(supercmd) < 0) {
            fprintf(stderr, "run command %s failed, %s\n", supercmd, strerror(errno));
        }
    }
    else if (argc == 4 && strcmp(argv[2], "get_iptables") == 0) {
        sprintf(supercmd, "iptables -L -nvx > %s 2>&1", argv[3]);
        printf("run command: %s\n", supercmd);
        if (system(supercmd) < 0) {
            fprintf(stderr, "run command %s failed, %s\n", supercmd, strerror(errno));
        } else {
            //mod the file attribute for accessible
            chown(argv[3], AID_SYSTEM, AID_SYSTEM);
            chmod(argv[3], 0750);
        }
    }
    else if (argc == 4 && strcmp(argv[2], "trace_on") == 0) {
        if (strlen(argv[3]) != 0) {
            char configs[8][100] = {
                    "/d/tracing/events/vmscan/mm_vmscan_kswapd_sleep/enable",
                    "/d/tracing/events/vmscan/mm_vmscan_kswapd_wake/enable",
                    "/d/tracing/events/vmscan/mm_vmscan_wakeup_kswapd/enable",
                    "/d/tracing/events/vmscan/mm_vmscan_direct_reclaim_begin/enable",
                    "/d/tracing/events/vmscan/mm_vmscan_direct_reclaim_end/enable",
                    "/d/tracing/events/vmscan/mm_vmscan_memcg_reclaim_begin/enable",
                    "/d/tracing/events/vmscan/mm_vmscan_memcg_reclaim_end/enable",
                    "/d/tracing/tracing_on" };
            for (int i = 0; i < sizeof(configs) / sizeof(configs[0]); i++) {
                FILE *config_fp = fopen(configs[i], "w");
                if (config_fp == NULL) {
                    fprintf(stderr, "open %s for write failed, %s\n", configs[i], argv[3], strerror(errno));
                    return;
                }
                fprintf(config_fp, "%d", atoi(argv[4]));
                fclose(config_fp);
            }
        }
    }
    else if (argc == 4 && strcmp(argv[2], "trigger_subsys") == 0) {
        ALOGD("trig_subsys_log %s\n", argv[3]);
        int subsys = atoi(argv[3]);
        if (0 == subsys) {
            //trigger catch modem log
            trigger_modem_log();
        }
    }
    else if (argc == 4 && strcmp(argv[2], "sched") == 0) {
        trigger_sched_perf(argv[3]);
    }
    else if (argc == 4 && strcmp(argv[2], "atrace") == 0) {
        int enable = atoi(argv[3]);
        ALOGD("cmd: atrace %d \n", enable);
        if (enable == 1) {
            sprintf(supercmd, "atrace --async_start -z gfx input view wm am pm hal res rs sched freq idle mmc binder_driver binder_lock");
        } else {
            sprintf(supercmd, "atrace --async_dump > /sdcard/trace_out");
        }
        if (system(supercmd) < 0) {
            fprintf(stderr, "run command %s failed, %s\n", supercmd, strerror(errno));
        }
    }
    else if (argc == 5 && strcmp(argv[2], "top") == 0) {
        chmod(argv[3], 0660);
        if (strcmp(argv[4], "ls") == 0) {
            sprintf(supercmd, "top -b -m 10 -n 1 -H -s 6 -o pid,tid,user,pr,ni,%%cpu,s,virt,res,pcy,cmd,name > %s", argv[3]);
        } else {
            sprintf(supercmd, "top -b -m 10 -n 1 -H -s 4 -o pid,tid,user,%%cpu,virt,res,name > %s", argv[3]);
        }
        printf("run command: %s\n", supercmd);
        if (system(supercmd) < 0) {
            fprintf(stderr, "run command %s failed, %s\n", supercmd, strerror(errno));
        }
    }
    else if (argc == 4 && strcmp(argv[2], "dmesg") == 0) {
        chmod(argv[3], 0660);
        sprintf(supercmd, "dmesg -T | tail -200 > %s", argv[3]);
        printf("run command: %s\n", supercmd);
        if (system(supercmd) < 0) {
            fprintf(stderr, "run command %s failed, %s\n", supercmd, strerror(errno));
        }
    }
    else if (argc == 5 && strcmp(argv[2], "set_chain_state") == 0) {
        char parents[2][10] = {"INPUT","OUTPUT"};
        for (char *parent: parents) {
            sprintf(supercmd, "iptables -D %s -j %s\n", parent, argv[3]);
            if (strcmp(argv[4], "enable") == 0) {
                sprintf(supercmd, "%s iptables -A %s -j %s\n",supercmd, parent, argv[3]);
            }
            printf("run command: %s", supercmd);
            if (system(supercmd) < 0) {
                fprintf(stderr, "run command %s failed, %s", supercmd, strerror(errno));
            }
        }
    }
    else if (argc == 6 && strcmp(argv[2], "init_gms_iptable_chain") == 0) {
        sprintf(supercmd, "iptables -N %s || iptables -F %s ", argv[3], argv[3]);
        sprintf(supercmd, "%s && iptables -A %s -m owner --uid-owner %s -j %s",supercmd, argv[3], argv[4], argv[5]);
        printf("run command: %s\n", supercmd);
        if (system(supercmd) < 0) {
            fprintf(stderr, "run command %s failed, %s\n", supercmd, strerror(errno));
        }
    }
    else {
        fprintf(stderr, "unsupported sudebug command!\n");
    }
}

static void octvm_test_client(int argc, char ** argv)
{
    if (argc == 3 && strcmp(argv[1], "cust_console") == 0) {
        //load
        void* test_client_lib = NULL;
        const char* test_lib_path = argv[2];
        if ((test_client_lib = dlopen(test_lib_path, RTLD_NOW | RTLD_LOCAL)) == NULL) {
            fprintf(stdout, "%s not find or load failed\n", test_lib_path);
            return;
        }
        //execute
        int (*test_console_main)(void) = NULL;
        test_console_main = (int (*) (void))dlsym(test_client_lib, "test_console_main");
        if (test_console_main != NULL) {
            test_console_main();
        } else {
            fprintf(stdout, "no console test function found\n");
        }
    }
    else if (argc == 2 && strcmp(argv[1], "wake_lock") == 0) {
        sp<ISystemKloService> sSystemKloProxy = NULL;
        sp<IBinder> binder = defaultServiceManager()->getService(String16("miui.whetstone.klo"));
        if (binder != NULL)
            sSystemKloProxy = ISystemKloService::asInterface(binder);
        if (sSystemKloProxy != NULL) {
            Vector<String16> args;
            args.add(String16("wake_lock"));
            args.add(String16("mcd_test_wake_lock"));
            sSystemKloProxy->klo_system_debug(args);
        } else {
            fprintf(stdout, "cannot get mcd binder service\n");
        }
    }
    else if (argc == 2 && strcmp(argv[1], "wake_unlock") == 0) {
        sp<ISystemKloService> sSystemKloProxy = NULL;
        sp<IBinder> binder = defaultServiceManager()->getService(String16("miui.whetstone.klo"));
        if (binder != NULL)
            sSystemKloProxy = ISystemKloService::asInterface(binder);
        if (sSystemKloProxy != NULL) {
            Vector<String16> args;
            args.add(String16("wake_unlock"));
            args.add(String16("mcd_test_wake_lock"));
            sSystemKloProxy->klo_system_debug(args);
        } else {
            fprintf(stdout, "cannot get mcd binder service\n");
        }
    }
    else if (argc == 2 && strcmp(argv[1], "get_maxbrightness") == 0) {
        int max_brightness = get_lcd_backlight_maxbrightness();
        fprintf(stdout, "%d", max_brightness);
    }
    else if (argc == 3 && strcmp(argv[1], "set_brightness") == 0) {
        int brightness = atoi(argv[2]);
        if (strcmp(argv[2], "0") != 0 && brightness == 0) {
            fprintf(stdout, "invalid brightness value\n");
            return;
        }
        sp<ISystemKloService> sSystemKloProxy = NULL;
        sp<IBinder> binder = defaultServiceManager()->getService(String16("miui.whetstone.klo"));
        if (binder != NULL)
            sSystemKloProxy = ISystemKloService::asInterface(binder);
        if (sSystemKloProxy != NULL) {
            Vector<String16> args;
            args.add(String16("set_brightness"));
            args.add(String16(argv[2]));
            sSystemKloProxy->klo_system_debug(args);
        } else {
            fprintf(stdout, "cannot get mcd binder service\n");
        }
    }
}

static void octvm_ssl_client(int argc, char ** argv)
{
    mc_switch_system_user();

    set_runtime_debug_state(true);
    if (strcmp(argv[1], "update_mid") == 0) {
        if (strstr(gModDevice, "_global") != NULL) {
            update_local_mid(SSL_HOST_NAME_GLB, SSL_PORT, /*NULL, NULL,*/ 0);
        } else {
            update_local_mid(SSL_HOST_NAME, SSL_PORT, /*NULL, NULL,*/ 0);
        }
    }
    else if (strcmp(argv[1], "get_mid") == 0) {
        fprintf(stdout, "local mid: %s", get_local_mid());
    }
}

static void octvm_ssl_client2(int argc, char ** argv)
{
    mc_switch_system_user();

    if (strcmp(argv[1], "get_mid") == 0) {
        String16 anonymous_id;
        sp<ISystemKloService> sSystemKloProxy = NULL;
        // get the binder service
        sp<IBinder> binder = defaultServiceManager()->getService(String16("miui.whetstone.klo"));
        if (binder != NULL) {
            sSystemKloProxy = ISystemKloService::asInterface(binder);
        }
        if (sSystemKloProxy != NULL) {
            sSystemKloProxy->klo_get_anonymous_id(anonymous_id);
        }
        else {
            ALOGE("get SystemKloService failed");
        }
        fprintf(stdout, "local mid: %s", String8(anonymous_id).string());
    }
}
