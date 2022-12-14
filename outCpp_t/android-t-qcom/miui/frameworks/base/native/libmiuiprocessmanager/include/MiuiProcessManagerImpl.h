#ifndef ANDROID_MIUIPROCESSMANAGERIMPL_H
#define ANDROID_MIUIPROCESSMANAGERIMPL_H

#include <IProcessManager.h>
#include "androidfw/processmanager/IMiuiProcessManagerServiceStub.h"

#include <binder/IServiceManager.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
#include <limits.h>
#include <utils/Mutex.h>
#include <utils/Looper.h>
#include <utils/Timers.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <android-base/strings.h>
#include <errno.h>
#include <android-base/file.h>
#include <mutex>

namespace android {

class MiuiProcessManagerImpl : public IMiuiProcessManagerServiceStub, public virtual RefBase {
private:
    MiuiProcessManagerImpl() {}
    static MiuiProcessManagerImpl* sImplInstance;
    static std::mutex mInstLock;

public:
    virtual ~MiuiProcessManagerImpl() {}
    static MiuiProcessManagerImpl* getInstance();
    virtual bool setSchedFifo(const ::std::vector<int32_t>& tids, int64_t duration,
            int32_t pid, int32_t mode);

};

__attribute__((__weak__, visibility("default")))
extern "C" IMiuiProcessManagerServiceStub* createMiuiProcessManager();

__attribute__((__weak__, visibility("default")))
extern "C" void destroyMiuiProcessManager(IMiuiProcessManagerServiceStub* impl);

}

#endif //ANDROID_MIUIPROCESSMANAGERIMPL_H
