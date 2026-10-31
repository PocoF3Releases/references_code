#include <stdlib.h>
#include <vector>
#include <string>
#include <dlfcn.h>
#include <android-base/stringprintf.h>
#include <android-base/properties.h>
#include <android-base/file.h>
#include <memunreachable/memunreachable.h>
#include <LeakPipe.h>

#include "MiDebuggerdImpl.h"
#include "Mimemunreachable.h"
#include "log.h"

#define UNREACHABLE_ENABLE_FLAG "t"
#define UNREACHABLE_NOT_ENABLE_FLAG "f"

void MiDebuggerdImpl::dumpUnreachableMemory(int input_read_fd) {
    char buf[1];

    int rc = TEMP_FAILURE_RETRY(read(input_read_fd, &buf, sizeof(buf)));
    if (rc == 1 && buf[0] == UNREACHABLE_ENABLE_FLAG[0]) {
        pid_t pid = getppid();
        android::UnreachableMemoryInfo info;
        android::GetUnreachableMemoryWithoutPtrace(info, pid, input_read_fd, 100);
        std::string leakMemInfo  =  info.ToString(true);
        LOGI("pid:%d unreachable memory info:\n%s", pid, leakMemInfo.c_str());
    }
}

void MiDebuggerdImpl::ptraceThreadsForUnreachable(pid_t ppid, pid_t crashing_tid, int output_pipe_fd, DebuggerdDumpType dump_type) {
    auto writeOutputPipe = [&](const char* content) {
        if (TEMP_FAILURE_RETRY(write(output_pipe_fd, content, 1)) != 1) {
            LOGE("failed to write to pseudothread");
        }
    };

    if (dump_type != kDebuggerdNativeBacktrace) {
        writeOutputPipe(UNREACHABLE_NOT_ENABLE_FLAG);
        return;
    }

    writeOutputPipe(UNREACHABLE_ENABLE_FLAG);
    LOGI("unreachable ptrace threads for %d", ppid);

    android::PtraceThreads((int)ppid, (int)crashing_tid, output_pipe_fd);
}

extern "C" IMiDebuggerdStub* create() {
    return new MiDebuggerdImpl;
}

extern "C" void destroy(IMiDebuggerdStub* impl) {
    delete impl;
}
