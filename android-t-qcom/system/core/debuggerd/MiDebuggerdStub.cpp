#include <iostream>
#include <dlfcn.h>
#include <string.h>
#include <log/log.h>
#include <async_safe/log.h>
#include <android-base/properties.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "MiDebuggerdStub.h"
#include "util.h"
#define STUB_LOG_TAG "MiDebuggerdStub"
IMiDebuggerdStub* MiDebuggerdStub::ImplInstance = NULL;
void* MiDebuggerdStub::LibImpl = NULL;
std::mutex MiDebuggerdStub::StubLock;
std::string MiDebuggerdStub::GetLibPath() {
    std::string processName = get_process_name(getpid());
    if (processName.find("/vendor/") == 0 || processName.find("/odm/") == 0) {
        return LIBPATH(vendor);
    } else {
        return LIBPATH(system);
    }
}
IMiDebuggerdStub* MiDebuggerdStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (!ImplInstance) {
        std::string libPath = GetLibPath();
        LibImpl = dlopen(libPath.c_str(), RTLD_NOW);
        if (LibImpl) {
            Create* create = (Create*)dlsym(LibImpl, "create");
            ImplInstance = create();
        } else {
            async_safe_format_log(ANDROID_LOG_INFO, STUB_LOG_TAG, "dlopen %s failed %s", libPath.c_str(), dlerror());
        }
    }
    return ImplInstance;
}
void MiDebuggerdStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibImpl) {
        Destroy* destroy = (Destroy*)dlsym(LibImpl, "destroy");
        destroy(ImplInstance);
        dlclose(LibImpl);
        LibImpl = NULL;
        ImplInstance = NULL;
    }
}
void MiDebuggerdStub::ptraceThreadsForUnreachable(pid_t ppid, pid_t crashing_tid, int output_pipe_fd, DebuggerdDumpType dump_type) {
    if(!IsEnableUnreachable(ppid,output_pipe_fd))
        return;
    IMiDebuggerdStub* Istub = GetImplInstance();
    if(Istub)
        Istub->ptraceThreadsForUnreachable(ppid, crashing_tid, output_pipe_fd, dump_type);
}
void MiDebuggerdStub::dumpUnreachableMemory(int input_read_fd) {
    if(!IsEnableUnreachableByPipe(input_read_fd))
        return;
    pid_t dumppid = clone(nullptr, nullptr, CLONE_VM | CLONE_FS | CLONE_FILES, nullptr);
    if (dumppid == -1) {
        async_safe_format_log(ANDROID_LOG_FATAL, STUB_LOG_TAG, "fork failed");
    } else if(dumppid == 0) {
        IMiDebuggerdStub* Istub = GetImplInstance();
        if(Istub)
            Istub->dumpUnreachableMemory(input_read_fd);
        _exit(0);
    }
    int status;
    if (TEMP_FAILURE_RETRY(waitpid(dumppid, &status, __WCLONE)) != dumppid) {
        async_safe_format_log(ANDROID_LOG_FATAL, STUB_LOG_TAG,"failed to waitpid for unreachable process");
    } else if (!WIFEXITED(status)) {
        async_safe_format_log(ANDROID_LOG_FATAL, STUB_LOG_TAG,"unreachable process didn't exit (status = %d)", status);
    } else if (WEXITSTATUS(status)) {
        async_safe_format_log(ANDROID_LOG_FATAL, STUB_LOG_TAG,"unreachable process exited with non-zero status: %s", strerror(WEXITSTATUS(status)));
    }
}
bool MiDebuggerdStub::IsEnableUnreachable(pid_t ppid, int output_pipe_fd) {
    std::string cmdline = get_process_name(ppid);
    std::string enableProcess =  android::base::GetProperty(UNREACHABLE_PROCESS_PROPERTY, "");
    std::string processName = "";
    size_t pos = cmdline.rfind("/");
    if (pos != std::string::npos) {
        processName = cmdline.substr(pos + 1);
    }
    auto writeOutputPipe = [&](const char* content) {
        if (TEMP_FAILURE_RETRY(write(output_pipe_fd, content, 1)) != 1) {
            async_safe_format_log(ANDROID_LOG_FATAL, STUB_LOG_TAG,"failed to write pipe");
        }
    };
    if (enableProcess == "" || processName == "" ||
        enableProcess.find(processName) == std::string::npos) {
        writeOutputPipe(UNREACHABLE_NOT_ENABLE_FLAG);
        return false;
    }
    writeOutputPipe(UNREACHABLE_ENABLE_FLAG);
    return true;
}
bool MiDebuggerdStub::IsEnableUnreachableByPipe(int input_read_fd) {
    char buf[4];
    int rc = TEMP_FAILURE_RETRY(read(input_read_fd, &buf, sizeof(buf)));
    return rc == 1 && buf[0] == UNREACHABLE_ENABLE_FLAG[0];
}
