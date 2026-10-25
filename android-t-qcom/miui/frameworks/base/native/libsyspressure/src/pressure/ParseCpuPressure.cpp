#include "common/Common.h"
#include "pressure/ParseCpuPressure.h"
#include "common/FileRead.h"
#include "common/EventsNodeThread.h"
#include "pressure/PressureMonitor.h"
#include "SysPressureCtrlManager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "syspressure.cpuinfo"

namespace android {

namespace syspressure {

#define STAT_PATH                       "/proc/stat"
#define LOADAVG_PATH                    "/proc/loadavg"
#define MIN_PARSE_CPUINFO_INTERVAL_MS   10000 // 10s

#define THERMAL_BALANCE_MODE_FILE       "/sys/class/thermal/thermal_message/balance_mode"
#define THERMAL_TEMPERATURE_FILE        "/sys/class/thermal/thermal_message/board_sensor_temp"
#define BACKGROUND_CPU_CORE             "/dev/cpuset/background/cpus"

#define CPU_PRESSURE_USE_LEVEL1         40
#define CPU_PRESSURE_USE_LEVEL2         80

#define THERMAL_BALANCE_MODE_VALUE      2
#define THERMAL_TEMP_NOTIFY_THRESHOLD   39

ParseCpuPressure::ParseCpuPressure() : mPreParseTime(0),
                    mThermalBalanceMode(THERMAL_BALANCE_MODE_VALUE),
                    mThermalTemperature(0),
                    mBackgroundCpuPolicy("0-2"){
    Vector<std::string> events;
    events.push_back(THERMAL_BALANCE_MODE_FILE);
    events.push_back(THERMAL_TEMPERATURE_FILE);
    events.push_back(BACKGROUND_CPU_CORE);
    mEventsNodeThread = new EventsNodeThread(events);
    mEventsNodeThread->setEventsDataCall(
                std::bind(&ParseCpuPressure::onEventsDataChange,
                        this, std::placeholders::_1, std::placeholders::_2));
    mEventsNodeThread->run("EventsNodeThread");
}

ParseCpuPressure::~ParseCpuPressure() {
    if (mEventsNodeThread != nullptr) {
        mEventsNodeThread->requestExit();
        mEventsNodeThread->join();
    }
}

/*
* parse /proc/stat info
*/
int ParseCpuPressure::parseCpuInfo() {
    // filter
    uint64_t currentTime = Utils::milliTime();
    if((currentTime - mPreParseTime) < MIN_PARSE_CPUINFO_INTERVAL_MS)
        return STATE_OK;
    mPreParseTime = currentTime;

    char *buf;
    ATRACE_CALL();
    // stat info
    static FileReadParam statParam(STAT_PATH);
    if ((buf = mFileRead.rereadFile(&statParam)) == nullptr) {
        return STATE_NO_MEMORY;
    }
    int ret = mCpuStatsInfo.parseRootCpuStat(buf);
    if(ret != STATE_OK) {
        return ret;
    }

    // cpu pressure
    checkCpuPressure();
    // cpu exception
    checkCpuException();
    return STATE_OK;
}

bool ParseCpuPressure::checkCpuPressure() {
    CpuInfos& relCpuInfos = mCpuStatsInfo.mRelTotalCpu;
    long totalTime = relCpuInfos.mUserTime + relCpuInfos.mNiceTime + relCpuInfos.mSysTime +
                    relCpuInfos.mIdleTime + relCpuInfos.mIOWaitTime + relCpuInfos.mIrqTime +
                    relCpuInfos.mSoftIrqTime;
    int cpuUse = (relCpuInfos.mUserTime + relCpuInfos.mNiceTime
                    + relCpuInfos.mSysTime) * 100.0 / totalTime;
    if (cpuUse >= CPU_PRESSURE_USE_LEVEL2) {
        SysPressureCtrlManager::getInstance()->notifyCpuPressure(PRESSURE_MEDIUM_LEVEL);
        ALOG(LOG_DEBUG, LOG_TAG, "process cpu pressure medium");
    } else if (cpuUse >= CPU_PRESSURE_USE_LEVEL1) {
       SysPressureCtrlManager::getInstance()->notifyCpuPressure(PRESSURE_LOW_LEVEL);
       ALOG(LOG_DEBUG, LOG_TAG, "process cpu pressure low");
   }
   return true;
}

bool ParseCpuPressure::checkCpuException() {
    int littleCoreException = 0;
    int bigCoreException = 0;
    CpuInfos * relCpuInfos = mCpuStatsInfo.mpRelCpuInfos;
    int cpuCount = mCpuStatsInfo.getCpuCount();
    for (int i = 0; i < cpuCount; i++) {
        long totalTime = relCpuInfos[i].mUserTime + relCpuInfos[i].mNiceTime
                    + relCpuInfos[i].mSysTime + relCpuInfos[i].mIdleTime
                    + relCpuInfos[i].mIOWaitTime + relCpuInfos[i].mIrqTime +
                    relCpuInfos[i].mSoftIrqTime;
        long cpuUse = (relCpuInfos[i].mUserTime + relCpuInfos[i].mNiceTime
                    + relCpuInfos[i].mSysTime) * 100.0 / totalTime;
        if (cpuUse >= kCpuExceptionThreshold) {
            if (i < cpuCount/2) {
                littleCoreException ++;
            } else {
                bigCoreException ++;
            }
        }
    }
    int cpuExceptionType = CPU_EXCEPTON_NOTE;
    if (bigCoreException >= 2 && littleCoreException >= 2) {
        cpuExceptionType = CPU_EXCEPTON_ALL_CORE;
    } else if (bigCoreException >= 2) {
        cpuExceptionType = CPU_EXCEPTON_BIG_CORE;
    } else if (littleCoreException >= 2) {
        cpuExceptionType = CPU_EXCEPTON_LITTLE_CORE;
    }
    if (cpuExceptionType != CPU_EXCEPTON_NOTE) {
        SysPressureCtrlManager::getInstance()->notifyCpuException(cpuExceptionType);
        ALOG(LOG_DEBUG, LOG_TAG, "process cpu exception type = %d", cpuExceptionType);
        return true;
    }
    return false;
}

uint64_t ParseCpuPressure::getProcCpuUsage(int32_t pid) {
    memset(mStatPath, 0, sizeof(mStatPath));
    sprintf(mStatPath, "/proc/%d/stat", pid);
    if(!Utils::existsFile(mStatPath)) {
        return STATE_NOT_FOUND;
    }
    // read
    FileReadParam param(mStatPath);
    char *buf;
    if ((buf = mFileRead.rereadFile(&param)) == nullptr) {
        return STATE_NO_MEMORY;
    }
    mFileRead.closeFile(&param);
    return mCpuStatsInfo.parseProcCpuStat(buf);
}

std::string ParseCpuPressure::getBackgroundCpuPolicy() {
    if (mThermalBalanceMode == THERMAL_BALANCE_MODE_VALUE) {
        return "0-2";
    }
    return mBackgroundCpuPolicy;
}

void ParseCpuPressure::thermalTemperatureChange(int temp) {
    temp = temp/1000;
    if (temp >= THERMAL_TEMP_NOTIFY_THRESHOLD && mThermalTemperature != temp) {
        mThermalTemperature = temp;
        SysPressureCtrlManager::getInstance()->notifyThermalTempChange(temp);
    }
}

void ParseCpuPressure::onEventsDataChange(const std::string path, const std::string buffer) {
    if (DEBUG) {
        ALOG(LOG_DEBUG, LOG_TAG, "onEventsDataChange %s buffer=%s", path.c_str(), buffer.c_str());
    }
    if (path == THERMAL_TEMPERATURE_FILE) {
        thermalTemperatureChange(atoi(buffer.c_str()));
    } else if (path == THERMAL_BALANCE_MODE_FILE) {
        mThermalBalanceMode = atoi(buffer.c_str());
    } else if (path == BACKGROUND_CPU_CORE) {
        mBackgroundCpuPolicy = buffer;
    }
}

} // namespace syspressure

} // namespace android