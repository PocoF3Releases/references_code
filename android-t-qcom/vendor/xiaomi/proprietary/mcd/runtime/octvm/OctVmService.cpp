#define LOG_TAG "octvm_runtime"

#include <ctype.h>
#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/PermissionCache.h>
#include <private/android_filesystem_config.h>
#include <cutils/properties.h>

extern "C" {
#include "mc_utils.h"
#include "memcontrol_priv.h"
}

#include "octvm.h"
#include "network.h"
#include "memcontrol.h"
#include "octvm/OctVmService.h"

namespace android {
const bool DEBUG = false;
//dump permission
static const String16 sDump("android.permission.DUMP");

const String8 OctVmService::SName = String8("miui.whetstone.mcd");
const String8 OctVmService::FeaturePropKey = String8("persist.sys.memctrl");

OctVmService::OctVmService()
{
    //empty
}

OctVmService::~OctVmService(){}

const char* OctVmService::getServiceName() {
    return SName;
}

bool OctVmService::isServiceStarted() {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->checkService(String16(SName));
    if (binder != 0) {
        ALOGI("miui.whetstone.mcd is already started");
        return true;
    } else {
        ALOGI("miui.whetstone.mcd is not started");
        return false;
    }
}

bool OctVmService::isFeatureOn()
{
    char feature_status[PROPERTY_VALUE_MAX];
    memset(feature_status, 0, PROPERTY_VALUE_MAX);
    property_get(FeaturePropKey.string(), feature_status, "");
    if (strcmp(feature_status, "on") == 0)
        return true;
    else
        return false;
}

void OctVmService::setFeatureStatus(char *status) {
    property_set(FeaturePropKey.string(), status);
}

void OctVmService::getFeatureStatus(char *status) {
    property_get(FeaturePropKey.string(), status, "");
    ALOGD("control feature %s:%s", FeaturePropKey.string(), status);
}

void OctVmService::createAllSystemCGroups()
{
    for (int i = 0; i < sAllCGroups->size(); i++) {
        struct task_cgroup *cg = sAllCGroups->valueAt(i);
        if (cgroup_new_group(cg) < 0){
            ALOGW("create cgroup %s failed.", cg->groupname);
        }
    }
}

void OctVmService::initAllCGroupTasks()
{
    for (int i = 0; i < sAllCGroups->size(); i++) {
        struct task_cgroup *cg = sAllCGroups->valueAt(i);
        ALOGI("Add default tasks to cgroup %s", cg->groupname);
        for (task_ilist p = cg->def_tasks; p != NULL; p = p->next) {
            int count = 0, pids[128];
            count = mc_get_task_pids(p->uid_name, p->task_name, pids);
            if (count <= 0) continue;
            cgroup_add_tasks(cg->groupname, pids, count);
            ALOGI("def_tasks[%d]: uid:%s, task_name:%s", count, p->uid_name, p->task_name);
        }
    }
}

int32_t OctVmService::local_dns_enable(String16& confPath)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }
    return net_dns_start_server(String8(confPath).string());
}

int32_t OctVmService::get_gpu_load()
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    return ::get_gpu_load();
}

int32_t OctVmService::local_dns_disable()
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }
    net_dns_stop_server();
    return 0;
}

int32_t OctVmService::local_dns_reload()
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    return net_dns_reload();
}

int32_t OctVmService::set_memory_mode(MemorySwapMode mode)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }
    //calling into liboctvm
    return tmc_mode_enter(mode, 0);
}

int32_t OctVmService::application_focused(String16 packageName, int pid, int uid)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    //TODO

    return 0;
}

int32_t OctVmService::application_inactive(String16 packageName, int pid, int uid)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    //TODO

    return 0;
}

int32_t OctVmService::application_started_bg(String16 processName, int pid, int uid)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    //TODO

    return 0;
}

