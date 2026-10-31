#include "common/Common.h"
#include "cpuinfo/CpuStatsInfo.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "syspressure.cpuinfo"

namespace android {

namespace syspressure {

/*=====================CpuInfos start=====================*/
CpuInfos::CpuInfos() : mUserTime(0),
                       mNiceTime(0),
                       mSysTime(0),
                       mIdleTime(0),
                       mIOWaitTime(0),
                       mIrqTime(0),
                       mSoftIrqTime(0) {
}

CpuInfos::CpuInfos(long unsigned userTime,
                   long unsigned niceTime,
                   long unsigned sysTime,
                   long unsigned idleTime,
                   long unsigned ioWaitTime,
                   long unsigned irqTime,
                   long unsigned softIrqTime) : mUserTime(userTime),
                       mNiceTime(niceTime),
                       mSysTime(sysTime),
                       mIdleTime(idleTime),
                       mIOWaitTime(ioWaitTime),
                       mIrqTime(irqTime),
                       mSoftIrqTime(softIrqTime) {
}

const CpuInfos CpuInfos::operator +(const CpuInfos& info) const {
    const CpuInfos result(mUserTime + info.mUserTime,
                          mNiceTime + info.mNiceTime,
                          mSysTime + info.mSysTime,
                          mIdleTime + info.mIdleTime,
                          mIOWaitTime + info.mIOWaitTime,
                          mIrqTime + info.mIrqTime,
                          mSoftIrqTime + info.mSoftIrqTime);
    return result;
}

const CpuInfos CpuInfos::operator -(const CpuInfos& info) const {
    const CpuInfos result(mUserTime - info.mUserTime,
                          mNiceTime - info.mNiceTime,
                          mSysTime - info.mSysTime,
                          mIdleTime - info.mIdleTime,
                          mIOWaitTime - info.mIOWaitTime,
                          mIrqTime - info.mIrqTime,
                          mSoftIrqTime - info.mSoftIrqTime);
    return result;
}

void CpuInfos::toJiffyMillis() {
    mUserTime = mUserTime * JIFFY_MS;
    mNiceTime = mNiceTime * JIFFY_MS;
    mSysTime = mSysTime * JIFFY_MS;
    mIdleTime = mIdleTime * JIFFY_MS;
    mIOWaitTime = mIOWaitTime * JIFFY_MS;
    mIrqTime = mIrqTime * JIFFY_MS;
    mSoftIrqTime = mSoftIrqTime * JIFFY_MS;
}

/*=====================CpuInfos end=====================*/

/*=====================CpuStatsInfo start=====================*/
CpuStatsInfo::CpuStatsInfo() : miCpuCount(0), mpRelCpuInfos(nullptr) {
    miCpuCount = readCpuCount();
    if(miCpuCount > 0) {
        mpBaseCpuInfos = new CpuInfos[miCpuCount];
        mpRelCpuInfos = new CpuInfos[miCpuCount];
    }
}

CpuStatsInfo::~CpuStatsInfo() {
    delete []mpBaseCpuInfos;
    delete []mpRelCpuInfos;
}

int CpuStatsInfo::parseRootCpuStat(char *buf) {
    char *savePtr;
    char *line = strtok_r(buf, "\n", &savePtr);
    if (mpBaseCpuInfos == nullptr || mpRelCpuInfos == nullptr) {
        return STATE_BAD_VALUE;
    }
    // total
    CpuInfos cpuTime;
    sscanf(line, "cpu  %lu %lu %lu %lu %lu %lu %lu %*d %*d %*d\n",
               &cpuTime.mUserTime, &cpuTime.mNiceTime, &cpuTime.mSysTime, &cpuTime.mIdleTime,
               &cpuTime.mIOWaitTime, &cpuTime.mIrqTime, &cpuTime.mSoftIrqTime);
    cpuTime.toJiffyMillis();
    mRelTotalCpu = cpuTime - mBaseTotalCpu;
    mBaseTotalCpu = cpuTime;
    long totalTime = mRelTotalCpu.mUserTime + mRelTotalCpu.mNiceTime + mRelTotalCpu.mSysTime +
                    mRelTotalCpu.mIdleTime + mRelTotalCpu.mIOWaitTime + mRelTotalCpu.mIrqTime +
                    mRelTotalCpu.mSoftIrqTime;

    if (DEBUG) {
        // cpu:total|user|kernel|iowait|irq|softirq
        ALOG(LOG_DEBUG, LOG_TAG, "cpu:%.1f|%.1f|%.1f|%.1f|%.1f|%.1f",
                      (totalTime - mRelTotalCpu.mIdleTime)*100.0/totalTime/*Total*/,
                      (mRelTotalCpu.mUserTime + mRelTotalCpu.mNiceTime)*100.0/totalTime/*User*/,
                      (mRelTotalCpu.mSysTime)*100.0/totalTime/*kernel*/,
                      (mRelTotalCpu.mIOWaitTime)*100.0/totalTime/*iowait*/,
                      (mRelTotalCpu.mIrqTime)*100.0/totalTime/*irq*/,
                      (mRelTotalCpu.mSoftIrqTime)*100.0/totalTime/*softirq*/);
    }
    // sub
    // cpui:total|user|kernel|iowait|irq|softirq|freq
    char scanline[MAX_READ_LINE];
    for (int i = 0; i < miCpuCount; i++) {
        line = strtok_r(nullptr, "\n", &savePtr);
        sprintf(scanline, "cpu%d %%lu %%lu %%lu %%lu %%lu %%lu %%lu %%*d %%*d %%*d\n", i);
        sscanf(line, scanline, &cpuTime.mUserTime, &cpuTime.mNiceTime,
                               &cpuTime.mSysTime, &cpuTime.mIdleTime,
                               &cpuTime.mIOWaitTime, &cpuTime.mIrqTime, &cpuTime.mSoftIrqTime);
        cpuTime.toJiffyMillis();
        mpRelCpuInfos[i] = cpuTime - mpBaseCpuInfos[i];
        mpBaseCpuInfos[i] = cpuTime;

        totalTime = mpRelCpuInfos[i].mUserTime + mpRelCpuInfos[i].mNiceTime
                    + mpRelCpuInfos[i].mSysTime + mpRelCpuInfos[i].mIdleTime
                    + mpRelCpuInfos[i].mIOWaitTime + mpRelCpuInfos[i].mIrqTime +
                    mpRelCpuInfos[i].mSoftIrqTime;
        if (DEBUG) {
            ALOG(LOG_DEBUG, LOG_TAG,"cpu%d:%.1f|%.1f|%.1f|%.1f|%.1f|%.1f", i,
                  (totalTime - mpRelCpuInfos[i].mIdleTime)*100.0/totalTime/*Total*/,
                  (mpRelCpuInfos[i].mUserTime + mpRelCpuInfos[i].mNiceTime)*100.0/totalTime/*User*/,
                  (mpRelCpuInfos[i].mSysTime)*100.0/totalTime/*kernel*/,
                  (mpRelCpuInfos[i].mIOWaitTime)*100.0/totalTime/*iowait*/,
                  (mpRelCpuInfos[i].mIrqTime)*100.0/totalTime/*irq*/,
                  (mpRelCpuInfos[i].mSoftIrqTime)*100.0/totalTime/*softirq*/);
        }
    }
    return STATE_OK;
}

int CpuStatsInfo::parseLoadavgInfo(char *buf) {
    sscanf(buf, "%f %f %f %*s %*s %*s\n",
               &mLoadavgInfo.mLoad1, &mLoadavgInfo.mLoad5, &mLoadavgInfo.mLoad15);
    return STATE_OK;
}
/*
 * Read the frequency stats for a given cpu.
 */
int CpuStatsInfo::readFreqStats(int cpu) {
    FILE *file;
    char filename[MAX_PATH_LENGTH];
    int freq = 0;

    sprintf(filename, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu);
    file = fopen(filename, "r");
    if (file) {
        fscanf(file, "%d", &freq);
        fclose(file);
    }
    return freq;
}

int CpuStatsInfo::getCpuCount() {
    return miCpuCount;
}

int CpuStatsInfo::readCpuCount() {
    int cpuCount = readCpuCountForFile("/sys/devices/system/cpu/present");
    if (cpuCount != readCpuCountForFile("/sys/devices/system/cpu/online")) {
        ALOG(LOG_ERROR, LOG_TAG, "present cpus != online cpus\n");
    }
    return cpuCount;
}

/*
 * Get the number of CPUs from a given filename.
 */
int CpuStatsInfo::readCpuCountForFile(const char *filename) {
    FILE *file;
    char line[10];
    int cpuCount;

    file = fopen(filename, "r");
    if (!file) {
        ALOG(LOG_ERROR, LOG_TAG, "Could not open %s\n", filename);
        return -1;
    }
    if (!fgets(line, sizeof(line), file))
        ALOG(LOG_ERROR, LOG_TAG, "Could not get %s contents\n", filename);
    fclose(file);

    if (strcmp(line, "0\n") == 0) {
        return 1;
    }

    if (1 == sscanf(line, "0-%d\n", &cpuCount)) {
        return cpuCount + 1;
    }

    ALOG(LOG_ERROR, LOG_TAG, "Unexpected input in file %s (%s).\n", filename, line);
    return -1;
}

/*
* parse /proc/%d/stat info
*
* @return cpu usage
*/
uint64_t CpuStatsInfo::parseProcCpuStat(char *buffer) {
    // field 3 is  state (R is running, S is sleeping,
    // D is sleeping in an uninterruptible wait, Z is zombie, T is traced or stopped)
    // field 14 is utime
    // field 15 is stime
    // field 18 is priority
    // field 19 is nice
    // field 20 is num_threads
    // field 24 is rss
    // field 39 is task_cpu
    // field 42 is iotime(time spent waiting for block IO)
     ALOG(LOG_DEBUG, LOG_TAG, "parseProcCpuStat %s", buffer);
    unsigned long long utime = 0, stime = 0, iotime = 0;
    int taskCpu = 0;
    if (sscanf(buffer,
              "%*u %*s %*s %*d %*d %*d %*d %*d %*d %*d "
              "%*d %*d %*d %llu %llu %*d %*d %*d %*d %*d "
              "%*d %*d %*d %*d %*d %*d %*d %*d %*d %*d "
              "%*d %*d %*d %*d %*d %*d %*d %*d %d %*d "
              "%*d %llu ",
               &utime, &stime, &taskCpu, &iotime) != 4) {
        return STATE_BAD_VALUE;
    }
    uint64_t cputime = utime + stime;
    return cputime;
}
/*=====================CpuStatsInfo end=====================*/
} // namespace syspressure

} // namespace android