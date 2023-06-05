#ifndef ANDROID_MISENSORSERVICE_H
#define ANDROID_MISENSORSERVICE_H

#include <android-base/macros.h>
#include <cutils/atomic.h>
#include <cutils/compiler.h>
#include <cutils/properties.h>
#include <utils/Errors.h>
#include <utils/Vector.h>
#include <utils/String8.h>
#include "SensorService.h"

using namespace android;


class IMiSensorServiceStub {
public:
    virtual ~IMiSensorServiceStub() {}
    virtual bool isonwhitelist(String8 packageName);
    virtual bool isSensorDisableApp(const String8& packageName);
    virtual void setSensorDisableApp(const String8& packageName);
    virtual void removeSensorDisableApp(const String8& packageName);
    virtual String8 getDumpsysInfo();
    virtual void SaveSensorUsageItem(int32_t sensorType, const String8& sensorName,
                                           const String8& packageName, bool activate);
    virtual void handleProxUsageTime(const sensors_event_t& event, bool report);
    virtual void handlePocModeUsageTime(const sensors_event_t& event, bool report);
    virtual void* timeToReport(void *arg);
    virtual bool enableSensorEventToOnetrack();
    virtual void handleSensorEvent(const sensors_event_t& event);
    virtual void dumpResultLocked(String8& result);
    virtual bool appIsControlledLocked(uid_t uid, int32_t type);
    virtual status_t executeCommand(const sp<SensorService>& service, const Vector<String16>& args);
};

typedef IMiSensorServiceStub* Create();
typedef void Destroy(IMiSensorServiceStub *);

#endif
