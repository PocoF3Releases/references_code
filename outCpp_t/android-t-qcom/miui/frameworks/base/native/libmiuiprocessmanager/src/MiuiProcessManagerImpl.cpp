#undef LOG_TAG
#define LOG_TAG "MiuiProcessManagerImpl"

#include "MiuiProcessManagerImpl.h"
#include <android-base/stringprintf.h>

using namespace android;

MiuiProcessManagerImpl* MiuiProcessManagerImpl::sImplInstance = NULL;
std::mutex MiuiProcessManagerImpl::mInstLock;

MiuiProcessManagerImpl* MiuiProcessManagerImpl::getInstance() {
    std::lock_guard<std::mutex> lock(mInstLock);
    if (!sImplInstance) {
        sImplInstance = new MiuiProcessManagerImpl();
    }
    return sImplInstance;
}

bool MiuiProcessManagerImpl::setSchedFifo(const ::std::vector<int32_t>& tids,
                                        int64_t duration, int32_t pid, int32_t mode) {
    ALOGI("setSchedFifo pid:%d, mode:%d", pid, mode);
    const sp<IServiceManager> sm(defaultServiceManager());
    sp <IBinder> binder = sm->getService(String16("ProcessManager"));
    sp<IProcessManager> mSpeedui;
    if (binder != nullptr) {
        mSpeedui = interface_cast<IProcessManager> (binder);
    } else {
        return false;
    }
    mSpeedui->beginSchedThreads(tids, duration, pid, mode);
    return true;
}

extern "C" IMiuiProcessManagerServiceStub* createMiuiProcessManager() {
    return MiuiProcessManagerImpl::getInstance();
}

extern "C" void destroyMiuiProcessManager(IMiuiProcessManagerServiceStub* impl) {
    delete impl;
}
