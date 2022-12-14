#define LOG_TAG "MIUINDBG_TOMB_PARSER"
#include <dirent.h>
#include <sys/stat.h>
#include <private/android_filesystem_config.h>
#include "tombstoneparser.h"
#include "ndbglog.h"

TombstoneParser::TombstoneParser(pid_t pid, pid_t tid) {
    char path[32];
    struct stat stats;

    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (stat(path, &stats)) {
        MILOGE("stat err %s errno=%d",path,errno);
        return;
    }

    if (stats.st_uid < AID_APP) {
        mSystemApp == true;
    } else {
        mSystemApp == false;
    }
    mPid = pid;
    mTid = tid;

    attachFile();
    parseHeader();
}

void TombstoneParser::attachFile() {
    DIR *dir;
    struct dirent *dentry;
    char path[PATH_MAX];

    dir = opendir(TOMBSTONE_DIR);
    if (!dir) return;

    long latestMonifyTime = 0;
    char* fileName = NULL;

    while ((dentry = readdir(dir))) {
        if (dentry->d_type == DT_DIR || strncmp(dentry->d_name, TOMBSTONE_PREFIX, strlen(TOMBSTONE_PREFIX))) {
            continue;
        }
        struct stat sb;
        snprintf(path, sizeof(path), TOMBSTONE_DIR"/%s", dentry->d_name);
        if (!stat(path, &sb)) {
            if ((long)sb.st_mtime >= latestMonifyTime) {
                latestMonifyTime = sb.st_mtime;
                if (fileName) {
                    free(fileName);
                }
                fileName = strdup(dentry->d_name);
            }
        }
    }
    closedir(dir);

    if (!fileName) return;

    char threadInfo[64];
    if (mTid != 0) {
        snprintf(threadInfo,sizeof(threadInfo),"pid: %d, tid: %d",mPid, mTid);
    } else {
        snprintf(threadInfo,sizeof(threadInfo),"pid: %d, tid: ",mPid);
    }
    MILOGI("thread info %s",threadInfo);
    dir = NULL;

    do {
        if (dir == NULL) {
            snprintf(path, sizeof(path), TOMBSTONE_DIR"/%s", fileName);
        } else {
            if (dentry->d_type == DT_DIR
                || strncmp(dentry->d_name, TOMBSTONE_PREFIX, strlen(TOMBSTONE_PREFIX))
                || !strcmp(fileName,dentry->d_name)){
                continue;
            }
            snprintf(path, sizeof(path), TOMBSTONE_DIR"/%s", dentry->d_name);
        }

        FILE *fp = fopen(path, "r");
        if (fp != NULL) {
            char line[1024];
            int count = 10;
            while (count-- && fgets(line, sizeof(line), fp)) {
                if (!strncmp(line, threadInfo, strlen(threadInfo))){
                    if (mTid != 0) {
                        mTombstonePath = path;
                        break;
                    } else {
                        char thread[PATH_MAX];
                        int tid = atoi(line+strlen(threadInfo));

                        snprintf(thread, sizeof(thread),"/proc/%d/task/%d",mPid,tid);
                        if (!access(thread,F_OK)) {
                            mTombstonePath = path;
                            mTid = tid;
                            break;
                        }
                    }
                }
            }
            fclose(fp);
            if (!mTombstonePath.isEmpty()) {
                break;
            }
        }

        if (dir == NULL) {
            dir = opendir(TOMBSTONE_DIR);
        }
    }while (dir && (dentry = readdir(dir)));

    free(fileName);

    if (dir) {
        closedir(dir);
    }
}

bool TombstoneParser::parseThreadInfo(char* line) {
    if (!mProcessName.isEmpty()) {
        return true;
    } else if (strncmp(line, "pid: ", sizeof("pid: ") - 1)) {
        return false;
    }

    char *nameTag,*threadName,*procName,*lb,*rb;
    __CHECK(nameTag = strstr(line, ", name: "), NULL, false);
    threadName = nameTag + sizeof(", name: ") - 1;
    __CHECK(lb = strstr(threadName, "  >>> "), NULL, false);
    procName = lb + sizeof("  >>> ") - 1;
    __CHECK(rb = strstr(procName, " <<<"), NULL, false);

    *lb = *rb = 0;
    mThreadName = threadName;
    mProcessName = procName;
    return true;
}

