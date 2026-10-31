/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>  /* statfs() */
#include <unistd.h>

#include <private/android_filesystem_config.h>  /* AID_SYSTEM */

#include "log.h"
#include "utils.h"

using namespace std;

/* GENERAL */

#ifdef LOG_TAG
#undef LOG_TAG
#endif
// confidentiality
#define LOG_TAG "MDGLogger"

// confidentiality
#define DEFRAG_LOG_TAG "MDG"
// log_level (DEBUG=3, INFO=4, WARN=5, ERROR=6, FATAL=7, SILENT=8)
#define PROP_LOG_LEVEL "persist.sys.mem_log_level"

#define LOG_DIR_TIME_FORMAT "%y%m%d-%H%M%S"
#define CSV_TIME_FORMAT "%y/%m/%d-%H:%M:%S"
#define LOG_TIME_FORMAT CSV_TIME_FORMAT

#define MAX_BEGINNING_LOG_NR 1000
#define MAX_ENDING_LOG_NR 10000

/* We must ensure that the total size of persistent log dir never
 * exceeds MAX_PERSISTENT_LOG_DIR_SIZE in corner case.
 *
 * defaults: 100 * 1024 * 1024 == 100 MiB
 */
#define MAX_PERSISTENT_LOG_DIR_SIZE ((off_t)100 * 1024 * 1024)

/* same with logger_write.c */
#define LOG_BUF_SIZE 1024

static double getElapsedSeconds(chrono::steady_clock::time_point begin,
                                chrono::steady_clock::time_point end) {
    using namespace chrono;
    duration<double> timeSpan = duration_cast<duration<double>>(end - begin);
    return timeSpan.count();
}

static string formatRealTime(chrono::system_clock::time_point p, string fmt) {
    time_t realTimeC = chrono::system_clock::to_time_t(p);
    stringstream ss;
    ss << put_time(localtime(&realTimeC), fmt.c_str());
    return ss.str();
}

template <typename T> T readFileAs(string path) {
	// A shortcut to read simple file which have only one line and can be
	// convert to T by istream.
    T ret{};
    ifstream file(path);
    if (file.good()) {
        file >> ret;
    } else {
        ALOGE("Failed to read %s, errno=%d", path.c_str(), errno);
    }
    file.close();
    return ret;
}

string LogEntry::toString() const {
    string ret;
    ret.reserve(17 + 1 + message.length());
    ret.append(formatRealTime(this->time, string(LOG_TIME_FORMAT)));
    ret.append(" ");
    ret.append(this->message);
    return ret;
}

LoggerBase::LoggerBase(const string loggerName, int csvFormatVersion)
    : mLoggerName(loggerName),
      mCsvFormatVersion(csvFormatVersion) {

    ALOGD("CSV_FORMAT_VERSION = %d", mCsvFormatVersion);
    ALOGD("COMPACT_VERSION = %s", COMPACT_VERSION);
    ALOGD("FILE_DEFRAG_VERSION = %s", FILE_DEFRAG_VERSION);

    mLogLevel = property_get_int32(PROP_LOG_LEVEL, ANDROID_LOG_INFO);
    ALOGD("The log level is %d.", mLogLevel);
}

void LoggerBase::setLogRootDirPath(const string &path) {
    mLogRootDirPath = path;
}

void LoggerBase::setLogDirPath(const string &path) {
    mOverridedLogDirPath = path;
}

void LoggerBase::logToAndroidLoggingSystem(bool enabled) {
    mLogToAndroid = enabled;
}

