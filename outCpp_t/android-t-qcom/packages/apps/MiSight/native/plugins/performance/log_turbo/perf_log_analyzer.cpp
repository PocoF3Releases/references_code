#define LOG_TAG "EXCEPTION_LOG"

#include "perf_log_analyzer.h"
#include <stdio.h>
#include <inttypes.h>
#include "plugin_factory.h"
#include "json_util.h"
#include "event_util.h"
#include "log.h"
#include "logturbo_event_utils.h"

#define LONG_MSG_RATIO 2

namespace android {
namespace MiSight {
constexpr int ERR_INFO_BUF = 1024;
PerfLogAnalyzer::~PerfLogAnalyzer() {
    mLongMsgs.clear();
    mLongBinders.clear();
    mLongLocks.clear();
    mDumpErrEvents.clear();
}

//check why this msg is long
void PerfLogAnalyzer::analyzeLongMsg(Json::Value longMsg)
{
    if (LONG_MSG_RATIO * longMsg[PERF_EVENT_KEY_RUNNING_TIME].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
        isLongMsgCPURunning(longMsg);
    } else {
        if(isLongMsgSystemJank(longMsg)) {
            return;
        }
        if(!isLongMsgLockJank(longMsg)) {
            isLongMsgBinderJank(longMsg);
        }
        else {
            saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_COMMON_METHOD);
        }
    }
}

bool PerfLogAnalyzer::isLongMsgBinderLockJank(Json::Value longMsg, Json::Value longBinder)
{
    for (int i = mLongLocks.size() - 1; i > 0; i--) {
        Json::Value longLock = mLongLocks[i];
        if (longLock[PERF_EVENT_KEY_DISPATCH_TIME].asInt64()  < longBinder[PERF_EVENT_KEY_DISPATCH_TIME].asInt64()
            || (longLock[PERF_EVENT_KEY_DISPATCH_TIME].asInt64() > longBinder[PERF_EVENT_KEY_DISPATCH_TIME].asInt64() + longBinder[PERF_EVENT_KEY_DURATION].asInt64())) {
            continue;
        }
        if (longLock[PERF_EVENT_KEY_PID].asInt() == longBinder[PERF_EVENT_KEY_BINDER_TARGET_TID].asInt()
            && LONG_MSG_RATIO * longLock[PERF_EVENT_KEY_LATENCY].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
            MISIGHT_LOGE("Err in (%d)%s, reasonId=%d", longLock[PERF_EVENT_KEY_PID].asInt(), longLock[PERF_EVENT_KEY_PROCESSNAME].asString().c_str(),
                    PERF_EVENT_ID_LONG_MSG_BINDER_LOCK);
            sendLongLockMsgBinderEventToPipline(longLock, longMsg, longBinder, PERF_EVENT_ID_LONG_MSG_BINDER_LOCK);
            saveLongLockBinderInfoToDump(longLock, longMsg, longBinder, PERF_EVENT_ID_LONG_MSG_BINDER_LOCK);
            return true;
        }
    }
    return false;
}

bool PerfLogAnalyzer::isLongMsgLockJank(Json::Value longMsg)
{
    //step two: check msg lock
    for (int i = mLongLocks.size(); i > 0; i--) {
        Json::Value longLock = mLongLocks[i];
        if (!(longLock[PERF_EVENT_KEY_DISPATCH_TIME].asInt64() >= longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() + longMsg[PERF_EVENT_KEY_LATENCY].asInt64()
            && longLock[PERF_EVENT_KEY_DISPATCH_TIME].asInt64() <= longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() + longMsg[PERF_EVENT_KEY_LATENCY].asInt64() + longMsg[PERF_EVENT_KEY_WALLTIME].asInt64())) {
            continue;
        }
        if (longLock[PERF_EVENT_KEY_PID].asInt() == longMsg[PERF_EVENT_KEY_PID].asInt()
            && LONG_MSG_RATIO * longLock[PERF_EVENT_KEY_LATENCY].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
            MISIGHT_LOGE("Err in (%d)%s, reasonId=%d", longLock[PERF_EVENT_KEY_PID].asInt(), longLock[PERF_EVENT_KEY_PROCESSNAME].asString().c_str(), PERF_EVENT_ID_APP_LONG_MSG_LOCK);
            sendLongLockMsgEventToPipline(longLock, longMsg, PERF_EVENT_ID_APP_LONG_MSG_LOCK);
            saveLongLockMsgInfoToDump(longLock, longMsg, PERF_EVENT_ID_APP_LONG_MSG_LOCK);
            return true;
        }
    }
    return false;
}

bool PerfLogAnalyzer::isLongMsgBinderJank(Json::Value longMsg)
{
    for (int i = mLongBinders.size() - 1; i > 0; i--) {
        Json::Value longBinder = mLongBinders[i];
        if (!(longBinder[PERF_EVENT_KEY_DISPATCH_TIME].asInt64() >= longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() + longMsg[PERF_EVENT_KEY_LATENCY].asInt64()
            && longBinder[PERF_EVENT_KEY_DISPATCH_TIME].asInt64() + longBinder[PERF_EVENT_KEY_DURATION].asInt64() <=
            longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() + longMsg[PERF_EVENT_KEY_LATENCY].asInt64() + longMsg[PERF_EVENT_KEY_WALLTIME].asInt64())) {
            continue;
        }

        if (LONG_MSG_RATIO * longBinder[PERF_EVENT_KEY_DURATION].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()
            && longBinder[PERF_EVENT_KEY_PID].asInt() == longMsg[PERF_EVENT_KEY_PID].asInt()) {
            if (LONG_MSG_RATIO * longBinder[PERF_EVENT_KEY_BINDER_TARGET_RUNNING].asInt64()> longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
                if (LONG_MSG_RATIO *longBinder[PERF_EVENT_KEY_BINDER_TARGET_CPU_UTIME].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
                    MISIGHT_LOGE("Err in (%d)%s, reasonId=%d", longBinder[PERF_EVENT_KEY_BINDER_TARGET_PID].asInt(),
                        longBinder[PERF_EVENT_KEY_PACKAGE].asString().c_str(), PERF_EVENT_ID_LONG_MSG_BINDER_RUNNING);
                    saveCommonMsgBinderInfoToDump(longMsg, longBinder, PERF_EVENT_ID_LONG_MSG_BINDER_RUNNING);
                    sendLongBinderTargetEventToPipline(longMsg, longBinder, PERF_EVENT_ID_LONG_MSG_BINDER_RUNNING);
                    return true;
                } else if (LONG_MSG_RATIO * longBinder[PERF_EVENT_KEY_BINDER_TARGET_CPU_STIME].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
                    if (LONG_MSG_RATIO *longBinder[PERF_EVENT_KEY_BINDER_TARGET_FREEPAGES].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
                        MISIGHT_LOGE("Err in system performance, reasonId=%d", PERF_EVENT_ID_SYSTEM_LONG_MSG_RECLAIM_BINDER);
                        sendLongMsgBinderEventToPipline(longMsg, longBinder, PERF_EVENT_ID_SYSTEM_LONG_MSG_RECLAIM_BINDER);
                        saveCommonMsgBinderInfoToDump(longMsg, longBinder, PERF_EVENT_ID_SYSTEM_LONG_MSG_RECLAIM_BINDER);
                        return true;
                    } else {
                        saveCommonMsgBinderInfoToDump(longMsg, longBinder, PERF_EVENT_ID_COMMON_METHOD);
                    }
                } else {
                    saveCommonMsgBinderInfoToDump(longMsg, longBinder, PERF_EVENT_ID_COMMON_METHOD);
                }
            } else if (LONG_MSG_RATIO * longBinder[PERF_EVENT_KEY_BINDER_TARGET_RUNNABLE].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
                MISIGHT_LOGE("Err in system performance, reasonId=%d", PERF_EVENT_ID_SYSTEM_LONG_MSG_RUNNABLE_BINDER);
                sendLongRunnableBinderEventToPipline(longMsg, longBinder, PERF_EVENT_ID_SYSTEM_LONG_MSG_RUNNABLE_BINDER);
                saveCommonMsgBinderInfoToDump(longMsg, longBinder, PERF_EVENT_ID_SYSTEM_LONG_MSG_RUNNABLE_BINDER);
                return true;
            } else if (LONG_MSG_RATIO * longBinder[PERF_EVENT_KEY_BINDER_TARGET_BLKIO].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
                MISIGHT_LOGE("Err in system performance, reasonId=%d", PERF_EVENT_ID_SYSTEM_LONG_MSG_BLKIO_BINDER);
                sendLongBinderTargetEventToPipline(longMsg, longBinder, PERF_EVENT_ID_SYSTEM_LONG_MSG_BLKIO_BINDER);
                saveCommonMsgBinderInfoToDump(longMsg, longBinder, PERF_EVENT_ID_SYSTEM_LONG_MSG_BLKIO_BINDER);
                return true;
            } else if (LONG_MSG_RATIO * longBinder[PERF_EVENT_KEY_BINDER_TARGET_SWAPIN].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
                MISIGHT_LOGE("Err in system performance, reasonId=%d", PERF_EVENT_ID_SYSTEM_LONG_MSG_SWAPIN_BINDER);
                sendLongMsgBinderEventToPipline(longMsg, longBinder, PERF_EVENT_ID_SYSTEM_LONG_MSG_SWAPIN_BINDER);
                saveCommonMsgBinderInfoToDump(longMsg, longBinder, PERF_EVENT_ID_SYSTEM_LONG_MSG_SWAPIN_BINDER);
                return true;
            }
            if(!isLongMsgBinderLockJank(longMsg, longBinder)) {
                saveCommonMsgBinderInfoToDump(longMsg, longBinder, PERF_EVENT_ID_COMMON_METHOD);
            }
            return true;
        }
    }
    return false;
}

bool PerfLogAnalyzer::isLongMsgSystemJank(Json::Value longMsg)
{
    if (LONG_MSG_RATIO * longMsg[PERF_EVENT_KEY_RUNNABLE_TIME].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
        MISIGHT_LOGE("Err in system performance, reasonId=%d", PERF_EVENT_ID_SYSTEM_LONG_MSG_RUNNABLE);
        sendParaClassToPipline(longMsg, PERF_EVENT_ID_SYSTEM_LONG_MSG_RUNNABLE);
        saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_SYSTEM_LONG_MSG_RUNNABLE);
        return true;
    } else if (LONG_MSG_RATIO * longMsg[PERF_EVENT_KEY_IO_TIME].asInt64()> longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
        MISIGHT_LOGE("Err in system performance, reasonId=%d", PERF_EVENT_ID_SYSTEM_LONG_MSG_BLKIO);
        sendParaClassToPipline(longMsg, PERF_EVENT_ID_SYSTEM_LONG_MSG_BLKIO);
        saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_SYSTEM_LONG_MSG_BLKIO);
        return true;
    } else if (LONG_MSG_RATIO * longMsg[PERF_EVENT_KEY_SWAPIN_TIME].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
        MISIGHT_LOGE("Err in system performance, reasonId=%d",PERF_EVENT_ID_SYSTEM_LONG_MSG_SWAPIN);
        sendParaClassToPipline(longMsg, PERF_EVENT_ID_SYSTEM_LONG_MSG_SWAPIN);
        saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_SYSTEM_LONG_MSG_SWAPIN);
        return true;
    }
    return false;
}