bool TombstoneParser::parseAbortMessage(char* line) {
    if (!strncmp(line, "Abort message: ",15)) {
        mAbortMsg = (line + 15);
        return true;
    }
    return false;
}

bool TombstoneParser::parseSignalInfo(char* line) {
    if (!mSigNumStr.isEmpty()) {
        return true;
    } else if (strncmp(line, "signal ", sizeof("signal ")-1)) {
        return false;
    }

    char *flb,*frb,*slb,*srb;
    char *sigTag,*sigNum,*sigStr,*codeTag,*CodeNum,*codeStr;

    /*
    tombstone_00:
    signal 7 (SIGBUS), code 2 (BUS_ADRERR), fault addr 0x7f6377caab

    tombstone.c:
    _LOG(log, logtype::HEADER, "signal %d (%s), code %d (%s), fault addr %s\n",
         signal, get_signame(signal), si.si_code, get_sigcode(signal, si.si_code), addr_desc);
    */

    __CHECK(sigTag = strstr(line,"signal "),NULL,false);
    sigNum = sigTag + sizeof("signal ")-1;
    __CHECK(flb = strstr(sigNum," ("),NULL,false);
    sigStr = flb + sizeof(" (") - 1;
    __CHECK(frb = strstr(sigStr,"), "),NULL,false);
    __CHECK(codeTag = strstr(frb,"code "),NULL,false);
    CodeNum = codeTag + sizeof("code ") - 1;
    __CHECK(slb = strstr(CodeNum," ("),NULL,false);
    codeStr = slb + sizeof(" (") - 1;
    __CHECK(srb = strstr(codeStr,"), "),NULL,false);

    *flb = *frb = *slb = *srb = 0;
    mSigNum = atoi(sigNum);
    mSigCode = atoi(CodeNum);
    mSigNumStr = sigStr;
    mSigCodeStr = codeStr;
    return true;
}

bool TombstoneParser::parseBacktrace(char* line, int length, FILE* fp) {
    if (!mBacktraces.isEmpty()) {
        return true;
    } else if (strncmp(line, "backtrace:", sizeof("backtrace:") - 1)) {
        return false;
    }
    while (fgets(line, length, fp)) {
        if (strncmp(line, "    #", 5)) {
            break;
        }
        mBacktraces.push(String8(line));
    }
    return true;
}

void TombstoneParser::parseHeader() {
    if (mTombstonePath.isEmpty()) {
        return;
    }

    FILE *fp = fopen(mTombstonePath.string(), "r");
    if (fp == NULL) return;

    char buffer[1024];
    int lineNum = 0;

    while (lineNum++  < 100 && fgets(buffer, sizeof(buffer), fp)) {
        if (!parseThreadInfo(buffer)) {
            continue;
        }
        if (mSigNumStr.isEmpty()) {
            if (!parseSignalInfo(buffer)) {
                continue;
            }
            if (!strcmp("SIGABRT", mSigNumStr.string())) {
                if (fgets(buffer, sizeof(buffer), fp)) {
                    parseAbortMessage(buffer);
                }
            }
        }
        if (parseBacktrace(buffer, 1024, fp)) {
            break;
        }
    }
    fclose(fp);

    print();
}

void TombstoneParser::print() const {
    if (mTombstonePath.isEmpty()) {
        MILOGI("no matched tombstone!");
        return;
    }
    MILOGI("pid: %d, tid: %d, name: %s  >>> %s <<<", mPid, mTid,
        mThreadName.string(), mProcessName.string());
    MILOGI("signal %d (%s), code %d (%s)",mSigNum,mSigNumStr.string(),mSigCode,mSigCodeStr.string());
    if (!mAbortMsg.isEmpty()) {
        MILOGI("Abort message: %s", mAbortMsg.string());
    }
    MILOGI("backtrace:");
    for (size_t i = 0; i < mBacktraces.size(); i++) {
        MILOGI("    #%02zu %s",i,mBacktraces.itemAt(i).string());
    }
}