void LoggerBase::log(int prio, const char *format, ...) {
    char msg[LOG_BUF_SIZE] = {0};

    va_list args;
    va_start(args, format);
    vsnprintf(msg, LOG_BUF_SIZE, format, args);
    va_end(args);

    LogEntry entry = {
        .message = string(msg),
        .time = chrono::system_clock::now()
    };

    if (prio >= mLogLevel) {
        if (mLogToAndroid) {
            __android_log_buf_write(LOG_ID_MAIN, prio, DEFRAG_LOG_TAG, msg);
        } else {
            cerr << entry.toString() << endl;
        }
    }

    /* pend to write log file for debug use */
    if (mBeginningLogs.size() < MAX_BEGINNING_LOG_NR) {
        mBeginningLogs.push_back(entry);
    } else {
        while (mEndingLogs.size() > MAX_ENDING_LOG_NR) {
            mEndingLogs.pop_front();  /* drop the earliest log */
        }
        mEndingLogs.push_back(entry);
    }
}

void LoggerBase::markExitReason(const string reason) { mExitReason = reason; }

void LoggerBase::traceBegin() {
    mBeginRealTime = chrono::system_clock::now();
    mBeginTime = chrono::steady_clock::now();
}

void LoggerBase::traceEnd() {
    mEndRealTime = chrono::system_clock::now();
    mEndTime = chrono::steady_clock::now();
}

void LoggerBase::save() {
    /* create root log dir if necessary */
    if (!createDirAndChangePermission(mLogRootDirPath)) {
        return;
    }

    /* prevent producing too many garbage logs in corner case */
    if (logRootDirExceedLimit()) {
        ALOGE("The total size of storage_compact log dir exceeds "
              "the limit %ld bytes.", MAX_PERSISTENT_LOG_DIR_SIZE);
        return;
    }

    /* create log dir */
    string logDirPath = getLogDirPath();
    if (!createDirAndChangePermission(logDirPath)) {
        return;
    }

    /* write files */
    saveCsv(logDirPath + "/" + mLoggerName + ".csv");
    saveLog(logDirPath + "/" + mLoggerName + ".txt");

    /* change symlink "latest" to dir logDirPath
     *
     * It is a convenient way to find out the latest logs for
     * developers. And the dumpstate picks up the latest logs by
     * accessing symlink "latest".
     */
    string latestSymLinkPath = mLogRootDirPath + "/latest";
    struct stat statBuf;
    if (lstat(latestSymLinkPath.c_str(), &statBuf) == 0) {
        /* if symlink exists previously */
        if (unlink(latestSymLinkPath.c_str()) != 0) {
            ALOGE("Failed to remove previous symlink \"%s\", errno=%d",
                  latestSymLinkPath.c_str(), errno);
            goto skipLatestLink;
        }
    }
    /* create symlink */
    if (symlink(logDirPath.c_str(), latestSymLinkPath.c_str()) != 0) {
        ALOGE("Failed to create symlink \"%s\", errno=%d",
              latestSymLinkPath.c_str(), errno);
        goto skipLatestLink;
    };
    changeSymbolicPermission(latestSymLinkPath);

skipLatestLink:
    createFinishFlagFile(logDirPath + "/" + mLoggerName + ".finish");
}

double LoggerBase::getTotalElapsedSeconds() {
    return ::getElapsedSeconds(mBeginTime, mEndTime);
}

