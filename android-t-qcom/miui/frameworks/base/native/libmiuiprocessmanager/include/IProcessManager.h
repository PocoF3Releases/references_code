#ifndef ANDROID_IPROCESSMANAGER_H
#define ANDROID_IPROCESSMANAGER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class IProcessManager : public IInterface {
public:
    DECLARE_META_INTERFACE(ProcessManager);
    virtual void beginSchedThreads(const ::std::vector<int32_t>& tids, int64_t duration,
        int32_t pid, int32_t mode) = 0;
    virtual void reportTrackStatus(int32_t uid, int32_t pid, int32_t sessionId, bool isMuted) = 0;

    enum {
        BEGIN_SCHED_THREADS_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION + 28,
        REPORT_TRACK_STATUS_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION + 33,
    };
};

} // namespace android

#endif //ANDROID_IPROCESSMANAGER_H