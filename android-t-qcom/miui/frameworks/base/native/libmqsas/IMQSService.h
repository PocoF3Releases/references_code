#ifndef MIUI_MQS_SERVICE_H
#define MIUI_MQS_SERVICE_H

#include <binder/IInterface.h>

namespace android {

class IMQSService : public IInterface {
public:
    DECLARE_META_INTERFACE(MQSService);
    virtual void reportSimpleEvent(int type, const String16& error) = 0;
    virtual void reportNativeEventV2(const String16& module, const String16& info, const String16& appId, int isGlobalNeed) = 0;
    virtual void reportNativeFeatureEvent(const String16& message, const String16& details, const String16& type) = 0;
    virtual void appCaptureLog(const String16& module, const String16& actionList, const String16& logPath, const String16& servicesList) = 0;
    virtual void reportNativeEventsV2(const String16& module, const std::vector<String16>& infolist, const String16& appId, int isGlobalNeed) = 0;
    virtual sp<IBinder> getOneTrackService() = 0;

    enum {
        REPORT_SIMPLE_EVENT = IBinder::FIRST_CALL_TRANSACTION,
        REPORT_NATIVE_EVENT_V2 = IBinder::FIRST_CALL_TRANSACTION + 1,
        REPORT_FEATURE_EVENT = IBinder::FIRST_CALL_TRANSACTION + 2,
        APP_CAPTURE_LOG = IBinder::FIRST_CALL_TRANSACTION + 3,
        REPORT_NATIVE_EVENTS_V2 = IBinder::FIRST_CALL_TRANSACTION + 4,
        GET_TRACKBINDER = IBinder::FIRST_CALL_TRANSACTION + 5,
    };
};

} // namespace android

#endif // MIUI_MQS_SERVICE_H