bool PerfLogAnalyzer::isLongMsgCPURunning(Json::Value longMsg)
{
    if (longMsg[PERF_EVENT_KEY_LIMIT_BY_THERMAL] == true) {
        MISIGHT_LOGE("Err in Thermal, reasonId=%d", PERF_EVENT_ID_THERMAL_LONG_MSG);
        saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_THERMAL_LONG_MSG);
        sendParaClassToPipline(longMsg, PERF_EVENT_ID_THERMAL_LONG_MSG);
        return true;
    } else if (LONG_MSG_RATIO * longMsg[PERF_EVENT_KEY_CPU_UTIME].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
        MISIGHT_LOGE("Err in (%d)%s/%d, reasonId=%d", longMsg[PERF_EVENT_KEY_PID].asInt(), longMsg[PERF_EVENT_KEY_PACKAGE].asString().c_str(), longMsg[PERF_EVENT_KEY_UID].asInt(),
                PERF_EVENT_ID_APP_LONG_MSG_RUNNING);
        saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_APP_LONG_MSG_RUNNING);
        sendParaClassToPipline(longMsg, PERF_EVENT_ID_APP_LONG_MSG_RUNNING);
        return true;
    } else if (LONG_MSG_RATIO * longMsg[PERF_EVENT_KEY_CPU_STIME].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
        //When app reclaim memory , cpu task state will be TASK_RUNNING.
        //Let system performance to check this issue.
        if (LONG_MSG_RATIO * longMsg[PERF_EVENT_KEY_RECLAIM_TIME].asInt64() > longMsg[PERF_EVENT_KEY_WALLTIME].asInt64()) {
            MISIGHT_LOGE("Err in system performance, reasonId=%d", PERF_EVENT_ID_SYSTEM_LONG_MSG_RECLAIM);
            saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_SYSTEM_LONG_MSG_RECLAIM);
            sendParaClassToPipline(longMsg, PERF_EVENT_ID_SYSTEM_LONG_MSG_RECLAIM);
            return true;
        } else {
            //There is some system reason we have not added yet.
            saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_COMMON_METHOD);
        }
    } else {
        saveCommonMsgInfoToDump(longMsg, PERF_EVENT_ID_COMMON_METHOD);
    }
    return false;
}

