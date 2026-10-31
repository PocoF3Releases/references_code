#include <utils/Log.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include "SensorControl.h"

namespace android {

#define LOG_TAG "SensorControl"

#define WAIT_PERIOD_MS          10

static char const* getSensorName(int type) {
    switch(type) {
        case SENSOR_TYPE_ACCELEROMETER:
            return "Acc";
        case SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED:
            return "Mag";
        case SENSOR_TYPE_MAGNETIC_FIELD:
            return "MagCal";
        case SENSOR_TYPE_ORIENTATION:
            return "Ori";
        case SENSOR_TYPE_GYROSCOPE:
            return "Gyr";
        case SENSOR_TYPE_LIGHT:
            return "Lux";
        case SENSOR_TYPE_LIGHT_MI:
            return "Lux";
        case SENSOR_TYPE_PRESSURE:
            return "Bar";
        case SENSOR_TYPE_TEMPERATURE:
            return "Tmp";
        case SENSOR_TYPE_PROXIMITY:
            return "Prx";
        case SENSOR_TYPE_GRAVITY:
            return "Grv";
        case SENSOR_TYPE_LINEAR_ACCELERATION:
            return "Lac";
        case SENSOR_TYPE_ROTATION_VECTOR:
            return "Rot";
        case SENSOR_TYPE_RELATIVE_HUMIDITY:
            return "Hum";
        case SENSOR_TYPE_AMBIENT_TEMPERATURE:
            return "Tam";
    }

    static char buf[64];
    sprintf(buf, "%d", type);
    return buf;
}

static int getSensorType(char const* type) {
    if (strcmp(type, "Acc") == 0) {
        return SENSOR_TYPE_ACCELEROMETER;
    }
    else if (strcmp(type, "Mag") == 0) {
        return SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED;
    }
    else if (strcmp(type, "MagCal") == 0) {
        return SENSOR_TYPE_MAGNETIC_FIELD;
    }
    else if (strcmp(type, "Ori") == 0) {
        return SENSOR_TYPE_ORIENTATION;
    }
    else if (strcmp(type, "Gyr") == 0) {
        return SENSOR_TYPE_GYROSCOPE;
    }
    else if (strcmp(type, "Lux") == 0) {
        return SENSOR_TYPE_LIGHT;
    }
    else if (strcmp(type, "Bar") == 0) {
        return SENSOR_TYPE_PRESSURE;
    }
    else if (strcmp(type, "Tmp") == 0) {
        return SENSOR_TYPE_TEMPERATURE;
    }
    else if (strcmp(type, "Prx") == 0) {
        return SENSOR_TYPE_PROXIMITY;
    }
    else if (strcmp(type, "Grv") == 0) {
        return SENSOR_TYPE_GRAVITY;
    }
    else if (strcmp(type, "Lac") == 0) {
        return SENSOR_TYPE_LINEAR_ACCELERATION;
    }
    else if (strcmp(type, "Rot") == 0) {
        return SENSOR_TYPE_ROTATION_VECTOR;
    }
    else if (strcmp(type, "Hum") == 0) {
        return SENSOR_TYPE_RELATIVE_HUMIDITY;
    }
    else if (strcmp(type, "Tam") == 0) {
        return SENSOR_TYPE_AMBIENT_TEMPERATURE;
    }
    else if (isdigit(type[0])) {
        return atoi(type);
    }

    return -1;
}

static int skipThisSensor(int type, int* types, int numTypes) {
    for (int i=0; i<numTypes; i++) {
        if (type == types[i]) {
            return 0;
        }
    }

    return numTypes != 0;
}
// ---------------------------------------------------------------------------

int SensorControl::sensor_log = 0;

SensorControl::~SensorControl()
{
    SENSOR_LOG("%s\n",__func__);
    if (mStatus == NO_ERROR) {
        stop();
    }
    SENSOR_LOG("2end %s\n",__func__);
}

SensorControl::SensorControl(const String16 &opPackageName)
    :mPreviousPriority(ANDROID_PRIORITY_NORMAL),
    mOpPackageName(opPackageName),
    mStatus(-1)
{
    SENSOR_LOG("%s\n",__func__);
}

SensorControl::SensorControl(const String16 &opPackageName, int type,
    callback_t cbf, void* user)
    :mPreviousPriority(ANDROID_PRIORITY_NORMAL),
    mOpPackageName(opPackageName),
    mStatus(-1)
{
    SENSOR_LOG("%s\n",__func__);
    mStatus = set(type, cbf, user);
    if (mStatus && type == SENSOR_TYPE_VIRTUAL_CCT) {
        mStatus = set(SENSOR_TYPE_FRONT_CCT, cbf, user);
    }
}

SensorCallback::SensorCallback(callback_t cbf, void* user)
{
    SENSOR_LOG("%s\n",__func__);

    mCbf = cbf;
    mUserData = user;
}


Return<void> SensorCallback::onEvent(const Event &e)
{

    if (mCbf != 0)
    mCbf((int)e.sensorType, mUserData, e);
    return Void();
}

int SensorControl::set(int type, callback_t cbf, void* user)
{
    SENSOR_LOG("%s\n",__func__);
    sp<IEventQueue> queue;
    Result res = Result::BAD_VALUE;
    int i;

    mSmgr = ISensorManager::getService();
    if ((sp<ISensorManager>)nullptr == mSmgr) {
        ALOGE("ISensorManager manager is NULL.");
        return -1;
    }

    Return<void> ret = mSmgr->createEventQueue(new SensorCallback(cbf, user),
                    [&queue, &res](const auto &q, auto r)
                    {
                        queue = q; res = r;
                    });
    if (!ret.isOk()) {
            ALOGE("%s: Failed to create sensor queue", __func__);
            return -1;
    }

    mSensorEventQueue = queue;

    hardware::hidl_vec<::android::hardware::sensors::V1_0::SensorInfo> sensorHandleVec;

    ret = mSmgr->getSensorList(
        [&sensorHandleVec, &res](const hardware::hidl_vec<::android::hardware::sensors::V1_0::SensorInfo>& list, Result result)
        {
            sensorHandleVec = list;
            res = result;
        });
    if (!ret.isOk()) {
        ALOGE("%s: Failed to get sensor list", __func__);
        return -1;
    }

    int show_list = 0;
    //debug sensor
    if (show_list) {
        SENSOR_LOG("%d sensors found:\n", sensorHandleVec.size());
        for (i = 0 ; i < sensorHandleVec.size(); i++) {
            const ::android::hardware::sensors::V1_0::SensorInfo &zSensor = sensorHandleVec[i];

            SENSOR_LOG("%-32s| vendor: %-28s | handle: %10d | type: %5d\n",
              zSensor.name.c_str(),
              zSensor.vendor.c_str(),
              zSensor.sensorHandle,
              zSensor.type
              );
        }
    }

    for (i = sensorHandleVec.size() - 1; i >= 0; i--)
    {
        SensorInfo &zSensor = sensorHandleVec[i];
        if (type == (int)zSensor.type && !(zSensor.flags  & static_cast<uint32_t>(SensorFlagBits::WAKE_UP))) {
            mSensor = zSensor;
            SENSOR_LOG("i=%d,type=%d\n", i, zSensor.type);
            break;
        }
    }
    if (i < 0) {
        ALOGE("%s: NO sensor match", __func__);
        return -1;
    }
    mStatus = 0;
    mType = (int)mSensor.type;
    return 0;
}

int SensorControl::set(int type, callback_t cbf, void* user, bool wakeup)
{
    SENSOR_LOG("%s\n",__func__);
    sp<IEventQueue> queue;
    Result res = Result::BAD_VALUE;
    int i;
    unsigned int sensor_wakeup_flag = 0;

    mSmgr = ISensorManager::getService();
    if ((sp<ISensorManager>)nullptr == mSmgr) {
        ALOGE("ISensorManager manager is NULL.");
        return -1;
    }

    Return<void> ret = mSmgr->createEventQueue(new SensorCallback(cbf, user),
                    [&queue, &res](const auto &q, auto r)
                    {
                        queue = q; res = r;
                    });
    if (!ret.isOk()) {
            ALOGE("%s: Failed to create sensor queue", __func__);
            return -1;
    }

    mSensorEventQueue = queue;

    hardware::hidl_vec<::android::hardware::sensors::V1_0::SensorInfo> sensorHandleVec;

    ret = mSmgr->getSensorList(
        [&sensorHandleVec, &res](const hardware::hidl_vec<::android::hardware::sensors::V1_0::SensorInfo>& list, Result result)
        {
            sensorHandleVec = list;
            res = result;
        });
    if (!ret.isOk()) {
        ALOGE("%s: Failed to get sensor list", __func__);
        return -1;
    }

    int show_list = 0;
    //debug sensor
    if (show_list) {
        SENSOR_LOG("%d sensors found:\n", sensorHandleVec.size());
        for (i = 0 ; i < sensorHandleVec.size(); i++) {
            const ::android::hardware::sensors::V1_0::SensorInfo &zSensor = sensorHandleVec[i];

            SENSOR_LOG("%-32s| vendor: %-28s | handle: %10d | type: %5d\n",
              zSensor.name.c_str(),
              zSensor.vendor.c_str(),
              zSensor.sensorHandle,
              zSensor.type
              );
        }
    }

    for (i = sensorHandleVec.size() - 1; i >= 0; i--)
    {
        SensorInfo &zSensor = sensorHandleVec[i];
        sensor_wakeup_flag = (zSensor.flags  & static_cast<uint32_t>(SensorFlagBits::WAKE_UP));
        sensor_wakeup_flag = wakeup ? sensor_wakeup_flag : !sensor_wakeup_flag;
        if (type == (int)zSensor.type && sensor_wakeup_flag) {
            mSensor = zSensor;
            SENSOR_LOG("i=%d,type=%d\n", i, zSensor.type);
            break;
        }
    }
    if (i < 0) {
        ALOGE("%s: NO sensor match", __func__);
        return -1;
    }
    mStatus = 0;
    mType = (int)mSensor.type;

    return 0;
}

int SensorControl::start(int samplingPeriodUs, int maxBatchReportLatencyUs, bool wakeup)
{
    SENSOR_LOG("%s in",__func__);
    AutoMutex lock(mLock);
    if (mStatus != 0)
    {
        ALOGE("%s: sensor does not ready yet", __func__);
        return mStatus;
    }
    int status = 0;
    Return<Result> ret = mSensorEventQueue->enableSensor(mSensor.sensorHandle,
        samplingPeriodUs, maxBatchReportLatencyUs);

    if(!ret.isOk()) {
        SENSOR_LOG("enable sensor of type %d error", mType);
        status = -1;
    }

    SENSOR_LOG("%s out",__func__);
    return status;
}

int SensorControl::stop()
{
    SENSOR_LOG("%s in\n",__func__);
    AutoMutex lock(mLock);

    int status = 0;
    if (mStatus != 0)
    {
        ALOGE("%s: sensor does not ready yet", __func__);
        return mStatus;
    }

    Return<Result> ret = mSensorEventQueue->disableSensor(mSensor.sensorHandle);
    if(!ret.isOk()) {
        SENSOR_LOG("disableSensor() of type %d failed", mType);
        status = -1;
    }

    SENSOR_LOG("%s out\n",__func__);
    return status;
}
};
