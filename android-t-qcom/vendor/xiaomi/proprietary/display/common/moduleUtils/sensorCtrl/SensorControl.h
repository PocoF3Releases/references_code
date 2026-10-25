#ifndef __SENSOR_CONTROL_H__
#define __SENSOR_CONTROL_H__

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <android/sensor.h>
#include <cutils/sched_policy.h>
#include <binder/IServiceManager.h>

#include <android/frameworks/sensorservice/1.0/ISensorManager.h>
#include <android/frameworks/sensorservice/1.0/IEventQueue.h>
#include <android/frameworks/sensorservice/1.0/types.h>
#include <sensors-base.h>
namespace android {

using ::android::frameworks::sensorservice::V1_0::ISensorManager;
using ::android::frameworks::sensorservice::V1_0::IEventQueue;
using ::android::frameworks::sensorservice::V1_0::Result;
using ::android::frameworks::sensorservice::V1_0::IEventQueueCallback;
using ::android::hardware::sensors::V1_0::SensorInfo;
using ::android::hardware::sensors::V1_0::SensorType;
using ::android::hardware::sensors::V1_0::Event;
using ::android::hardware::sensors::V1_0::SensorFlagBits;
using ::android::hardware::Void;
using ::android::hardware::Return;
//using namespace android;

#define SENSOR_TYPE_LIGHT_MI    33171013 //workarround
#define SENSOR_TYPE_LIGHT_PRIMARY_SCREEN    5
#define SENSOR_TYPE_LIGHT_SECOND_SCREEN    33171081
#define SENSOR_TYPE_LIGHT_RGB   33171058 //RGB sensor.
#define SENSOR_TYPE_LIGHT_REAR  33171055 //rear light sensor
#define SENSOR_TYPE_FRONT_CCT   33171088 //cct sensor
#define SENSOR_TYPE_VIRTUAL_CCT 33171108 //virtual cct sensor
#define SENSOR_TYPE_FOLD_STATUS  33171087 //fold status
#define SNS_MAX_VENDOR_NAME_SIZE 80

#define SENSOR_LOG(fmt, args...) \
    do { \
        if (SensorControl::sensor_log == 1) \
            ALOGD(fmt, ##args); \
        else \
            ALOGV(fmt, ##args); \
    } while (0)

#define MAX_SUPPORT_SNUM 16

typedef struct ASensorInfo {
    ASensorEvent event;
    char vendor[SNS_MAX_VENDOR_NAME_SIZE];
} ASensorInfo;

class SensorControl :public RefBase {

public:
    //callback function for user get event.
    typedef void (*callback_t)(int type, void* user, const Event &event);
    // @brief Simple SensorCotrol's construct function .
    SensorControl(const String16 &opPackageName);
    // @brief Complex SensorControl's construct function,it may not use 'set' function.
    SensorControl(const String16 &opPackageName, int type, callback_t cbf, void* user);
  /*! @brief Set SensorCotrol's configure.
    @details Client shall use this method to configure own callback function by using SensorControl(const String16 &opPackageName) contruct SensorControl class.

    @param[in] type , it is sensor 's type.
    @param[in] cbf , it is  user's callback function.
    @param[in] user, it is user object.

    @return \link 0:success,-1: failure \endlink

   */
    int set(int type, callback_t cbf, void* user);
    /*! @brief Set SensorCotrol's configure.
    @details Client shall use this method to configure own callback function by using SensorControl(const String16 &opPackageName) contruct SensorControl class.

    @param[in] type , it is sensor 's type.
    @param[in] cbf , it is  user's callback function.
    @param[in] user, it is user object.
    @param[in] user, it is user object.

    @return \link 0:success,-1: failure \endlink

   */
    int set(int type, callback_t cbf, void* user, bool wakeup);
  /*! @brief Start SensorCotrol catpure sensor data.
    @details Client shall use this method to start to get sensor data.

    @param[in] samplingPeriodUs is get sensor data interval time.
    @param[in] maxBatchReportLatencyUs is max sensor report latency time.
    @param[in] wakeup is sensor run type ,wakeup equal false represent the sensor don't again call it.

    @return \link 0:success,-1: failure \endlink

   */
    int start(int samplingPeriodUs, int maxBatchReportLatencyUs, bool wakeup);
  /*! @brief Stop SensorCotrol catpure sensor data.
    @details it will stop to get sensor data and wait capture thread stop.

    @param[in] No

    @return \link 0:success,-1: failure \endlink

   */
    int stop();

    ~SensorControl();

    static int sensor_log;
protected:

    int                     mPreviousPriority;
    SchedPolicy             mPreviousSchedulingGroup;

private:

    String16                mOpPackageName;
    int                     mStatus;
    sp<ISensorManager>      mSmgr;
    sp<IEventQueue>         mSensorEventQueue;
    SensorInfo              mSensor;
    int                     mNeedWakeUp;
    int                     mType;
    mutable Mutex           mLock;
};

class SensorCallback : public IEventQueueCallback
{
public:
    typedef void (*callback_t)(int type, void* user, const Event &event);

    SensorCallback(callback_t cbf, void* user);
    Return<void> onEvent(const Event &e);

private:
    callback_t              mCbf;
    void*                   mUserData;
};
};
#endif  // __SENSOR_CONTROL_H__