int32_t OctVmService::sudebug_camera_command_execute(const Vector<String16>& args)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    //calling mcd_init to execute as root
    int argc = args.size();
    String8 sudebug_command;
    if (argc == 3
            && String8(args.itemAt(0)) == "sudebug_camera"
            && String8(args.itemAt(1)) == "kill_camera_processes_by_names") {
        String8 procNames(args.itemAt(2));
        String8 procIds;
        convertProcessNamesToIds(procNames, procIds);
        sudebug_command.appendFormat("%s %s %s", "sudebug_camera", "kill_camera_process_by_pid", procIds.string());
    } else {
        for(int index = 0; index < argc; index++) {
            sudebug_command.appendFormat("%s", String8(args.itemAt(index)).string());
            if (index < argc-1)
                sudebug_command.appendFormat(" ");
        }
    }
    property_set(MCD_EXTRA_PARAMS, sudebug_command.string());
    property_set("ctl.start", "mcd_init");

    return 0;
}

int32_t OctVmService::sudebug_command_execute(const Vector<String16>& args)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    //calling mcd_init to execute as root
    int argc = args.size();
    String8 sudebug_command;
    if (argc == 3
            && String8(args.itemAt(0)) == "sudebug"
            && String8(args.itemAt(1)) == "kill_processes_by_names") {
        String8 procNames(args.itemAt(2));
        String8 procIds;
        convertProcessNamesToIds(procNames, procIds);
        sudebug_command.appendFormat("%s %s %s", "sudebug", "kill_process_by_pid", procIds.string());
    } else {
        for(int index = 0; index < argc; index++) {
            sudebug_command.appendFormat("%s", String8(args.itemAt(index)).string());
            if (index < argc-1)
                sudebug_command.appendFormat(" ");
        }
    }
    property_set(MCD_EXTRA_PARAMS, sudebug_command.string());
    property_set("ctl.start", "mcd_init");

    return 0;
}

int32_t OctVmService::convertProcessNamesToIds(const String8& procNames, String8& outProcIds)
{
    char pProcNames[procNames.length() + 1];
    strcpy(pProcNames,procNames.string());
    Vector<const char *> namesVector;
    char *result = strtok(pProcNames, "#");
    while(result != NULL) {
        if (strlen(result) != 0) {
            namesVector.add(result);
        }
        result = strtok(NULL, "#");
    }
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        ALOGE("convertProcessNamesToIds could not open proc");
        return -1;
    }
    char filename[64];
    char cmdline[256];
    FILE *file;
    struct dirent *pid_dir;
    while((pid_dir = readdir(proc_dir)) != NULL) {
        if (!isdigit(pid_dir->d_name[0])) {
            continue;
        }
        memset(filename, 0, sizeof(filename));
        sprintf(filename, "/proc/%s/cmdline", pid_dir->d_name);
        file = fopen(filename, "r");
        if (file == NULL) {
            ALOGE("convertProcessNamesToIds open filename=%s failed", filename);
            continue;
        }
        memset(cmdline, 0, sizeof(cmdline));
        fgets(cmdline, 255, file);
        fclose(file);
        // e.g: cmdline=/system/bin/logd pName=logd
        const char* cmdlineProcName = strrchr(cmdline, '/');
        if (cmdlineProcName == NULL) {
            cmdlineProcName = cmdline;
        } else {
            cmdlineProcName++;
        }
        if (cmdlineProcName == NULL || strlen(cmdlineProcName) == 0) {
            continue;
        }
        if (DEBUG) ALOGD("convertProcessNamesToIds pid=%s cmdline=%s name=%s"
                , pid_dir->d_name, cmdline, cmdlineProcName);
        int nameSize = namesVector.size();
        for(int i = 0; i < nameSize; i++) {
            if (strcmp(cmdlineProcName, namesVector[i]) == 0) {
                if (!outProcIds.isEmpty()) {
                    outProcIds.appendFormat("%s", "#");
                }
                outProcIds.appendFormat("%s", pid_dir->d_name);
                namesVector.removeAt(i);
                nameSize--;
                break;
            }
        }
        if (nameSize == 0) {
            break;
        }
    }
    closedir(proc_dir);
    if (DEBUG) ALOGD("convertProcessNamesToIds input=%s output=%s", String8(procNames).string(), outProcIds.string());
    return 0;
}

