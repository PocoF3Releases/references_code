#ifndef ANDROID_MIDEBUGGERDIMPL_H
#define ANDROID_MIDEBUGGERDIMPL_H

#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <IMiDebuggerdStub.h>
#include "common/include/dump_type.h"

class MiDebuggerdImpl:public IMiDebuggerdStub {
public:
    virtual ~MiDebuggerdImpl() {}
    virtual void ptraceThreadsForUnreachable(pid_t ppid, pid_t crashing_tid, int output_pipe_fd, DebuggerdDumpType dump_type);
    virtual void dumpUnreachableMemory(int input_read_fd);
};

extern "C" IMiDebuggerdStub* create();
extern "C" void destroy(IMiDebuggerdStub* impl);

#endif