void PerfLogAnalyzer::saveLongMsgInfo(Json::Value exceptionLog)
{
    Json::Value msg = exceptionLog;
    if (mThermal != NULL) {
      //Just get cpu cooling device now.
        hidl_vec<CoolingDevice> list;
        mThermal->getCurrentCoolingDevices(
                true, CoolingType::CPU, [&list](ThermalStatus status, hidl_vec<CoolingDevice> cooling_devices) {
                    if (ThermalStatusCode::SUCCESS == status.code && cooling_devices.size() > 0) {
                        list = std::move(cooling_devices);
                    }
                });
        for (size_t i = 0; i < list.size(); i++) {
            if (list[i].value >= (int)ThrottlingSeverity::SEVERE) {
                msg[PERF_EVENT_KEY_LIMIT_BY_THERMAL] = true;
            }
        }
    }
    mLongMsgs.next() = msg;
}

void PerfLogAnalyzer::analyzeDoFrameLate(Json::Value exceptionLog)
{
    int64_t longMsgAllTime = 0;
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    int64_t planTimeEr = exceptionLog[PERF_EVENT_KEY_PLANTIME_ER].asInt64();

    for (int i = mLongMsgs.size() - 1; i > 0; i--) {
        Json::Value longMsg = mLongMsgs[i];
        if (longMsg[PERF_EVENT_KEY_PID] == pid) {
            if ((longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() + longMsg[PERF_EVENT_KEY_LATENCY].asInt64() + longMsg[PERF_EVENT_KEY_WALLTIME].asInt64() < planTimeEr)
                || (longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() > exceptionLog[PERF_EVENT_KEY_PLANTIME_ER].asInt64())) {
                continue;
            }
            longMsgAllTime +=  longMsg[PERF_EVENT_KEY_WALLTIME].asInt64();
            analyzeLongMsg(longMsg);
        }
    }
    if (longMsgAllTime > 0) {
        std::string packageName = exceptionLog[PERF_EVENT_KEY_PACKAGE].asString();
        int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
        MISIGHT_LOGE("Err in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_BUSY_MAIN);
        sendParaClassToPipline(exceptionLog, PERF_EVENT_ID_BUSY_MAIN);
        saveComInfoToDump(exceptionLog, PERF_EVENT_ID_BUSY_MAIN);
    }
}

void PerfLogAnalyzer::analyzeActivityLifeCycleLate(Json::Value exceptionLog)
{
    int64_t longMsgAllTime = 0;
    int64_t planTimeEr = exceptionLog[PERF_EVENT_KEY_PLANTIME_ER].asInt64();
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    for (int i = mLongMsgs.size() - 1; i > 0; i--) {
        Json::Value longMsg = mLongMsgs[i];
        if (longMsg[PERF_EVENT_KEY_PID] == pid) {
            if (longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() + longMsg[PERF_EVENT_KEY_LATENCY].asInt64() + longMsg[PERF_EVENT_KEY_WALLTIME].asInt64() < planTimeEr
                || longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() > planTimeEr) {
                continue;
            }
            longMsgAllTime +=  longMsg[PERF_EVENT_KEY_WALLTIME].asInt64();
            analyzeLongMsg(longMsg);
        }
    }
    if (longMsgAllTime > 0) {
        std::string packageName = exceptionLog[PERF_EVENT_KEY_PACKAGE].asString();
        int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
        MISIGHT_LOGE("Err in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_ACTIVITY_LATE);
        sendParaClassToPipline(exceptionLog, PERF_EVENT_ID_ACTIVITY_LATE);
        saveComInfoToDump(exceptionLog, PERF_EVENT_ID_ACTIVITY_LATE);
    }
}

void PerfLogAnalyzer::analyzeLongTraversals(Json::Value exceptionLog)
{
    int64_t eventTimeTmp = exceptionLog[PERF_EVENT_KEY_EVENT_TIME].asInt64();
    std::string eventTime = TimeUtil::GetTimeDateStr(eventTimeTmp/1000, TimeFormat);
    std::string packageName = exceptionLog[PERF_EVENT_KEY_PACKAGE].asString();
    int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    MISIGHT_LOGE("Err in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_TRAVERSALS);
    sendEventToPipline(exceptionLog, PERF_EVENT_ID_TRAVERSALS);
    saveComInfoToDump(exceptionLog, PERF_EVENT_ID_TRAVERSALS);
}

void PerfLogAnalyzer::analyzeLongDoFrame(Json::Value exceptionLog)
{
    int64_t eventTimeTmp = exceptionLog[PERF_EVENT_KEY_EVENT_TIME].asInt64();
    std::string eventTime = TimeUtil::GetTimeDateStr(eventTimeTmp/1000, TimeFormat);
    std::string packageName = exceptionLog[PERF_EVENT_KEY_PACKAGE].asString();
    int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    MISIGHT_LOGE("Err in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_DO_FRAME);
    sendEventToPipline(exceptionLog, PERF_EVENT_ID_DO_FRAME);
    saveComInfoToDump(exceptionLog, PERF_EVENT_ID_DO_FRAME);
}

void PerfLogAnalyzer::analyzeSlowInputInServer(Json::Value exceptionLog)
{
    int64_t eventTimeTmp = exceptionLog[PERF_EVENT_KEY_EVENT_TIME].asInt64();
    std::string eventTime = TimeUtil::GetTimeDateStr(eventTimeTmp/1000, TimeFormat);

    MISIGHT_LOGE("Err in InputDispatcher, reasonId=%d", PERF_EVENT_ID_INPUT_SLOW_SERVER);
    sendEventToPipline(exceptionLog, PERF_EVENT_ID_INPUT_SLOW_SERVER);
    saveComInfoToDump(exceptionLog, PERF_EVENT_ID_INPUT_SLOW_SERVER);
}

void PerfLogAnalyzer::analyzeSlowInputBeforeApp(Json::Value exceptionLog)
{
    //Input plan time is from event time , time base SystemClock,updateMillis
    int64_t longMsgAllTime = 0;
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    int64_t disaptchTime = exceptionLog[PERF_EVENT_KEY_DISPATCH_TIME].asInt64();

    for (int i = mLongMsgs.size() - 1; i > 0; i--) {
        Json::Value longMsg = mLongMsgs[i];
        if (longMsg[PERF_EVENT_KEY_PID] == pid) {
            if (longMsg[PERF_EVENT_KEY_PLANTIME_UP].asInt64() + longMsg[PERF_EVENT_KEY_LATENCY].asInt64() + longMsg[PERF_EVENT_KEY_WALLTIME].asInt64() < disaptchTime
                    || longMsg[PERF_EVENT_KEY_PLANTIME_UP].asInt64() > disaptchTime) {
                continue;
            }
            longMsgAllTime +=  longMsg[PERF_EVENT_KEY_WALLTIME].asInt64();
            analyzeLongMsg(longMsg);
        }
    }

    //This period contains Inputdispatcher, but Inputdispatcher will output in
    //PERF_EVENT_ID_INPUT_SLOW_SERVER
    if (longMsgAllTime > 0) {
        std::string packageName = exceptionLog[PERF_EVENT_KEY_PACKAGE].asString();
        int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
        MISIGHT_LOGE("Err in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_INPUT_SLOW_BEFORE_APP);
        sendEventToPipline(exceptionLog, PERF_EVENT_ID_INPUT_SLOW_BEFORE_APP);
        saveComInfoToDump(exceptionLog, PERF_EVENT_ID_INPUT_SLOW_BEFORE_APP);
    }
}

void PerfLogAnalyzer::analyzeSlowInputInApp(Json::Value exceptionLog)
{
    std::string packageName = exceptionLog[PERF_EVENT_KEY_PACKAGE].asString();
    int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    MISIGHT_LOGE("Err in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_INPUT_SLOW_IN_APP);
    sendEventToPipline(exceptionLog, PERF_EVENT_ID_INPUT_SLOW_IN_APP);
    saveComInfoToDump(exceptionLog, PERF_EVENT_ID_INPUT_SLOW_IN_APP);
}

//analyze slow method in SurfaceFlinger
void PerfLogAnalyzer::analyzeSurfaceFlingerSlowMethod(Json::Value exceptionLog)
{
    MISIGHT_LOGE("Err in SurfaceFlinger, reasonId=%d", PERF_EVENT_ID_SF_SLOW_METHOD);
    sendEventToPipline(exceptionLog, PERF_EVENT_ID_SF_SLOW_METHOD );
    saveComInfoToDump(exceptionLog, PERF_EVENT_ID_SF_SLOW_METHOD);
}

void PerfLogAnalyzer::analyzeSlowCommonMethod(Json::Value exceptionLog)
{
    std::string packageName = exceptionLog[PERF_EVENT_KEY_PACKAGE].asString();
    int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    MISIGHT_LOGW("Warning in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_COMMON_METHOD);
    sendEventToPipline(exceptionLog, PERF_EVENT_ID_COMMON_METHOD);
    saveComInfoToDump(exceptionLog, PERF_EVENT_ID_COMMON_METHOD);
}

void PerfLogAnalyzer::analyzeSlowProviderOnCreate(Json::Value exceptionLog)
{
    std::string class_name = exceptionLog[PERF_EVENT_KEY_CLASSNAME].asString();
    int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    MISIGHT_LOGW("Warning in (%d)%s/%d, reasonId=%d", pid, class_name.c_str(), uid, PERF_EVENT_ID_PROVIDER_SLOW_CREATE);
    sendEventToPipline(exceptionLog, PERF_EVENT_ID_PROVIDER_SLOW_CREATE);
    saveComInfoToDump(exceptionLog, PERF_EVENT_ID_PROVIDER_SLOW_CREATE);
}

void PerfLogAnalyzer::analyzeSlowBroadcastOnReceive(Json::Value exceptionLog)
{
    int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    MISIGHT_LOGW("Warning in (%d)%s/%d, reasonId=%d", pid, exceptionLog[PERF_EVENT_KEY_BROADCAST].asString().c_str(), uid, PERF_EVENT_ID_BROADCAST_SLOW_CREATE);
    sendEventToPipline(exceptionLog, PERF_EVENT_ID_BROADCAST_SLOW_CREATE);
    saveComInfoToDump(exceptionLog, PERF_EVENT_ID_BROADCAST_SLOW_CREATE);
}

void PerfLogAnalyzer::analyzeSlowServiceOnCreate(Json::Value exceptionLog)
{
    int64_t eventTimeTmp = exceptionLog[PERF_EVENT_KEY_EVENT_TIME].asInt64();
    std::string eventTime = TimeUtil::GetTimeDateStr(eventTimeTmp/1000, TimeFormat);
    int uid = exceptionLog[PERF_EVENT_KEY_UID].asInt();
    int pid = exceptionLog[PERF_EVENT_KEY_PID].asInt();
    int64_t disaptchTime = exceptionLog[PERF_EVENT_KEY_DISPATCH_TIME].asInt64();
    int64_t longMsgAllTime = 0;

    for (int i = mLongMsgs.size() - 1; i > 0; i--) {
        Json::Value longMsg = mLongMsgs[i];
        if (longMsg[PERF_EVENT_KEY_PID] == pid) {
            if (longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() + longMsg[PERF_EVENT_KEY_LATENCY].asInt64() + longMsg[PERF_EVENT_KEY_WALLTIME].asInt64() < disaptchTime
                    || longMsg[PERF_EVENT_KEY_PLANTIME_ER].asInt64() > exceptionLog[PERF_EVENT_KEY_DISPATCH_TIME].asInt64()) {
                continue;
            }
            longMsgAllTime +=  longMsg[PERF_EVENT_KEY_WALLTIME].asInt64();
            analyzeLongMsg(longMsg);
        }
    }
    std::string packageName = exceptionLog[PERF_EVENT_KEY_PACKAGE].asString();
    int64_t monitorTime = exceptionLog[PERF_EVENT_KEY_MONITOR_TIME].asInt64();

    if (LONG_MSG_RATIO * longMsgAllTime < monitorTime) {
        MISIGHT_LOGW("Warning in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_SERVICE_SLOW_CREATE);
        sendEventToPipline(exceptionLog, PERF_EVENT_ID_SERVICE_SLOW_CREATE);
        saveComInfoToDump(exceptionLog, PERF_EVENT_ID_SERVICE_SLOW_CREATE);
    } else {
        MISIGHT_LOGW("Warning in (%d)%s/%d, reasonId=%d", pid, packageName.c_str(), uid, PERF_EVENT_ID_APP_SERVICE_CREATE_LATE);
        sendEventToPipline(exceptionLog, PERF_EVENT_ID_APP_SERVICE_CREATE_LATE);
        saveComInfoToDump(exceptionLog, PERF_EVENT_ID_APP_SERVICE_CREATE_LATE);
    }
}

void PerfLogAnalyzer::analyze(Json::Value exceptionLog)
{
    int eventID = exceptionLog["EventID"].asInt();
    switch (eventID) {
    case PERF_EVENT_FIRST_ID_LONG_MSG: {
        saveLongMsgInfo(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_BUSY_MAIN: {
        analyzeDoFrameLate(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_ACTIVITY_LATE: {
        analyzeActivityLifeCycleLate(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_TRAVERSALS: {
        analyzeLongTraversals(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_DO_FRAME: {
        analyzeLongDoFrame(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_PROVIDER_SLOW_CREATE: {
        analyzeSlowProviderOnCreate(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_BROADCAST_SLOW_CREATE: {
        analyzeSlowBroadcastOnReceive(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_SERVICE_SLOW_CREATE: {
        analyzeSlowServiceOnCreate(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_INPUT_SLOW_SERVER: {
        analyzeSlowInputInServer(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_INPUT_SLOW_BEFORE_APP: {
        analyzeSlowInputBeforeApp(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_INPUT_SLOW_IN_APP: {
        analyzeSlowInputInApp(exceptionLog);
        break;
    }
    case PERF_EVENT_ID_SF_SLOW_METHOD: {
        analyzeSurfaceFlingerSlowMethod(exceptionLog);
        break;
    }
    case PERF_EVENT_FIRST_ID_LONG_BINDER: {
        Json::Value longBinder = exceptionLog;
        mLongBinders.next() = longBinder;
        break;
    }
    case PERF_EVENT_FIRST_ID_LONG_LOCK: {
        Json::Value longLock = exceptionLog;
        mLongLocks.next() = longLock;
        break;
    }
    case PERF_EVENT_ID_COMMON_METHOD: {
        analyzeSlowCommonMethod(exceptionLog);
        break;
    }
    default: {
        std::string unIdentifyLog = JsonUtil::ConvertJsonToStr(exceptionLog);
        MISIGHT_LOGE("UnIdentified log: eventID %d, info  %s", eventID, unIdentifyLog.c_str());
        break;
    }
    }
}

void PerfLogAnalyzer::Dump(int fd, const std::vector<std::string>& cmds)
{
    String8 result;
    for (int i = mDumpErrEvents.size() - 1; i > 0; i--) {
        std::string& errEvent = mDumpErrEvents[i];
        result.appendFormat("%s", errEvent.c_str());
        result.appendFormat("\n");
        write(fd, result.string(), result.size());
    }
}

REGISTER_PLUGIN(PerfLogAnalyzer);
bool PerfLogAnalyzer::CanProcessEvent(sp<Event> event)
{
    if(event->eventId_ == PERF_EVENT_ID_LOGTURBO_SOURCE) {
        return true;
    }
    return false;
}

void PerfLogAnalyzer::OnLoad()
{
    // set plugin info
    SetVersion("PerfLogAnalyzer-1.0");
    SetName("PerfLogAnalyzer");
}

EVENT_RET PerfLogAnalyzer::OnEvent(sp<Event> &event)
{
    //annalyse
    Json::Value exceptionLog;
    std::string  eventbundle = event->GetValue(PERF_DEV_SAVE_EVENT);
    JsonUtil::ConvertStrToJson(eventbundle, exceptionLog);
    analyze(exceptionLog);
    return ON_SUCCESS;
}

void PerfLogAnalyzer:: sendEventToPipline(Json::Value errInfo, int eventId)
{
    std::string exceptionLog =  JsonUtil::ConvertJsonToStr(errInfo);
    sp<Event> event = new Event("PerformanceLog");
    event->eventId_ =  eventId;
    event->happenTime_ = std::time(nullptr);
    event->messageType_ = Event::MessageType::FAULT_EVENT;
    event->SetValue(EventUtil::DEV_SAVE_EVENT, exceptionLog);
    GetPluginContext()->InsertEventToPipeline("dftFaultEvent", event);
}

void PerfLogAnalyzer:: sendParaClassToPipline(Json::Value errInfo, int eventId)
{
    Json::Value root;
    root["ParaClass"] = errInfo;
    sendEventToPipline(root, eventId);
}

void PerfLogAnalyzer::sendLongMsgBinderEventToPipline(Json::Value longMsg, Json::Value longBinder, int eventId)
{
    Json::Value root, ParaRoot; //902001250
    root["EventTime"] = TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str();
    root["PackageName"] =longMsg[PERF_EVENT_KEY_PACKAGE];
    root["PID"] =  longMsg[PERF_EVENT_KEY_PID];
    root["MsgWallTime"] =  longMsg[PERF_EVENT_KEY_WALLTIME];
    root["SystenBinderCostTime"] = longBinder[PERF_EVENT_KEY_BINDER_TARGET_SWAPIN];
    root["Interface"] = longBinder[PERF_EVENT_KEY_BINDER_INTERFACE];
    root["Code"] = longBinder[PERF_EVENT_KEY_DURATION];
    root["MsgPlanTime"] =  longMsg[PERF_EVENT_KEY_PLANTIME_CURR];
    root["Latency"] =  longMsg[PERF_EVENT_KEY_LATENCY];
    root["MsgSeq"] =  longMsg[PERF_EVENT_KEY_SEQ];
    root["TargetName"] =  longMsg[PERF_EVENT_KEY_TARGET_NAME];
    root["Callback"] =  longMsg[PERF_EVENT_KEY_CALLBACKS];
    root["MsgWhat"] = longMsg[PERF_EVENT_KEY_MSG_WHAT];
    ParaRoot["ParaClass"] = root;
    sendEventToPipline(ParaRoot, eventId);
}

void PerfLogAnalyzer::sendLongBinderTargetEventToPipline(Json::Value longMsg, Json::Value  longBinder, int eventId)
{
    Json::Value root, ParaRoot;
    root = longMsg;
    root["EventTime"] = TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str(); //902001203
    root["BinderName"] =longBinder[PERF_EVENT_KEY_PACKAGE];
    root["BinderPid"] = longBinder[PERF_EVENT_KEY_BINDER_TARGET_PID];
    root["BinderTargetCost"] =  longBinder[PERF_EVENT_KEY_BINDER_TARGET_BLKIO];
    root["Interface"] = longBinder[PERF_EVENT_KEY_BINDER_INTERFACE];
    root["Code"] = longBinder[PERF_EVENT_KEY_DURATION];
    ParaRoot["ParaClass"] = root;
    sendEventToPipline(ParaRoot, eventId);
}

void PerfLogAnalyzer::sendLongRunnableBinderEventToPipline(Json::Value longMsg,Json::Value longBinder, int eventId)
{
    Json::Value root;
    root = longMsg;
    root["EventTime"] = TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str(); //902001202
    root["BinderTargetTime"] =  longBinder[PERF_EVENT_KEY_BINDER_TARGET_BLKIO];
    sendEventToPipline(root, eventId);
}

void PerfLogAnalyzer::sendLongLockMsgBinderEventToPipline(Json::Value longLock, Json::Value longMsg, Json::Value longBinder, int eventId) {
    Json::Value root; // 902001201
    root = longLock;
    root["EventTime"] = TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str(); //902001203
    root["PackageName"] =longBinder[PERF_EVENT_KEY_PACKAGE];
    root["MsgName"] =  longMsg[PERF_EVENT_KEY_PACKAGE];
    root["MsgPid"] = longMsg[PERF_EVENT_KEY_PID];
    root["MsgWallTime"] = longMsg[PERF_EVENT_KEY_WALLTIME];
    root["Code"] = longBinder[PERF_EVENT_KEY_DURATION];
    root["Interface"] = longBinder[PERF_EVENT_KEY_BINDER_INTERFACE];
    root["MsgPlanTime"] =  longMsg[PERF_EVENT_KEY_PLANTIME_CURR];
    root["Latency"] =  longMsg[PERF_EVENT_KEY_LATENCY];
    root["MsgSeq"] =  longMsg[PERF_EVENT_KEY_SEQ];
    root["TargetName"] =  longMsg[PERF_EVENT_KEY_TARGET_NAME];
    root["Callback"] =  longMsg[PERF_EVENT_KEY_CALLBACKS];
    root["MsgWhat"] = longMsg[PERF_EVENT_KEY_MSG_WHAT];
    sendEventToPipline(root, eventId);
}

void PerfLogAnalyzer::sendLongLockMsgEventToPipline(Json::Value longLock, Json::Value longMsg, int eventId) {
    Json::Value root;
    root = longLock;
    root["EventTime"] = TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str(); //902001203
    root["PackageName"] = longMsg[PERF_EVENT_KEY_PACKAGE];
    root["MsgPid"] = longMsg[PERF_EVENT_KEY_PID];
    root["MsgWallTime"] = longMsg[PERF_EVENT_KEY_WALLTIME];
    root["MsgPlanTime"] =  longMsg[PERF_EVENT_KEY_PLANTIME_CURR];
    root["Latency"] =  longMsg[PERF_EVENT_KEY_LATENCY];
    root["MsgSeq"] =  longMsg[PERF_EVENT_KEY_SEQ];
    root["TargetName"] =  longMsg[PERF_EVENT_KEY_TARGET_NAME];
    root["Callback"] =  longMsg[PERF_EVENT_KEY_CALLBACKS];
    root["MsgWhat"] = longMsg[PERF_EVENT_KEY_MSG_WHAT];
    sendEventToPipline(root, eventId);
}

void PerfLogAnalyzer::saveCommonMsgInfoToDump(Json::Value longMsg, int eventId)
{
    char errInfo[ERR_INFO_BUF] = {};
    snprintf(errInfo,  sizeof(errInfo), "%s: EventId = %d Err in %s pid:%d msg wallTime=%" PRId64 "ms, runningTime=%" PRId64
        "ms, runnableTime=%" PRId64"ms, cpuUTime=%" PRId64"ms, cpuSTime=%" PRId64"ms, ioTime=%" PRId64
        "ms, swapinTime=%" PRId64"ms, reclaimTime=%" PRId64"ms, msg planTime:%" PRId64 "ms latency:%" PRId64
        "ms seq:%d targetName:%s callback:%s msgWhat:%d", TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str(), eventId,
        longMsg[PERF_EVENT_KEY_PACKAGE].asString().c_str(), longMsg[PERF_EVENT_KEY_PID].asInt(),
        longMsg[PERF_EVENT_KEY_WALLTIME].asInt64(), longMsg[PERF_EVENT_KEY_RUNNING_TIME].asInt64(),
        longMsg[PERF_EVENT_KEY_RUNNABLE_TIME].asInt64(), longMsg[PERF_EVENT_KEY_CPU_UTIME].asInt64(),
        longMsg[PERF_EVENT_KEY_CPU_STIME].asInt64(),  longMsg[PERF_EVENT_KEY_IO_TIME].asInt64(),
        longMsg[PERF_EVENT_KEY_SWAPIN_TIME].asInt64(), longMsg[PERF_EVENT_KEY_RECLAIM_TIME].asInt64(),
        longMsg[PERF_EVENT_KEY_PLANTIME_CURR].asInt64(), longMsg[PERF_EVENT_KEY_LATENCY].asInt64(),
        longMsg[PERF_EVENT_KEY_SEQ].asInt(), longMsg[PERF_EVENT_KEY_TARGET_NAME].asString().c_str(),
        longMsg[PERF_EVENT_KEY_CALLBACKS].asString().c_str(),longMsg[PERF_EVENT_KEY_MSG_WHAT].asInt());
    mDumpErrEvents.next() = errInfo;
}

void PerfLogAnalyzer::saveCommonMsgBinderInfoToDump(Json::Value longMsg, Json::Value longBinder, int eventId)
{
    char errInfo[ERR_INFO_BUF] = {};
    snprintf(errInfo, sizeof(errInfo), "%s: EventId= %d Err in %s pid:%d msg took %" PRId64"ms, binder time=%" PRId64
        "ms, interface=%s, code=%d,msg planTime:%s latency:%" PRId64"ms seq:%d targetName:%s callback:%s msgWhat:%d",
        TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str(), eventId, longMsg[PERF_EVENT_KEY_PACKAGE].asString().c_str(),
        longMsg[PERF_EVENT_KEY_PID].asInt(), longMsg[PERF_EVENT_KEY_WALLTIME].asInt64(),
        longBinder[PERF_EVENT_KEY_DURATION].asInt64(), longBinder[PERF_EVENT_KEY_BINDER_INTERFACE].asString().c_str(),
        longBinder[PERF_EVENT_KEY_DURATION].asInt(), longMsg[PERF_EVENT_KEY_PLANTIME_CURR].asString().c_str(),
        longMsg[PERF_EVENT_KEY_LATENCY].asInt64(), longMsg[PERF_EVENT_KEY_SEQ].asInt(),
        longMsg[PERF_EVENT_KEY_TARGET_NAME].asString().c_str(), longMsg[PERF_EVENT_KEY_CALLBACKS].asString().c_str(),
        longMsg[PERF_EVENT_KEY_MSG_WHAT].asInt());
    mDumpErrEvents.next() = errInfo;
}

void PerfLogAnalyzer::saveLongLockBinderInfoToDump(Json::Value longLock, Json::Value longMsg, Json::Value longBinder, int eventId)
{
    char errInfo[ERR_INFO_BUF] = {};
    snprintf(errInfo, sizeof(errInfo), "%s:EventId= %d Err in %s lock contention in binder cause %s pid:%d msg took %" PRId64"ms,"
        "binder interface=%s, code=%d, lock waiter_thread:%s,waiter_time:%" PRId64
        "ms, waiter_fileName:%s, waiter_lineNumber:%d, waiter_methodName:%s,ownerFileName:%s, ownerLineNumber:%d, ownerMethodName:%s"
        "msg planTime:%s latency:%" PRId64"ms seq:%d targetName:%s callback:%s msgWhat:%d",
        TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str(), eventId,
        longLock[PERF_EVENT_KEY_PROCESSNAME].asString().c_str(), longMsg[PERF_EVENT_KEY_PACKAGE].asString().c_str(), longMsg[PERF_EVENT_KEY_PID].asInt(),
        longMsg[PERF_EVENT_KEY_WALLTIME].asInt64(), longBinder[PERF_EVENT_KEY_BINDER_INTERFACE].asString().c_str(), longBinder[PERF_EVENT_KEY_DURATION].asInt(),
        longLock[PERF_EVENT_KEY_THREAD_NAME].asString().c_str(),
        longLock[PERF_EVENT_KEY_LATENCY].asInt64(), longLock[PERF_EVENT_KEY_FILE_NAME].asString().c_str(),  longLock[PERF_EVENT_KEY_LINE_NUMBER].asInt(),
        longLock[PERF_EVENT_KEY_METHOD_NAME].asString().c_str(), longLock[PERF_EVENT_KEY_OWNER_FILE_NAME].asString().c_str(),
        longLock[PERF_EVENT_KEY_OWNER_LINE_NUMBER].asInt(), longLock[PERF_EVENT_KEY_OWNER_METHOD_NAME].asString().c_str(),
        longMsg[PERF_EVENT_KEY_PLANTIME_CURR].asString().c_str(), longMsg[PERF_EVENT_KEY_LATENCY].asInt64(), longMsg[PERF_EVENT_KEY_SEQ].asInt(),
        longMsg[PERF_EVENT_KEY_TARGET_NAME].asString().c_str(), longMsg[PERF_EVENT_KEY_CALLBACKS].asString().c_str(),longMsg[PERF_EVENT_KEY_MSG_WHAT].asInt());
    mDumpErrEvents.next() = errInfo;
}

void PerfLogAnalyzer::saveLongLockMsgInfoToDump(Json::Value longLock, Json::Value longMsg, int eventId) {
    char errInfo[ERR_INFO_BUF] = {};
    snprintf(errInfo, sizeof(errInfo), "%s:EventId= %d Err in %s lock contention cause %s pid:%d msg took %" PRId64
        "ms, waiter_thread:%s, waiter_time:%" PRId64"ms, waiter_fileName:%s, waiter_lineNumber:%d, waiter_methodName:%s,"
        "ownerFileName:%s, ownerLineNumber:%d, ownerMethodName:%s, msg planTime:%s latency:%" PRId64
        "ms seq:%d targetName:%s callback:%s msgWhat:%d",
        TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str(), eventId,
        longLock[PERF_EVENT_KEY_PROCESSNAME].asString().c_str(), longMsg[PERF_EVENT_KEY_PACKAGE].asString().c_str(),
        longMsg[PERF_EVENT_KEY_PID].asInt(), longMsg[PERF_EVENT_KEY_WALLTIME].asInt64(),
        longLock[PERF_EVENT_KEY_THREAD_NAME].asString().c_str(), longLock[PERF_EVENT_KEY_LATENCY].asInt64(),
        longLock[PERF_EVENT_KEY_FILE_NAME].asString().c_str(),  longLock[PERF_EVENT_KEY_LINE_NUMBER].asInt(),
        longLock[PERF_EVENT_KEY_METHOD_NAME].asString().c_str(), longLock[PERF_EVENT_KEY_OWNER_FILE_NAME].asString().c_str(),
        longLock[PERF_EVENT_KEY_OWNER_LINE_NUMBER].asInt(), longLock[PERF_EVENT_KEY_OWNER_METHOD_NAME].asString().c_str(),
        longMsg[PERF_EVENT_KEY_PLANTIME_CURR].asString().c_str(), longMsg[PERF_EVENT_KEY_LATENCY].asInt64(),
        longMsg[PERF_EVENT_KEY_SEQ].asInt(), longMsg[PERF_EVENT_KEY_TARGET_NAME].asString().c_str(),
        longMsg[PERF_EVENT_KEY_CALLBACKS].asString().c_str(),longMsg[PERF_EVENT_KEY_MSG_WHAT].asInt());
    mDumpErrEvents.next() = errInfo;
}

void PerfLogAnalyzer::saveComInfoToDump(Json::Value exceptionLog, int eventId) {
    char errInfo[ERR_INFO_BUF] = {};
    std::string eventTime =TimeUtil::GetTimeDateStr(time(0), TimeFormat).c_str();
    Json::Value account = exceptionLog;
    int ret = 0;
    if ((eventId%1000)/100 ==3) {
        ret  = snprintf(errInfo, sizeof(errInfo),  "%s EventId: %d The warninginfo is: ", eventTime.c_str(), eventId);
    }
    else {
        ret  = snprintf(errInfo, sizeof(errInfo),  "%s EventId: %d The errorinfo is: ", eventTime.c_str(), eventId);
    }
    for (auto iter = account.begin(); iter != account.end(); iter++) {
        if(iter.key().asString() == "EventID") {
            continue;
        }
        if (ret < 0 || ret >= ERR_INFO_BUF) {
            break;
        }
        if (iter->type() == Json::stringValue) {
            ret += snprintf(errInfo+ret, ERR_INFO_BUF-ret,  "%s:%s.", iter.key().asString().c_str(),
                   account[iter.key().asString()].asString().c_str());
        }
        else if(iter->type() == Json::intValue) {
            ret += snprintf(errInfo+ret, ERR_INFO_BUF-ret, "%s:%" PRId64".", iter.key().asString().c_str(),
                   account[iter.key().asString()].asInt64());
        }
        else{
            ret += snprintf(errInfo+ret, ERR_INFO_BUF-ret,  "%s:%s,", iter.key().asString().c_str(),
                   account[iter.key().asString()].asString().c_str());
         }
    }
    mDumpErrEvents.next() = errInfo;
}

void PerfLogAnalyzer::OnUnload() {

}
}
}//namespace android
