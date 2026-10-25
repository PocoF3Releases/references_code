#ifndef MIUI_MQS_SERVICE_MANAGER_H
#define MIUI_MQS_SERVICE_MANAGER_H

#include <utils/Singleton.h>
#include "IMQSService.h"
#include "ITrackBinder.h"

namespace android {

class MQSServiceManager: public Singleton<MQSServiceManager> {
    friend class Singleton<MQSServiceManager>;

    MQSServiceManager();
    void connectMQSServiceLocked();
    void onMQSServiceDied();
    void connectTrackBinderLocked();
    void onTrackBinderDied();

    sp<IMQSService> mMQSService;
    sp<ITrackBinder> mTrackBinder;
    sp<IBinder::DeathRecipient> mDeathObserver;

    mutable Mutex mLock;

    enum {
        EVENT_SENSOR = 67,
    };

    class DeathObserver : public IBinder::DeathRecipient {
        MQSServiceManager& mMQSServiceManager;
        virtual void binderDied(const wp<IBinder>& who) {
            ALOGE("Mqsas service died [%p]", who.unsafe_get());
            mMQSServiceManager.onMQSServiceDied();
            mMQSServiceManager.onTrackBinderDied();
        }
    public:
        DeathObserver(MQSServiceManager& mgr) : mMQSServiceManager(mgr) { }
    };

public:
    void reportSensorException(const String8& error);
    void reportEventV2(const String8& module, const String8& info, const String8& appId, bool isGlobalNeed);
    void reportNativeFeatureEvent(const String8& message, const String8& details, const String8& type);
    void appCaptureLog(const String8& module, const String8& actionList, const String8& logPath, const String8& servicesList);
    void reportEventsV2(const String8& module, const std::vector<String16>& infolist, const String8& appId, bool isGlobalNeed);
    /* return result code, return 0 if called onetrack successfully, otherwise return 1*/
    int reportOneTrackEvent(const String8& appId, const String8& pkn, const String8& data, int flag);
    int reportOneTrackEvents(const String8& appId, const String8& pkn, const std::vector<String16>& dataList, int flag);
};

} // namespace android

#endif // MIUI_MQS_SERVICE_MANAGER_H