string LoggerBase::getBuildFingerprint() {
    char buf[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.build.fingerprint", buf, "");
    return string(buf);
}

string LoggerBase::getBootimageBuildFingerprint() {
    char buf[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.bootimage.build.fingerprint", buf, "");
    return string(buf);
}

bool LoggerBase::changePermission(string path, bool isDir) {
    /* return true if succeed in changing to proper permission for the path,
     * elsewise false */
    struct stat statBuf;
    if (stat(path.c_str(), &statBuf) != 0) {
        ALOGD("Path %s not exists, and would not change the permission, "
              "errno=%d.", path.c_str(), errno);
        return false;
    }

    mode_t modeBit = S_IRUSR | S_IWUSR | (isDir ? S_IXUSR : 0);
    modeBit |= S_IRGRP | (isDir ? S_IXGRP : 0);
    modeBit |= S_IROTH | (isDir ? S_IXOTH : 0);
    if (chmod(path.c_str(), modeBit) != 0) {
        ALOGD("Failed to chmod for path %s, errno=%d.", path.c_str(), errno);
        return false;
    }

    if (chown(path.c_str(), AID_SYSTEM, AID_SYSTEM) != 0) {
        ALOGD("Failed to chown to user system for path %s, errno=%d.",
              path.c_str(), errno);
        return false;
    }

    return true;
}

bool LoggerBase::changeSymbolicPermission(string path) {
    /* return true if succeed in changing to proper permission for the path,
     * elsewise false */
    struct stat statBuf;
    if (lstat(path.c_str(), &statBuf) != 0) {
        ALOGD("The symbolic link %s not exists and it would not change the"
              " permission, errno=%d.", path.c_str(), errno);
        return false;
    }

    if (lchown(path.c_str(), AID_SYSTEM, AID_SYSTEM) != 0) {
        ALOGD("Failed to lchown to user system for path %s, errno=%d.",
              path.c_str(), errno);
        return false;
    }

    return true;
}


bool LoggerBase::createDirAndChangePermission(string dirPath) {
    /* return true if the dir specified by dirPath is present */
    struct stat statBuf;
    if (stat(dirPath.c_str(), &statBuf) != 0) {
        /* dirPath not exists */
        if (mkdir(dirPath.c_str(), S_IRWXU) != 0) {
            ALOGE("Failed to create directory %s, errno=%d", dirPath.c_str(),
                  errno);
            return false;
        }
        ALOGD("Created directory %s", dirPath.c_str());
        changePermission(dirPath, true);
    }
    return true;
}

string LoggerBase::getLogDirPath() {
    if (mOverridedLogDirPath.length() > 0) {
        return mLogRootDirPath + "/" + mOverridedLogDirPath;
    } else {
        return mLogRootDirPath + "/"
               + formatRealTime(mBeginRealTime, string(LOG_DIR_TIME_FORMAT));
    }
}

bool LoggerBase::logRootDirExceedLimit() {
    /* return true if error or exceed the limit elsewise false */
#define POPEN_BUF 1024
    char buf[POPEN_BUF] = {0};
    string cmd;
    FILE *fd;

    /* total size */
    cmd = string("du -skx ") + mLogRootDirPath;
    /* du params:
     *   -s     only total size of each argument
     *   -k     1024 bytes blocks (default)
     *   -x     don't leave this filesystem
     */
    fd = popen(cmd.c_str(), "r");
    if (!fd) {
        ALOGE("Failed to popen du for the total size of log dir %s, errno=%d",
              mLogRootDirPath.c_str(), errno);
        return true;
    }
    if (fgets(buf, POPEN_BUF, fd) == NULL) {
        ALOGE("Failed to get output of du, errno=%d", errno);
        pclose(fd);
        return true;
    }
    if (strstr(buf, "No such file or directory") != NULL) {
        ALOGI("Directory %s not exists", mLogRootDirPath.c_str());
        pclose(fd);
        return false;
    }

    pclose(fd);

    off_t usedKiloBytes = 0;
    sscanf(buf, "%ld", &usedKiloBytes);
    ALOGD("The total size of %s is %ld KiB.", mLogRootDirPath.c_str(),
          usedKiloBytes);

    return (usedKiloBytes * 1024) > MAX_PERSISTENT_LOG_DIR_SIZE;
#undef POPEN_BUF
}

void LoggerBase::saveCsv(string path) {
    ofstream file(path);
    if (!file.good()) {
        ALOGE("Failed to write csv file %s, errno=%d", path.c_str(), errno);
        return;
    }

    file << "csvFormatVersion," << mCsvFormatVersion << endl;
    file << "loggerName," << mLoggerName << endl;

    file << "compactVersion," << COMPACT_VERSION << endl;
    file << "fileDefragVersion," << FILE_DEFRAG_VERSION << endl;

    file << "bootimageBuildFingerprint," << getBootimageBuildFingerprint()
         << endl;
    file << "buildFingerprint," << getBuildFingerprint() << endl;

    auto metConstraint = checkConstraint();
    file << "metConstraintNumber," << metConstraint.size() << endl;
    for (string cons : metConstraint) {
        file << "constrain," << cons << endl;
    }

    file << "beginRealTime,"
         << formatRealTime(mBeginRealTime, string(CSV_TIME_FORMAT)) << endl
         << "endRealTime,"
         << formatRealTime(mEndRealTime, string(CSV_TIME_FORMAT))
         << endl;

    file << "totalElapsedTime," << getTotalElapsedSeconds() << endl;

    file << "exitReason," << mExitReason << endl;

    this->saveCsvInner(file);

    file.close();
    changePermission(path, false);
}

void LoggerBase::saveLog(string path) {
    ofstream file(path);
    if (!file.good()) {
        ALOGE("Failed to write log file %s, errno=%d", path.c_str(), errno);
        return;
    }

    file << "The first " << MAX_BEGINNING_LOG_NR << " logs:" << endl;
    for (auto entry : mBeginningLogs) { file << entry.toString() << endl; }
    file << endl;

    file << "The last " << MAX_ENDING_LOG_NR << " logs:" << endl;
    for (auto entry : mEndingLogs) { file << entry.toString() << endl; }
    file << endl;

    file.close();
    changePermission(path, false);
}

void LoggerBase::createFinishFlagFile(string path) {
    ofstream file(path);
    if (!file.good()) {
        ALOGE("Failed to create finish flag file %s, errno=%d", path.c_str(),
              errno);
        return;
    }
    file << "This file indicates the end of " << mLoggerName
         << " logging." << endl;
    file.close();
    changePermission(path, false);
}

/* COMPACT */

#define COMPACT_CSV_FORMAT_VERSION 4
static char dirToCompact[] = "/data";

double GroupStat::getElapsedSeconds() {
    return ::getElapsedSeconds(beginTime, endTime);
}

CompactLogger::CompactLogger()
    : LoggerBase("compact", COMPACT_CSV_FORMAT_VERSION) {

    char deviceNameBuf[1024] = {0};  // be same with get_block_device()
    if (get_block_device(dirToCompact, deviceNameBuf) < 0) {
        ALOGE("Failed to get_block_device");
        memset(deviceNameBuf, 0, sizeof(deviceNameBuf) / sizeof(char));
    }
    mBlockDevicePath = string(deviceNameBuf);
}

void CompactLogger::traceBegin() { LoggerBase::traceBegin(); }

void CompactLogger::traceGroupBegin(int groupNo) {
    mTracingInGroup = true;

    MbGroupsEntry entry = readMbGroupsByGroupNo(groupNo);
    mPendingGroupStat = {
        .groupNo = groupNo,
        .freeBlockCountBefore = entry.free,
        .freeBlockCountAfter = 0,
        .fragCountBefore = entry.frags,
        .fragCountAfter = 0,
        .movedBlockNr = 0,
        .fsWrittenBefore = readFsLifetimeWriteKBytes(),
        .fsWrittenAfter = 0,
        .beginRealTime = chrono::system_clock::now(),
        .endRealTime = chrono::system_clock::time_point(),
        .beginTime = chrono::steady_clock::now(),
        .endTime = chrono::steady_clock::time_point(),
    };
}

void CompactLogger::traceGroupEnd(int groupNo, int movedBlockNr) {
    mTracingInGroup = false;

    MbGroupsEntry entry = readMbGroupsByGroupNo(groupNo);

    GroupStat &p = mPendingGroupStat;
    p.freeBlockCountAfter = entry.free;
    p.fragCountAfter = entry.frags;
    p.movedBlockNr = movedBlockNr;
    p.fsWrittenAfter = readFsLifetimeWriteKBytes();
    p.endRealTime = chrono::system_clock::now();
    p.endTime = chrono::steady_clock::now();

    mGroupStats.push_back(p);
}

void CompactLogger::traceEnd(__u64 totalIcheckNum, __u64 icheckHitNum,
                             __u64 icheckLeftNum, __u64 icheckRightNum,
                             __u64 icheckCacheHitNum,
                             __u64 totalNcheckNum, __u64 ncheckHitNum,
                             __u64 ncheckLeftNum, __u64 ncheckRightNum,
                             __u64 ncheckCacheHitNum) {

	LoggerBase::traceEnd();

    mTotalIcheckNum = totalIcheckNum;
    mIcheckHitNum = icheckHitNum;
    mIcheckLeftNum = icheckLeftNum;
    mIcheckRightNum = icheckRightNum;
    mIcheckCacheHitNum = icheckCacheHitNum;
    mTotalNcheckNum = totalNcheckNum;
    mNcheckHitNum = ncheckHitNum;
    mNcheckLeftNum = ncheckLeftNum;
    mNcheckRightNum = ncheckRightNum; mNcheckCacheHitNum = ncheckCacheHitNum;
}

string CompactLogger::getBlockDeviceName() {
    return mBlockDevicePath.substr(mBlockDevicePath.find_last_of('/') + 1,
                                   mBlockDevicePath.length());
}

void CompactLogger::saveCsvInner(ofstream &file) {
    // statfs
    struct statfs sf;
    if (statfs(dirToCompact, &sf) == 0) {
        file << "comment," << "errno" << "," << "f_type" << ","
             << "f_bsize" << "," << "f_blocks" << "," << "f_bfree" << ","
             << "f_bavail" << "," << "f_files" << "," << "f_ffree" << ","
             << "f_namelen" << "," << "f_frsize" << "," << "f_flags" << endl;
        file << "statfs," << 0 << "," << sf.f_type << ","
             << sf.f_bsize << "," << sf.f_blocks << "," << sf.f_bfree << ","
             << sf.f_bavail << "," << sf.f_files << "," << sf.f_ffree << ","
             << sf.f_namelen << "," << sf.f_frsize << "," << sf.f_flags
             << endl;
    } else {
        ALOGE("Failed to statfs path %s, errno=%d", dirToCompact, errno);
        file << "comment,errno" << endl;
        file << "statfs," << errno << endl;
    }

    // icheck statistics
    file << "comment," << "totalIcheckNum" << "," << "icheckHitNum" << ","
         << "icheckLeftNum" << "," << "icheckRightNum" << ","
         << "icheckCacheHitNum" << endl;
    file << "icheck," << mTotalIcheckNum << "," << mIcheckHitNum << ","
         << mIcheckLeftNum << "," << mIcheckRightNum << ","
         << mIcheckCacheHitNum << endl;

    // ncheck statistics
    file << "comment," << "totalNcheckNum" << "," << "ncheckHitNum" << ","
         << "ncheckLeftNum" << "," << "ncheckRightNum" << ","
         << "ncheckCacheHitNum" << endl;
    file << "ncheck," << mTotalNcheckNum << "," << mNcheckHitNum << ","
         << mNcheckLeftNum << "," << mNcheckRightNum << ","
         << mNcheckCacheHitNum << endl;

    // group statistics
    file << "groupNumber," << mGroupStats.size() << endl;
    file << "comment," << "groupNo,"
         << "freeBlockCountBefore," << "freeBlockCountAfter,"
         << "fragCountBefore," << "fragCountAfter,"
         << "movedBlockNr,"
         << "fsWrittenBefore," << "fsWrittenAfter,"
         << "beginRealTime," << "beginEndTime,"
         << "getElapsedSeconds()"
         << endl;
    for (auto g : mGroupStats) {
        file << "group," << g.groupNo << ','
             << g.freeBlockCountBefore << ',' << g.freeBlockCountAfter << ','
             << g.fragCountBefore << ',' << g.fragCountAfter << ','
             << g.movedBlockNr << ','
             << g.fsWrittenBefore << ',' << g.fsWrittenAfter << ','
             << formatRealTime(g.beginRealTime, string(CSV_TIME_FORMAT)) << ','
             << formatRealTime(g.endRealTime, string(CSV_TIME_FORMAT)) << ','
             << g.getElapsedSeconds() << endl;
    }
}

bool CompactLogger::readMbGroups(MbGroupsResult &result) {
    string path = string("/proc/fs/ext4/") + getBlockDeviceName()
                  + "/mb_groups";

    ifstream file(path);
    if (!file.good()) {
        ALOGE("Failed to read mb_groups %s, errno=%d", path.c_str(), errno);
        file.close();
        return false;
    }

    string line;
    getline(file, line);  // skip the header line
    while (getline(file, line)) {
        MbGroupsEntry entry;
        sscanf(line.c_str(), "#%d%*[ ]:%*[ ]%d%*[ ]%d%*[ ]%d",
               &entry.groupNo, &entry.free, &entry.frags, &entry.first);
        result.push_back(entry);
    }
    file.close();
    return true;
}

MbGroupsEntry CompactLogger::readMbGroupsByGroupNo(int groupNo) {
    MbGroupsResult result;
    if (!readMbGroups(result)) {
        ALOGE("Failed to call readMbGroups().");
        return MbGroupsEntry();
    }
    auto it = find_if(result.begin(), result.end(),
                      [&](auto &e) { return e.groupNo == groupNo; });
    if (it == result.end()) {
        ALOGE("Can't find entry by readMbGroupsByGroupNo(groupNo=%d).",
               groupNo);
        return MbGroupsEntry();
    }
    return *it;
}

unsigned long long CompactLogger::readFsLifetimeWriteKBytes() {
    string path = string("/sys/fs/ext4")
                  + "/" + getBlockDeviceName() + "/lifetime_write_kbytes";
    return readFileAs<unsigned long long>(path);
}

vector<string> CompactLogger::checkConstraint() {
    vector<string> r;

    if ((int)this->mGroupStats.size() < this->mMaxSearchGroupNr
            && mExitReason != "stop"
            && mExitReason != "user_cancel"
            // not startswith "no_group_need_be_defrag_"
            && mExitReason.find("no_group_need_be_defrag_") != 0) {
        r.push_back("groupNr_1");
    }

    if ((int)this->mGroupStats.size() > this->mMaxSearchGroupNr) {
        auto didCompactGroupNr = count_if(
            this->mGroupStats.begin(), this->mGroupStats.end(),
            [](auto gs) { return gs.fragCountAfter - gs.fragCountBefore > 0; }
        );
        if (didCompactGroupNr > this->mMaxSearchGroupNr) {
            r.push_back("groupNr_2");
        }
    }

    return r;
}

/* FILE DEFRAG */

#define FILE_DEFRAG_CSV_FORMAT_VERSION 2
#define MAX_FILE_STAT_NR 50

FileStat::FileStat(string path) : path(path) {
    if (path.length() > 0) {
        // get file size
        struct stat sb;
        if (stat(path.c_str(), &sb) != 0) {
            ALOGW("Failed to stat file %s, errno=%d", path.c_str(), errno);
        } else {
            this->size = sb.st_size;
        }
    }
}

double FileStat::getElapsedSeconds() {
    return ::getElapsedSeconds(beginTime, endTime);
}

FileDefragLogger::FileDefragLogger()
    : LoggerBase("filedefrag", FILE_DEFRAG_CSV_FORMAT_VERSION),
      mTracingInFile(false),
      mFileStats(LimitedSizeHeap(MAX_FILE_STAT_NR)) { }

void FileDefragLogger::traceBegin() { LoggerBase::traceBegin(); }

void FileDefragLogger::traceFileBegin(string path) {
    mTracingInFile = true;
    mPendingFileStat = FileStat(path);
    mPendingFileStat.beginRealTime = chrono::system_clock::now();
    mPendingFileStat.beginTime = chrono::steady_clock::now();
}

void FileDefragLogger::traceFileEnd(int extentNrBefore, int extentNrAfter) {
    mTracingInFile = false;

    FileStat &f = mPendingFileStat;
    f.endRealTime = chrono::system_clock::now();
    f.endTime = chrono::steady_clock::now();
    f.extentNrBefore = extentNrBefore;
    f.extentNrAfter = extentNrAfter;

    if (extentNrAfter >= 0) {
        mFileStats.push(f);
    }
}

void FileDefragLogger::traceEnd(
        int totalExtentsNrBefore, int totalExtentsNrAfter,
        long *gDefragStat, long *gDonorCallDefragEstat) {
    LoggerBase::traceEnd();

    mTotalExtentsNrBefore = totalExtentsNrBefore;
    mTotalExtentsNrAfter = totalExtentsNrAfter;

    mpDefragStat = gDefragStat;
    mpDonorCallDefragEstat = gDonorCallDefragEstat;
}

void FileDefragLogger::saveCsvInner(ofstream &file) {
    // total extents number
    file << "comment" << ","
         << "totalExtentsNrBefore" << "," << "totalExtentsNrAfter" << endl;
    file << "totalExtentsNr" << ","
         << mTotalExtentsNrBefore << "," << mTotalExtentsNrAfter << endl;

    // defrag stat (see filedefrag.h)
    static const string defragStatKeys[] = {
        "FILE_REG", "FILE_SIZE_ZERO", "FILE_BLOCKS_ZERO", "FILE_OPEN_FAIL",
        "FILE_GET_EXT", "FILE_CHG_PHY_TO_LOG", "FILE_CHECK", "FILE_SYNC",
        "DONOR_FILE_JOIN_EXT", "DONOR_FILE_OPEN", "DONOR_FILE_UNLINK",
        "DONOR_FILE_FALLOCATE", "DONOR_FILE_GET_EXT",
        "DONOR_FILE_CHG_PHY_TO_LOG", "DONOR_DEFRAG_OK", "DONOR_NO_DEFRAG_OK",
        "DONOR_DEFRAG_FAIL"
    };
    file << "comment";
    for (auto key : defragStatKeys) { file << "," << key; }
    file << endl;
    file << "defragStat";
    for (size_t i = 0; i < sizeof(defragStatKeys) / sizeof(string); i++) {
        if (mpDefragStat != nullptr) {
            file << "," << mpDefragStat[i];
        } else {
            file << ",null";
        }
    }
    file << endl;

    // donor call defrag estat (see filedefrag.h)
    static const string donorCallDefragEstatKeys[] = {
        "DFILE_PAGE_IN_CORE", "DFILE_FREE_PAGES", "DFILE_IOC_MOVE_EXT"
    };
    file << "comment";
    for (auto key : donorCallDefragEstatKeys) { file << "," << key; }
    file << endl;
    file << "donorCallDefragEstat";
    for (size_t i = 0; i < sizeof(donorCallDefragEstatKeys) / sizeof(string);
         i++) {
        if (mpDonorCallDefragEstat != nullptr) {
            file << "," << mpDonorCallDefragEstat[i];
        } else {
            file << ",null";
        }
    }
    file << endl;

    // file stats
    file << "fileNumber," << mFileStats.size() << endl;
    file << "comment" << "," << "path" << "," << "size" << ","
         << "extentNrBefore" << "," << "extentNrAfter" << ","
         << "beginRealTime" << "," << "endRealTime" << ","
         << "getElapsedSeconds()" << endl;
    for (auto fs : mFileStats.sort()) {
        file << "file" << "," << fs.path << "," << fs.size << ","
             << fs.extentNrBefore << "," << fs.extentNrAfter << ","
             << formatRealTime(fs.beginRealTime, string(CSV_TIME_FORMAT)) << ','
             << formatRealTime(fs.endRealTime, string(CSV_TIME_FORMAT)) << ','
             << fs.getElapsedSeconds() << endl;
    }
}

LogLevel parseLogLevelArg(char *arg) {
    string args(arg);
    transform(args.begin(), args.end(), args.begin(), ::tolower);
    if (args == "0") {
        return INFO;
    } else if (args == "1") {
        return DEBUG;
    } else if (args == "2") {
        return VERBOSE;
    }
    return INFO;
}
