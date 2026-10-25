#ifndef _PERF_LOG_ANALYZER_H_
#define _PERF_LOG_ANALYZER_H_

#include "log_analyzer.h"
#include "plugin.h"
#include <string>
#include <mutex>
#include <utils/String8.h>
#include <utils/threads.h>
#include <json/json.h>
#include "ringbuffer.h"
#include <android/hardware/thermal/2.0/IThermal.h>
#include <android/hardware/thermal/2.0/IThermalChangedCallback.h>
#include <android/hardware/thermal/2.0/types.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/manager/1.0/IServiceNotification.h>

using ::android::sp;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::hardware::thermal::V1_0::ThermalStatus;
using ::android::hardware::thermal::V1_0::ThermalStatusCode;
using ::android::hardware::thermal::V2_0::CoolingDevice;
using ::android::hardware::thermal::V2_0::CoolingType;
using ::android::hardware::thermal::V2_0::IThermal;
using ::android::hardware::thermal::V2_0::Temperature;
using ::android::hardware::thermal::V2_0::ThrottlingSeverity;

namespace android {
namespace MiSight {
class PerfLogAnalyzer : public Plugin {
public:
    // MiSight
    std::string TimeFormat = "%Y-%m-%d %H:%M:%S";
    EVENT_RET OnEvent(sp<Event> &event);
    bool CanProcessEvent(sp<Event> event);
    void OnLoad();
    void OnUnload();
    void Dump(int fd, const std::vector<std::string>& cmds);
    virtual ~PerfLogAnalyzer();
private:
    RingBuffer<std::string, 100> mDumpErrEvents;

    RingBuffer<Json::Value, 100> mLongMsgs;
    RingBuffer<Json::Value, 100> mLongBinders;
    RingBuffer<Json::Value, 100> mLongLocks;

    void analyze(Json::Value exceptionLog);
    ::android::sp<IThermal> mThermal; // Thermal ServiceNotification listener
    void saveCommonMsgInfoToDump(Json::Value longMsg, int eventId);
    void saveCommonMsgBinderInfoToDump(Json::Value longMsg, Json::Value longBinder, int eventId);
    void analyzeLongMsg(Json::Value longMsg);
    void saveLongMsgInfo(Json::Value exceptionLog);
    void analyzeDoFrameLate(Json::Value exceptionLog);
    void analyzeActivityLifeCycleLate(Json::Value exceptionLog);
    void analyzeLongTraversals(Json::Value exceptionLog);
    void analyzeLongDoFrame(Json::Value exceptionLog);
    void analyzeSlowInputInServer(Json::Value exceptionLog);
    void analyzeSlowInputBeforeApp(Json::Value exceptionLog);
    void analyzeSlowInputInApp(Json::Value exceptionLog);
    void analyzeSurfaceFlingerSlowMethod(Json::Value exceptionLog);
    void analyzeSlowCommonMethod(Json::Value exceptionLog);
    void analyzeSlowProviderOnCreate(Json::Value exceptionLog);
    void analyzeSlowBroadcastOnReceive(Json::Value exceptionLog);
    void analyzeSlowServiceOnCreate(Json::Value exceptionLog);
    bool isLongMsgSystemJank(Json::Value longMsg);
    bool isLongMsgBinderJank(Json::Value longMsg);
    bool isLongMsgBinderLockJank(Json::Value longMsg, Json::Value longBinder);
    bool isLongMsgLockJank(Json::Value longMsg);
    bool isLongMsgCPURunning(Json::Value longMsg);
    void sendLongMsgBinderEventToPipline(Json::Value longMsg, Json::Value longBinder, int eventId);
    void sendLongBinderTargetEventToPipline(Json::Value longMsg, Json::Value longBinder, int eventId);
    void sendLongRunnableBinderEventToPipline(Json::Value longMsg, Json::Value longBinder, int eventId);
    void sendLongLockMsgBinderEventToPipline(Json::Value longLock,Json::Value longMsg, Json::Value longBinder, int eventId);
    void sendLongLockMsgEventToPipline(Json::Value longLock, Json::Value longMsg, int eventId);
    void sendParaClassToPipline(Json::Value errInfo, int eventId);
    void sendEventToPipline(Json::Value errInfo, int eventId);
    void saveComInfoToDump(Json::Value exceptionLog, int eventId);
    void saveLongLockMsgInfoToDump(Json::Value longLock, Json::Value longMsg, int eventId);
    void saveLongLockBinderInfoToDump(Json::Value longLock, Json::Value longMsg, Json::Value longBinder, int eventId);
};
} //namespace android
}
#endif /*_PERF_LOG_ANALYZER_H_*/
