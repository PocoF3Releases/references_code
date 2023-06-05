#include "androidfw/processmanager/MiuiProcessManagerServiceStub.h"

using namespace android;
void* MiuiProcessManagerServiceStub::LibMiuiProcessManagerServiceImpl = nullptr;
IMiuiProcessManagerServiceStub* MiuiProcessManagerServiceStub::ImplInstance = nullptr;
std::mutex MiuiProcessManagerServiceStub::StubLock;

IMiuiProcessManagerServiceStub* MiuiProcessManagerServiceStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!LibMiuiProcessManagerServiceImpl) {
        LibMiuiProcessManagerServiceImpl = dlopen(LIB_MIUIPROCESS_PATH, RTLD_LAZY);
        if (LibMiuiProcessManagerServiceImpl) {
            CreateMiuiProcessImpl* create = (CreateMiuiProcessImpl *)dlsym
                                     (LibMiuiProcessManagerServiceImpl, "createMiuiProcessManager");
            if (create) {
                ImplInstance = create();
            } else {
                ALOGE("MiuiProcessManagerServiceStub dlsym failed \"create\", reason:%s",
                    dlerror());
            }
        } else {
            ALOGE("MiuiProcessManagerServiceStub open %s failed! dlopen - %s",
                LIB_MIUIPROCESS_PATH, dlerror());
        }
    }
    return ImplInstance;
}

void MiuiProcessManagerServiceStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibMiuiProcessManagerServiceImpl) {
        DestroyMiuiProcessImpl* destroy = (DestroyMiuiProcessImpl *)dlsym
                                    (LibMiuiProcessManagerServiceImpl, "destroyMiuiProcessManager");
        if (destroy) {
            destroy(ImplInstance);
            dlclose(LibMiuiProcessManagerServiceImpl);
            LibMiuiProcessManagerServiceImpl = nullptr;
            ImplInstance = nullptr;
        } else {
            ALOGE("MiuiProcessManagerServiceStub dlsym failed \"destroy\", reason:%s" , dlerror());
        }
    }
}

bool MiuiProcessManagerServiceStub::setSchedFifo(const ::std::vector<int32_t>& tids,
                                                    int64_t duration, int32_t pid, int32_t mode) {
    IMiuiProcessManagerServiceStub* Istub = GetImplInstance();
    if (Istub) {
        ALOGD("MiuiProcessManagerServiceStub %s", __func__);
        return Istub->setSchedFifo(tids, duration, pid, mode);
    } else {
        ALOGD("MiuiProcessManagerServiceStub not found IMiuiProcessManagerServiceStub");
        return false;
    }
    return false;
}
