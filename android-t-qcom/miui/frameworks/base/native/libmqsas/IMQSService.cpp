#include <binder/Parcel.h>
#include "IMQSService.h"

namespace android {

class BpMQSService : public BpInterface<IMQSService> {
public:
    BpMQSService(const sp<IBinder>& impl)
        : BpInterface<IMQSService>(impl) {}

    virtual void reportSimpleEvent(int type, const String16& error) {
        Parcel data, reply;
        data.writeInterfaceToken(IMQSService::getInterfaceDescriptor());
        data.writeInt32(type);
        data.writeString16(error);
        remote()->transact(REPORT_SIMPLE_EVENT, data, &reply, IBinder::FLAG_ONEWAY);
    }

    virtual void reportNativeEventV2(const String16& module, const String16& info, const String16& appId, int isGlobalNeed) {
        Parcel data, reply;
        data.writeInterfaceToken(IMQSService::getInterfaceDescriptor());
        data.writeString16(module);
        data.writeString16(info);
        data.writeString16(appId);
        data.writeInt32(isGlobalNeed);
        remote()->transact(REPORT_NATIVE_EVENT_V2, data, &reply, IBinder::FLAG_ONEWAY);
    }

    virtual void reportNativeEventsV2(const String16& module, const std::vector<String16>& infolist, const String16& appId, int isGlobalNeed) {
        Parcel data, reply;
        data.writeInterfaceToken(IMQSService::getInterfaceDescriptor());
        data.writeString16(module);
        data.writeString16Vector(infolist);
        data.writeString16(appId);
        data.writeInt32(isGlobalNeed);
        remote()->transact(REPORT_NATIVE_EVENTS_V2, data, &reply, IBinder::FLAG_ONEWAY);
    }

    virtual void reportNativeFeatureEvent(const String16& message, const String16& details, const String16& type) {
        Parcel data, reply;
        data.writeInterfaceToken(IMQSService::getInterfaceDescriptor());
        data.writeString16(message);
        data.writeString16(details);
        data.writeString16(type);
        remote()->transact(REPORT_FEATURE_EVENT, data, &reply, IBinder::FLAG_ONEWAY);
    }

    virtual void appCaptureLog(const String16& module, const String16& actionList, const String16& logPath, const String16& servicesList) {
        Parcel data, reply;
        data.writeInterfaceToken(IMQSService::getInterfaceDescriptor());
        data.writeString16(module);
        data.writeString16(actionList);
        data.writeString16(logPath);
        data.writeString16(servicesList);
        remote()->transact(APP_CAPTURE_LOG, data, &reply, IBinder::FLAG_ONEWAY);
    }

    virtual sp<IBinder> getOneTrackService() {
        Parcel data, reply;
        data.writeInterfaceToken(IMQSService::getInterfaceDescriptor());
        remote()->transact(GET_TRACKBINDER, data, &reply);
        // fail on exception
        if (reply.readExceptionCode() != 0) {
            ALOGE("Exception on getOneTrackService");
            return nullptr;
        }
        return reply.readStrongBinder();
    }
};

IMPLEMENT_META_INTERFACE(MQSService, "miui.mqsas.IMQSService");

} // namespace android
