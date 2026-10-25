#include <binder/Parcel.h>
#include "IProcessManager.h"

namespace android {

class BpProcessManager : public BpInterface<IProcessManager> {
public:
    BpProcessManager(const sp<IBinder>& impl)
        : BpInterface<IProcessManager>(impl) {}

    virtual void beginSchedThreads(const ::std::vector<int32_t>& tids, int64_t duration,
                                    int32_t pid, int32_t mode) {
        Parcel parcelData, reply;
        parcelData.writeInterfaceToken(IProcessManager::getInterfaceDescriptor());
        parcelData.writeInt32Vector(tids);
        parcelData.writeInt64(duration);
        parcelData.writeInt32(pid);
        parcelData.writeInt32(mode);
        remote()->transact(BEGIN_SCHED_THREADS_TRANSACTION,
            parcelData, &reply, IBinder::FLAG_ONEWAY);
    }

    virtual void reportTrackStatus(int32_t uid, int32_t pid, int32_t sessionId, bool isMuted) {
        Parcel parcelData, reply;
        parcelData.writeInterfaceToken(IProcessManager::getInterfaceDescriptor());
        parcelData.writeInt32(uid);
        parcelData.writeInt32(pid);
        parcelData.writeInt32(sessionId);
        parcelData.writeBool(isMuted);
        remote()->transact(REPORT_TRACK_STATUS_TRANSACTION,
            parcelData, &reply, IBinder::FLAG_ONEWAY);
    }
};

IMPLEMENT_META_INTERFACE(ProcessManager, "miui.IProcessManager");

} // namespace android
