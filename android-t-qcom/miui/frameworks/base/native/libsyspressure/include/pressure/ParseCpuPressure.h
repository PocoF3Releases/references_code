/**
 * parse memory info.
 */
#pragma once

#include <utils/Errors.h>
#include <vector>
#include "common/FileRead.h"
#include "cpuinfo/CpuStatsInfo.h"

namespace android {

namespace syspressure {

class EventsNodeThread;

enum CpuExceptionType {
    CPU_EXCEPTON_NOTE = 0,
    CPU_EXCEPTON_LITTLE_CORE,
    CPU_EXCEPTON_BIG_CORE,
    CPU_EXCEPTON_ALL_CORE
};

class ParseCpuPressure {
public:
    ParseCpuPressure();
    ~ParseCpuPressure();
    int parseCpuInfo();
    uint64_t getProcCpuUsage(int32_t pid);
    std::string getBackgroundCpuPolicy();
private:
    bool checkCpuPressure();
    bool checkCpuException();
    void onEventsDataChange(const std::string path, const std::string buffer);
    void thermalTemperatureChange(int temp);
private:
    sp<EventsNodeThread> mEventsNodeThread;
    FileRead mFileRead;
    char mStatPath[MAX_PATH_LENGTH];
    CpuStatsInfo mCpuStatsInfo;
    uint64_t mPreParseTime;
    int mThermalBalanceMode;
    int mThermalTemperature;
    std::string mBackgroundCpuPolicy;
};

} // namespace syspressure

} // namespace android