#ifndef ANDROID_SENSORSERVICE_STUB_H
#define ANDROID_SENSORSERVICE_STUB_H
#include <mutex>
#include "IMiSensorServiceStub.h"

#define LIBPATH "libmisensorserviceimpl.so"

using namespace android;

class MiSensorServiceStub {
    MiSensorServiceStub() {}
    static void *LibMiSensorServiceImpl;
    static IMiSensorServiceStub *ImplInstance;
    static IMiSensorServiceStub *GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;

public:
    virtual ~MiSensorServiceStub() {}
    static bool isonwhitelist(String8 packageName);
    static bool isSensorDisableApp(const String8& packageName);
    static void setSensorDisableApp(const String8& packageName);
    static void removeSensorDisableApp(const String8& packageName);
    static String8 getDumpsysInfo();
    static void SaveSensorUsageItem(int32_t sensorType, const String8& sensorName,
                                       const String8& packageName, bool activate);
    static void handleProxUsageTime(const sensors_event_t& event, bool report);
    static void handlePocModeUsageTime(const sensors_event_t& event, bool report);
    static void* timeToReport(void *arg);
    static void handleSensorEvent(const sensors_event_t& event);
    static void dumpResultLocked(String8& result);
    static bool appIsControlledLocked(uid_t uid, int32_t type);
    static status_t executeCommand(const sp<SensorService>& service, const Vector<String16>& args);
};

#endif
