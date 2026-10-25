//
// Created by wt on 20-8-17.
//

#include <binder/Parcel.h>
#include "ITrackBinder.h"

namespace android {

class BpTrackBinder : public BpInterface<ITrackBinder> {
public:
    BpTrackBinder(const sp<IBinder>& impl)
        : BpInterface<ITrackBinder>(impl) {}

    virtual void trackEvent(const String16& appId, const String16& pkn, const String16& data, int flag) {
        Parcel parcelData, reply;
        parcelData.writeInterfaceToken(ITrackBinder::getInterfaceDescriptor());
        parcelData.writeString16(appId);
        parcelData.writeString16(pkn);
        parcelData.writeString16(data);
        parcelData.writeInt32(flag);
        remote()->transact(REPORT_TRACK_EVENT, parcelData, &reply, IBinder::FLAG_ONEWAY);
    }

    virtual void trackEvents(const String16& appId, const String16& pkn, const std::vector<String16>& dataList, int flag) {
        Parcel parcelData, reply;
        parcelData.writeInterfaceToken(ITrackBinder::getInterfaceDescriptor());
        parcelData.writeString16(appId);
        parcelData.writeString16(pkn);
        parcelData.writeString16Vector(dataList);
        parcelData.writeInt32(flag);
        remote()->transact(REPORT_TRACK_EVENTS, parcelData, &reply, IBinder::FLAG_ONEWAY);
    }
};

IMPLEMENT_META_INTERFACE(TrackBinder, "com.miui.analytics.ITrackBinder");

} // namespace android
