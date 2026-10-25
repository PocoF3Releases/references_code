/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _FREEFRAG_H_
#define _FREEFRAG_H_
#include <utils/String8.h>
#include <mutex>

namespace android {

class FreeFrag {
public:
    static void getStats(String8& fragInfo);
    static void defragDataPartition();
    static void stopDefrag();
    static void execProcess(pid_t *pid, const char *kbinPath, const char *bin, char *const args[]);
    static void recoverStopFlag();
    static int RunStorageCompact();
    static int RunFileDefrag();

private:
    static std::mutex pidMutex;
    static pid_t compactPid;
    static pid_t defragPid;
    static int stopFlag;
};

};//namespace android

#endif /* _FREEFRAG_H_ */
