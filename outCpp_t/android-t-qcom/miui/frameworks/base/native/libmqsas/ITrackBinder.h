//
// Created by wt on 20-8-17.
//

#ifndef ANDROID_ITRACKBINDER_H
#define ANDROID_ITRACKBINDER_H

#include <binder/IInterface.h>

namespace android {

class ITrackBinder : public IInterface {
public:
    DECLARE_META_INTERFACE(TrackBinder);
    virtual void trackEvent(const String16& appId, const String16& pkn, const String16& data, int flag) = 0;
    virtual void trackEvents(const String16& appId, const String16& pkn, const std::vector<String16>& dataList, int flag) = 0;


    enum {
        REPORT_TRACK_EVENT = IBinder::FIRST_CALL_TRANSACTION,
        REPORT_TRACK_EVENTS = IBinder::FIRST_CALL_TRANSACTION + 1,
    };
};

} // namespace android

#endif //ANDROID_ITRACKBINDER_H