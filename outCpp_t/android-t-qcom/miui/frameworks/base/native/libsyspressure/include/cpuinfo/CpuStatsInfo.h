/**
 * parse proc/stat info.
 */
#pragma once

#include "common/Common.h"
#include "common/FileRead.h"

namespace android {

namespace syspressure {

class CpuInfos {
friend class CpuStatsInfo;
friend class ParseCpuPressure;
public:
    CpuInfos();
    CpuInfos(long unsigned userTime,
               long unsigned niceTime,
               long unsigned sysTime,
               long unsigned idleTime,
               long unsigned ioWaitTime,
               long unsigned irqTime,
               long unsigned softIrqTime);
    const CpuInfos operator +(const CpuInfos& info) const;
    const CpuInfos operator -(const CpuInfos& info) const;
private:
    void toJiffyMillis();
private:
    long unsigned mUserTime;
    long unsigned mNiceTime;
    long unsigned mSysTime;
    long unsigned mIdleTime;
    long unsigned mIOWaitTime;
    long unsigned mIrqTime;
    long unsigned mSoftIrqTime;
};

class LoadavgInfo {
friend class CpuStatsInfo;
friend class ParseCpuPressure;
private:
    float mLoad1;
    float mLoad5;
    float mLoad15;
};

class CpuStatsInfo {
friend class ParseCpuPressure;
public:
    CpuStatsInfo();
    ~CpuStatsInfo();
    int parseRootCpuStat(char *buf);
    int parseLoadavgInfo(char *buf);
    uint64_t parseProcCpuStat(char *buf);
    int getCpuCount();
private:
    int readCpuCount();
    int readCpuCountForFile(const char *filename);
    int readFreqStats(int cpu);
private:
    int miCpuCount;
    CpuInfos mBaseTotalCpu;
    CpuInfos * mpBaseCpuInfos;

    CpuInfos mRelTotalCpu;
    CpuInfos * mpRelCpuInfos;
    LoadavgInfo mLoadavgInfo;
};

} // namespace syspressure

} // namespace android