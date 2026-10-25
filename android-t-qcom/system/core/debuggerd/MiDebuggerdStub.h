#ifndef ANDROID_MIDEBUGGERDSTUB_H
#define ANDROID_MIDEBUGGERDSTUB_H
#include <mutex>
#include "IMiDebuggerdStub.h"
#define LIBPATH(dir) #dir"/lib64/libmidebuggerdimpl_"#dir".so"
#define UNREACHABLE_PROCESS_PROPERTY "debuggerd.unreachable.process.name"
#define UNREACHABLE_ENABLE_FLAG "t"
#define UNREACHABLE_NOT_ENABLE_FLAG "f"
class MiDebuggerdStub
{
private:
    static void* LibImpl;
    static IMiDebuggerdStub* ImplInstance;
    static IMiDebuggerdStub* GetImplInstance();
    static void DestroyImplInstance();
    static std::mutex StubLock;
    static std::string GetLibPath();
    static bool IsEnableUnreachable(pid_t ppid, int output_pipe_fd);
    static bool IsEnableUnreachableByPipe(int input_read_fd);
public:
    virtual ~MiDebuggerdStub() {}
    static void dumpUnreachableMemory(int input_read_fd);
    static void ptraceThreadsForUnreachable(pid_t ppid, pid_t crashing_tid, int output_pipe_fd, DebuggerdDumpType dump_type);
};
#endif
