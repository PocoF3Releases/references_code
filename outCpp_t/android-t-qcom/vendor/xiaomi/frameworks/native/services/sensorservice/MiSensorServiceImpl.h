#ifndef ANDROID_ISWHITEAPP_IMPL_H
#define ANDROID_ISWHITEAPP_IMPL_H

#include <log/log.h>
#include <utils/Errors.h>
#include <utils/String8.h>
#include <IMiSensorServiceStub.h>
#include "SensorControl.h"

namespace android {

class MiSensorServiceImpl : public IMiSensorServiceStub {
public:
    virtual ~MiSensorServiceImpl() {}
    virtual bool isonwhitelist(String8 packageName);
    // MIUI ADD:
    Vector<String8> mSensorDisableNames;
    // END
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
extern "C" IMiSensorServiceStub* create();
extern "C" void destroy(IMiSensorServiceStub* impl);

}//namespace android

#endif