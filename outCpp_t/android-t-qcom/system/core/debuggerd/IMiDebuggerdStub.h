#ifndef ANDROID_IMIDEBUGGERDSTUB_H
#define ANDROID_IMIDEBUGGERDSTUB_H
#include "common/include/dump_type.h"
class IMiDebuggerdStub
{
public:
    virtual ~IMiDebuggerdStub() {}
    virtual void ptraceThreadsForUnreachable(pid_t ppid, pid_t crashing_tid, int output_pipe_fd, DebuggerdDumpType dump_type);
    virtual void dumpUnreachableMemory(int input_read_fd);
};
typedef IMiDebuggerdStub* Create();
typedef void Destroy(IMiDebuggerdStub *);
#endif
