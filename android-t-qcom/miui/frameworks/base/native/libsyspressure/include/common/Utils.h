/**
 * Utils.h
 * 共通工具类
 */
#pragma once
#include "common/Common.h"

using namespace std;

namespace android {

namespace syspressure {

class Utils
{
public:
    static inline bool stringToLong(const char* str, int64_t* value) {
        char* endptr;
        long long val = strtoll(str, &endptr, 10);
        if (str == endptr || val > INT64_MAX) {
            return false;
        }
        *value = (int64_t)val;
        return true;
    }
    /**
     * sleep us
     **/
    static inline void milliSleep(int32_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    static inline uint64_t milliTime(bool isRealTime = false) {
        timespec now;
        clock_gettime(isRealTime ? CLOCK_REALTIME : CLOCK_MONOTONIC, &now);
        return static_cast<uint64_t>(now.tv_sec) * UINT64_C(1000) + now.tv_nsec / UINT64_C(1000000);
    }

    static inline uint64_t microTime(bool isRealTime= false) {
        timespec now;
        clock_gettime(isRealTime ? CLOCK_REALTIME : CLOCK_MONOTONIC, &now);
        return static_cast<uint64_t>(now.tv_sec) * UINT64_C(1000000) + now.tv_nsec / UINT64_C(1000);
    }

    static inline uint64_t nanoTime(bool isRealTime= false) {
        timespec now;
        clock_gettime(isRealTime ? CLOCK_REALTIME : CLOCK_MONOTONIC, &now);
        return static_cast<uint64_t>(now.tv_sec) * UINT64_C(1000000000) + now.tv_nsec;
    }

    static inline std::string formatTime(uint64_t milli) {
        uint64_t sec = milli/UINT64_C(1000);
        int usec = (int)(milli%UINT64_C(1000));
        tm * ctm = localtime((const time_t*)&(sec));
        char tmp[30] = {0};
        sprintf(tmp, "%02d-%02d %02d:%02d:%02d.%.3d",
                            ctm->tm_mon, ctm->tm_mday,
                            ctm->tm_hour, ctm->tm_min, ctm->tm_sec, usec);
        return std::string(tmp);
    }

    static inline int stopProcess(pid_t pid) {
        return kill(pid,SIGSTOP);
    }

    static inline int startProcess(pid_t pid) {
        return kill(pid,SIGCONT);
    }

    static inline bool existsFile(const char * const path) {
        return access(path, F_OK) != -1;
    }
};

} // namespace syspressure

} // namespace android