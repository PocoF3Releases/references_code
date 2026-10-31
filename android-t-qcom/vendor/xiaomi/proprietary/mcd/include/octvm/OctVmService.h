#ifndef OCTVM_MEMCTRLPROXY_H
#define OCTVM_MEMCTRLPROXY_H

#include <utils/String8.h>
#include <utils/String16.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "memcontrol.h"
#include "octvm/IOctVmService.h"

namespace android {

//memctrl operation proxy
class OctVmService : public BinderService<OctVmService>, public BnOctVmService
{
    friend class BinderService<OctVmService>;

public:
    OctVmService();
    ~OctVmService();

    static const char* getServiceName();

    static bool isServiceStarted();

    static bool isFeatureOn();

    static void setFeatureStatus(char *status);

    static void getFeatureStatus(char *status);

    static void createAllSystemCGroups();

    static void initAllCGroupTasks();

    int32_t local_dns_enable(String16& confPath);

    int32_t local_dns_disable();

    int32_t local_dns_reload();

    int32_t get_gpu_load();

    int32_t set_memory_mode(MemorySwapMode mode);

    int32_t application_focused(String16 packageName, int pid, int uid);

    int32_t application_inactive(String16 packageName, int pid, int uid);

    int32_t application_started_bg(String16 processName, int pid, int uid);

    int32_t sudebug_camera_command_execute(const Vector<String16>& args);

    int32_t sudebug_command_execute(const Vector<String16>& args);

    int32_t get_pid_by_name(Vector<const char*>& args, Vector<key_value_pair_t<String8, String8>>& outNameId);

    int32_t dump(int fd, const Vector<String16>& args);

private:
    int32_t convertProcessNamesToIds(const String8& procNames, String8& outProcIds);

public:
    static const String8 SName;
    static const String8 FeaturePropKey;

    static void addSystemCGroup(struct task_cgroup *cg);
    static struct task_cgroup * getSystemCGroup(String8 groupname);

private:
    static KeyedVector<String8, struct task_cgroup *> *sAllCGroups;
};

};

#endif
