
#include "SensorControl.h"
#include <SensorEventConnection.h>
#include <SensorDirectConnection.h>
#include <SensorDevice.h>
namespace android {
#define MULTI_USERS_STEP    100000
Mutex gSensorControlLock;
sp<SensorService::SensorControl> gSensorControl = NULL;
sp<SensorService::SensorControl> defaultSensorControl()
{
    if (gSensorControl != NULL) return gSensorControl;
    {
        AutoMutex _l(gSensorControlLock);
        if (gSensorControl == NULL) {
            gSensorControl = new SensorService::SensorControl();
        }
    }
    return gSensorControl;
}
status_t SensorService::SensorControl::stopSensorsByUid(const sp<SensorService>& service,
                                                        uid_t uid) {
    ConnectionSafeAutolock connLock = service->mConnectionHolder.lock(service->mLock);
    for (const sp<SensorEventConnection>& conn : connLock.getActiveConnections()) {
        if (conn->getUid() == uid) {
            SensorDevice& dev(SensorDevice::getInstance());
            dev.setUidControlStateForConnection(conn.get(), true);
        }
    }
    return NO_ERROR;
}
status_t SensorService::SensorControl::resumeSensorsByUid(const sp<SensorService>& service,
                                                          uid_t uid) {
    ConnectionSafeAutolock connLock = service->mConnectionHolder.lock(service->mLock);
    for (const sp<SensorEventConnection>& conn : connLock.getActiveConnections()) {
        if (conn->getUid() == uid) {
            SensorDevice& dev(SensorDevice::getInstance());
            dev.setUidControlStateForConnection(conn.get(), false);
        }
    }
    return NO_ERROR;
}
void SensorService::SensorControl::updateControlTypes(int uid, SortedVector<int> types) {
    auto index = mControlledTypes.find(uid);
    if (!types.isEmpty() ) {
        if (index == mControlledTypes.end() || typeChanged(types, index->second)) {
            mControlledTypes[uid] = types;
        }
    } else if (index != mControlledTypes.end()) {
        mControlledTypes.erase(index);
    }
}
bool SensorService::SensorControl::appIsControlled(const sp<SensorService>& service, uid_t uid,
                                                   int32_t type) const{
    Mutex::Autolock _l(service->mLock);
    return appIsControlledLocked(uid, type);
}
bool SensorService::SensorControl::appIsControlledLocked(uid_t uid, int32_t type) const{
    if (mControlledUids.indexOf(uid) < 0) return false;
    if (type == -1) return true;
    auto types = mControlledTypes.find(uid);
    return types == mControlledTypes.end() || types->second.indexOf(type) >= 0;
}
void SensorService::SensorControl::removeControlApp(const sp<SensorService>& service, uid_t uid) {
    Mutex::Autolock _l(service->mLock);
    mControlledUids.remove(uid);
}
void SensorService::SensorControl::addControlApp(const sp<SensorService>& service, uid_t uid) {
    Mutex::Autolock _l(service->mLock);
    if (mControlledUids.indexOf(uid) < 0) {
        mControlledUids.add(uid);
    }
}
status_t SensorService::SensorControl::executeCommand(const sp<SensorService>& service,
                                                                 const Vector<String16>& args) {
    String8 uid = (String8)args[2];
    int iUid = atoi(uid.string());
    /* just get app's sensor info */
    if ((iUid%MULTI_USERS_STEP) <= 10000) {
        return NO_ERROR;
    }
    Mutex::Autolock _l(mControlLock);
    if (args[1] == String16("stop")) {
        if (appIsControlled(service, iUid, -1)) {
            return NO_ERROR;
        }
        addControlApp(service, iUid);
        stopSensorsByUid(service, iUid);
        return NO_ERROR;
    } else if (args[1] == String16("resume")) {
        if (!appIsControlled(service, iUid, -1)) {
            return NO_ERROR;
        }
        resumeSensorsByUid(service, iUid);
        removeControlApp(service, iUid);
        return NO_ERROR;
    } else if (args[1] == String16("update")){
        SortedVector<int32_t> types;
        String8 arg3 = (String8)args[3];
        if (arg3 != String8("all")) {
            char* buf = new char[strlen(arg3.string()) + 1];
            strcpy(buf, arg3.string());
            char* token = strtok(buf, ",");
            while (token != nullptr) {
                types.add(atoi(token));
                token = strtok(nullptr, ",");
            }
        }
        updateControlTypes(iUid, types);
        return NO_ERROR;
    }
    return INVALID_OPERATION;
}
bool SensorService::SensorControl::typeChanged(SortedVector<int32_t>& x,
                                               SortedVector<int32_t>& y) {
    return x.size() != y.size() || !std::equal(x.begin(), x.end(), y.begin());
}
void SensorService::SensorControl::dumpResultLocked(String8& result) {
    if (mControlledUids.size() > 0) {
        result.appendFormat("controlled apps : \n");
        for(size_t i = 0; i < mControlledUids.size(); i++) {
            result.appendFormat("\t %d", mControlledUids.itemAt(i));
            auto type = mControlledTypes.find(mControlledUids.itemAt(i));
            if (type != mControlledTypes.end()) {
                result.appendFormat(":");
                for (size_t j = 0; j < type->second.size(); ++j) {
                    result.appendFormat("%d ", type->second.itemAt(j));
                }
            }
            result.appendFormat(",");
        }
        result.appendFormat("\n");
    }
}
} // namespace android