int32_t OctVmService::get_pid_by_name(Vector<const char*>& args
        , Vector<key_value_pair_t<String8, String8>>& outNameId) {
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        ALOGE("get_pid_by_name could not open proc");
        return -1;
    }
    char filename[64];
    char cmdline[256];
    FILE *file;
    struct dirent *pid_dir;
    int nameSize = args.size();
    while((pid_dir = readdir(proc_dir)) != NULL && (nameSize > 0)) {
        if (!isdigit(pid_dir->d_name[0])) {
            continue;
        }
        memset(filename, 0, sizeof(filename));
        sprintf(filename, "/proc/%s/cmdline", pid_dir->d_name);
        file = fopen(filename, "r");
        if (file == NULL) {
            ALOGE("get_pid_by_name open filename=%s failed", filename);
            continue;
        }
        memset(cmdline, 0, sizeof(cmdline));
        fgets(cmdline, 255, file);
        fclose(file);
        // e.g: cmdline=/system/bin/logd pName=logd
        const char* cmdlineProcName = strrchr(cmdline, '/');
        if (cmdlineProcName == NULL) {
            cmdlineProcName = cmdline;
        } else {
            cmdlineProcName++;
        }
        if (cmdlineProcName == NULL || strlen(cmdlineProcName) == 0) {
            continue;
        }
        if (DEBUG) ALOGD("get_pid_by_name pid=%s cmdline=%s name=%s"
                , pid_dir->d_name, cmdline, cmdlineProcName);
        for(int i = 0; i < nameSize; i++) {
            if (strcmp(args[i], cmdlineProcName) == 0) {
                outNameId.add(key_value_pair_t<String8, String8>(String8(args[i]), String8(pid_dir->d_name)));
                args.removeAt(i);
                nameSize--;
                break;
            }
        }
    }
    closedir(proc_dir);
    return 0;
}

int32_t OctVmService::dump(int fd, const Vector<String16>& args)
{
    String8 result;
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    pid_t callingPid = IPCThreadState::self()->getCallingPid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM && callingUid != AID_SHELL &&
            !PermissionCache::checkPermission(sDump, callingPid, callingUid)) {
        result.appendFormat("Permission Denial: "
            "can't dump SurfaceFlinger from pid=%d, uid=%d\n", callingPid, callingUid);
        write(fd, result.string(), result.size());
    } else {
        //dump memory control(cgroup) info
        if (1) {
            result.appendFormat("ignore for avoid fc in some case.\n");
            write(fd, result.string(), result.size());
        } else
        if (isFeatureOn()) {
            for (int i = 0; i < sAllCGroups->size(); i++) {
                struct task_cgroup *cg = sAllCGroups->valueAt(i);
                cgroup_dump_print(fd, cg->groupname);
            }
            //dump tasks not in any cgroup
            cgroup_dump_print(fd, " ");
        }
        else {
            result.appendFormat("memory control feature not enabled on the target.\n");
            write(fd, result.string(), result.size());
        }
    }
    return 0;
}

KeyedVector<String8, struct task_cgroup *>* OctVmService::sAllCGroups = new KeyedVector<String8, struct task_cgroup *>();

void OctVmService::addSystemCGroup(struct task_cgroup *cg)
{
    sAllCGroups->add(String8(cg->groupname), cg);
}

struct task_cgroup* OctVmService::getSystemCGroup(String8 groupname)
{
    if (sAllCGroups->indexOfKey(groupname) >= 0) {
        return sAllCGroups->valueFor(groupname);
    }
    return NULL;
}

}; // namespace android
