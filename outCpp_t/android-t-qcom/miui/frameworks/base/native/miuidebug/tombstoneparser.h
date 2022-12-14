#ifndef _MIUINDBG_TOMBSTONE_PARSER_H_
#define _MIUINDBG_TOMBSTONE_PARSER_H_
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <utils/String8.h>
#include <utils/Vector.h>

using namespace android;

class TombstoneParser {
public:
    TombstoneParser(pid_t pid, pid_t tid = 0);
    void attachFile();
    void parseHeader();
    bool parseThreadInfo(char* line);
    bool parseAbortMessage(char* line);
    bool parseSignalInfo(char* line);
    bool parseBacktrace(char* line, int length, FILE* fp);
    pid_t getPid() const { return mPid; }
    pid_t getTid() const{ return mTid; }

    bool isSystemApp() const { return mSystemApp; }
    const String8& getTheadName() const { return mThreadName; }
    const String8& getProcessName() const { return mProcessName; }
    const String8& getSigNumStr() const { return mSigNumStr; }
    const String8& getAbortMsg() const { return mAbortMsg; }
    const String8& getTombFilePath() const { return mTombstonePath; }
    const Vector<String8>& getBacktraces() const { return mBacktraces; }
    void print() const;

private:
    pid_t   mPid;
    pid_t   mTid;
    bool    mSystemApp;
    String8 mThreadName;
    String8 mProcessName;
    String8 mTombstonePath;
    int     mSigNum;
    int     mSigCode;
    String8 mSigNumStr;
    String8 mSigCodeStr;
    String8 mAbortMsg;
    Vector<String8> mBacktraces;
#define TOMBSTONE_DIR       "/data/tombstones"
#define TOMBSTONE_PREFIX    "tombstone_"
#define __CHECK(_condition,_value,_ret) \
            do { \
                if ((_condition) == _value) { \
                    printf("check false line=%d\n",__LINE__); \
                    return _ret; \
                } \
            }while (0)
};

#endif
