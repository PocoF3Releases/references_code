#ifndef ANDROID_MIUIPROCESSMANAGERSERVICESTUB_H
#define ANDROID_MIUIPROCESSMANAGERSERVICESTUB_H
#include "IMiuiProcessManagerServiceStub.h"
#include <mutex>
#include <dlfcn.h>
#include <log/log.h>

#if defined(__LP64__)
#define LIB_MIUIPROCESS_PATH "/system/lib64/libmiuiprocessmanager.so"
#else
#define LIB_MIUIPROCESS_PATH "/system/lib/libmiuiprocessmanager.so"
#endif

namespace android {

class MiuiProcessManagerServiceStub {
private:
    MiuiProcessManagerServiceStub() {}
    static void *LibMiuiProcessManagerServiceImpl;
    static IMiuiProcessManagerServiceStub *ImplInstance;
    static IMiuiProcessManagerServiceStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

public:
    virtual ~MiuiProcessManagerServiceStub() {}
    static bool setSchedFifo(const ::std::vector<int32_t>& tids, int64_t duration,
            int32_t pid, int32_t mode);

};
}
#endif //ANDROID_MIUIPROCESSMANAGERSERVICESTUB_H
