/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef LOG_H
#define LOG_H

#include <chrono>
#include <deque>
#include <queue>  /* priority_queue */
#include <string>
#include <vector>

#include <cutils/properties.h>
#include <log/log.h>
#include <utils/Log.h>

using namespace std;

/* GENERAL */

#define LOG_IF_HAS_LOGGER(prio, ...) (void)log.log(prio, __VA_ARGS__);

#define LOGV(...) LOG_IF_HAS_LOGGER(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#define LOGD(...) LOG_IF_HAS_LOGGER(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define LOGI(...) LOG_IF_HAS_LOGGER(ANDROID_LOG_INFO, __VA_ARGS__)
#define LOGW(...) LOG_IF_HAS_LOGGER(ANDROID_LOG_WARN, __VA_ARGS__)
#define LOGE(...) LOG_IF_HAS_LOGGER(ANDROID_LOG_ERROR, __VA_ARGS__)

enum LogLevel {
    NOTSET = 0,
    DEFAULT,
    VERBOSE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    SILENT
};

struct LogEntry {
    chrono::system_clock::time_point time;
    string message;

    string toString() const;
};

class LoggerBase {
public:
    LoggerBase(const string loggerName, int csvFormatVersion);
    virtual ~LoggerBase() {};

    void setLogRootDirPath(const string &path);
    void setLogDirPath(const string &path);
    void setLogLevel(LogLevel logLevel) { mLogLevel = logLevel; }
    void logToAndroidLoggingSystem(bool enabled);

    void log(int prio, const char *format, ...);
    void markExitReason(const string reason);

    void save();

protected:
    const string mLoggerName;
    string mLogRootDirPath, mOverridedLogDirPath;
    const int mCsvFormatVersion;
    int mLogLevel;
    bool mLogToAndroid;

    virtual void traceBegin();
    virtual void traceEnd();

    /* statistic data */
    chrono::system_clock::time_point mBeginRealTime, mEndRealTime;
    chrono::steady_clock::time_point mBeginTime, mEndTime;
    double getTotalElapsedSeconds();

    string mExitReason;

    virtual void saveCsvInner(ofstream &file) = 0;

private:
    /* logs */
    vector<LogEntry> mBeginningLogs;
    deque<LogEntry> mEndingLogs;

    /* fingerprint */
    string getBuildFingerprint();
    string getBootimageBuildFingerprint();

    /* file io */
    static bool changePermission(string path, bool isDir);
    static bool changeSymbolicPermission(string path);
    static bool createDirAndChangePermission(string dirPath);
    string getLogDirPath();
    bool logRootDirExceedLimit();

    void saveCsv(string logPath);
    void saveLog(string logPath);
    void createFinishFlagFile(string path);

    virtual vector<string> checkConstraint() { return vector<string>(); }

    // disable copy
    LoggerBase(const LoggerBase&) = delete;
};

/* COMPACT */

struct MbGroupsEntry {
    int groupNo, free, frags, first;
};

typedef vector<MbGroupsEntry> MbGroupsResult;

struct GroupStat {
    int groupNo;
    int freeBlockCountBefore, freeBlockCountAfter;
    int fragCountBefore, fragCountAfter;
    int movedBlockNr;
    unsigned long long fsWrittenBefore, fsWrittenAfter;
    chrono::system_clock::time_point beginRealTime, endRealTime;
    chrono::steady_clock::time_point beginTime, endTime;

    double getElapsedSeconds();
};

class CompactLogger : public LoggerBase {
public:
    CompactLogger();
    virtual ~CompactLogger() {};

    void traceBegin() override;
    void traceGroupBegin(int groupNo);
    void traceGroupEnd(int groupNo, int movedBlockNr);
    void traceEnd(__u64 totalIcheckNum, __u64 icheckHitNum,
                  __u64 icheckLeftNum, __u64 icheckRightNum,
                  __u64 icheckCacheHitNum,
                  __u64 totalNcheckNum, __u64 ncheckHitNum,
                  __u64 ncheckLeftNum, __u64 ncheckRightNum,
                  __u64 ncheckCacheHitNum);

    void setMaxSearchGroupNr(int nr) { mMaxSearchGroupNr = nr; }
private:
    using LoggerBase::traceEnd;

    bool mTracingInGroup;

    string mBlockDevicePath;
    string getBlockDeviceName();

    void saveCsvInner(ofstream &file) override;

    /* statistic data */
    GroupStat mPendingGroupStat;
    vector<GroupStat> mGroupStats;

    __u64 mTotalIcheckNum, mIcheckHitNum;
    __u64 mIcheckLeftNum, mIcheckRightNum;
    __u64 mIcheckCacheHitNum;

    __u64 mTotalNcheckNum, mNcheckHitNum;
    __u64 mNcheckLeftNum, mNcheckRightNum;
    __u64 mNcheckCacheHitNum;

    /* fs info */
    bool readMbGroups(MbGroupsResult &result);
    MbGroupsEntry readMbGroupsByGroupNo(int groupNo);

    /* lifetime */
    unsigned long long readFsLifetimeWriteKBytes();

    /* debugging */
    vector<string> checkConstraint() override;
    int mMaxSearchGroupNr = 16;
};

/* FILE DEFRAG */

struct FileStat {
    FileStat() { FileStat(string()); }
    FileStat(string path);

    string path;
    int extentNrBefore = 0, extentNrAfter = 0;
    off_t size = 0;

    chrono::system_clock::time_point beginRealTime, endRealTime;
    chrono::steady_clock::time_point beginTime, endTime;

    bool operator < (const FileStat &another) const {
        // keep the FileStat with less size on the root of the heap
        return size > another.size;
    }

    double getElapsedSeconds();
};

class LimitedSizeHeap : public std::priority_queue<FileStat> {
public:
    LimitedSizeHeap(size_t limit) : mLimit(limit) {}
    void push(const FileStat &x) {
        for (; this->size() > mLimit - 1; priority_queue::pop());
        priority_queue::push(x);
    }
    vector<FileStat> sort() {
        vector<FileStat> r(this->c.begin(), this->c.end());
        std::sort(r.begin(), r.end());
        return r;
    }
private:
    const unsigned int mLimit;
};

class FileDefragLogger : public LoggerBase {
public:
    FileDefragLogger();
    virtual ~FileDefragLogger() {}

    void traceBegin() override;
    void traceFileBegin(string path);
    void traceFileEnd(int extentNrBefore, int extentNrAfter);
    void traceEnd(int totalExtentsNrBefore, int totalExtentsNrAfter,
                  long *gDefragStat, long *gDonorCallDefragEstat);

private:
    using LoggerBase::traceEnd;

    bool mTracingInFile;
    FileStat mPendingFileStat;
    LimitedSizeHeap mFileStats;

    int mTotalExtentsNrBefore, mTotalExtentsNrAfter;
    long *mpDefragStat, *mpDonorCallDefragEstat;

    void saveCsvInner(ofstream &file) override;
};

LogLevel parseLogLevelArg(char *arg);
#endif // LOG_H
