#ifndef ANDROID_SENSOR_CONTROL_H
#define ANDROID_SENSOR_CONTROL_H

#include "SensorService.h"
#include "MiSensorServiceImpl.h"
namespace android {

class SensorService::SensorControl:public virtual RefBase {
public:
    SensorControl(){};
    virtual ~SensorControl(){};
    // protected by mService->mLock
    void dumpResultLocked(String8& result);
    bool appIsControlled(const sp<SensorService>& service, uid_t uid, int32_t type) const;
    bool appIsControlledLocked(uid_t uid, int32_t type) const;
    // protected by mControlLock
    status_t executeCommand(const sp<SensorService>& service, const Vector<String16>& args);
    mutable Mutex mControlLock;
private:
    status_t stopSensorsByUid(const sp<SensorService>& service, uid_t uid);
    status_t resumeSensorsByUid(const sp<SensorService>& service, uid_t uid);

    // protected by mService->mLock as follows
    void removeControlApp(const sp<SensorService>& service, uid_t uid);
    void addControlApp(const sp<SensorService>& service, uid_t uid);
    void updateControlTypes(int uid, SortedVector<int32_t> vector);
    bool typeChanged(SortedVector<int32_t>& x, SortedVector<int32_t>& y);
    SortedVector<int32_t> mControlledUids;
    std::map<uid_t, SortedVector<int32_t>> mControlledTypes;
};

sp<SensorService::SensorControl> defaultSensorControl();
}

#endif // ANDROID_SENSOR_RECORD_H
