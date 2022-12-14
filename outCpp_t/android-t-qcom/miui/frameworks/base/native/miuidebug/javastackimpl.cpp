#include "ndbglog.h"

#ifdef JAVA_STACK_IMPL_FOR_KK
#include "Dalvik.h"
extern "C" void dump_current_thread(){
    DebugOutputTarget target;

    dvmCreateLogOutputTarget(&target, ANDROID_LOG_INFO, LOG_TAG);
    dvmPrintDebugMessage(&target, "DALVIK CURRENT THREAD:\n");

    Thread* self = dvmThreadSelf();
    if (self) {
        ThreadStatus oldstatus = self->status;
        self->status = THREAD_TIMED_WAIT;
        dvmDumpThreadEx(&target, self, false);
        self->status = oldstatus;
    } else {
        dvmPrintDebugMessage(&target, "NATIVE THREAD!\n");
    }
}

extern "C" void dump_all_threads(){
    DebugOutputTarget target;

    dvmCreateLogOutputTarget(&target, ANDROID_LOG_INFO, LOG_TAG);
    dvmPrintDebugMessage(&target, "DALVIK ALL THREADS:\n");

    Thread* thread = gDvm.threadList;
    ThreadStatus oldstatus;
    while (thread != NULL) {
        oldstatus = thread->status;
        dvmPrintDebugMessage(&target, "status=%d,tmpstatus=%d:\n",oldstatus,THREAD_TIMED_WAIT);
        thread->status = THREAD_TIMED_WAIT;
        dvmDumpThreadEx(&target, thread, false);
        thread->status = oldstatus;
        thread = thread->next;
    }
}

extern "C" void trim_core_dump(uintptr_t dump_java) {
    return;
}

#else   /*JAVA_STACK_IMPL_FOR_KK*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <sstream>
#if PLATFORM_SDK_VERSION >= 26
#define ART_DEFAULT_GC_TYPE_IS_CMS  1
#define ART_RUNTIME_BASE_MEMORY_TOOL_H_
#endif
#include "thread_list.h"
#include "thread-inl.h"

using namespace art;

extern "C" void dump_current_thread(){
    std::ostringstream ss;
    Thread* self = Thread::Current();

    if (self == nullptr) {
        ss << "(Aborting thread was not attached to runtime!)\n";
    } else {
        Thread::DumpState(ss, self, self->GetTid());
        self->DumpJavaStack(ss);
    }

    LOG(INFO) << ss.str();
}

#if 0
extern "C" void dump_all_threads(){
    std::ostringstream ss;
    Runtime* runtime = Runtime::Current();
    if (runtime != nullptr) {
        ThreadList* thread_list = runtime->GetThreadList();
        if (thread_list != nullptr) {
            for (const auto& thread : thread_list->GetList()) {
                Thread::DumpState(ss, thread, thread->GetTid());
                thread->DumpJavaStack(ss);
            }
        }
    } else {
        ss << "(Aborting process was not attached to runtime!)\n";
    }

    LOG(INFO) << ss.str();
}
#endif


typedef enum {
    TRIM_TYPE_RESERVED = (1 << 0),
    TRIM_TYPE_JAVA     = (1 << 1),
    TRIM_TYPE_LIB_CODE = (1 << 2),
    TRIM_TYPE_FONT     = (1 << 3),
    TRIM_TYPE_USR      = (1 << 4),
    TRIM_TYPE_FOR_FULL_DUMP = (TRIM_TYPE_RESERVED|TRIM_TYPE_LIB_CODE|TRIM_TYPE_FONT|TRIM_TYPE_USR),
    TRIM_TYPE_MAX
} TRIM_TYPE;


static void skip_segment(uintptr_t start, uintptr_t end) {
    if (end > start) {
        int rc = madvise((void*)start, end - start, MADV_DONTDUMP);
        if (rc == -1) {
            MILOGI("madvise err! start=%p, end=%p, errno=%d",(void*)start, (void*)end, errno);
        }
    }
}

static int permision_c_to_i(const char* ptr){
    int perm = 0;
    if (ptr[0] == 'r')
        perm |= 0x4;
    if (ptr[1] == 'w')
        perm |= 0x2;
    if (ptr[2] == 'x')
        perm |= 0x1;
    return perm;
}

static void trim_core(uintptr_t trim_type) {
    FILE* maps = fopen("/proc/self/maps", "r");
    if (maps == nullptr) {
        return;
    }
    char line[4096];
    while (fgets(line, sizeof(line), maps)) {
        uintptr_t start, end;
        int name_pos;
        char perms[5];

        if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR " %4s %*x %*x:%*x %*d %n", &start, &end, perms, &name_pos) == 3) {
            const char* name = &line[name_pos];
            int perm = permision_c_to_i(perms);

#define CHECK_TRIM(_trim, _perm, _name) \
    do { \
        if ((trim_type & _trim) == _trim) { \
            if ((_perm == 0xf || perm == _perm) && strncmp(name, _name, sizeof(_name)-1) == 0) { \
                skip_segment(start, end); \
                continue; \
            } \
        } \
    } while(0)
            CHECK_TRIM(TRIM_TYPE_JAVA,     0xf, "/dev/ashmem/dalvik");
            CHECK_TRIM(TRIM_TYPE_JAVA,     0xf, "/data/dalvik-cache/");
            CHECK_TRIM(TRIM_TYPE_RESERVED, 0x0, "/dev/ashmem/dalvik");
            CHECK_TRIM(TRIM_TYPE_FONT,     0x4, "/system/fonts/");
            CHECK_TRIM(TRIM_TYPE_USR,      0x4, "/system/usr/");
#ifdef __aarch64__
            CHECK_TRIM(TRIM_TYPE_LIB_CODE, 0x5, "/system/lib64/");
            CHECK_TRIM(TRIM_TYPE_LIB_CODE, 0x5, "/system/vendor/lib64/");
#else
            CHECK_TRIM(TRIM_TYPE_LIB_CODE, 0x5, "/system/lib/");
            CHECK_TRIM(TRIM_TYPE_LIB_CODE, 0x5, "/system/vendor/lib/");
#endif
        }
    }
    fclose(maps);
}

extern "C" void trim_core_dump(uintptr_t dump_java) {

    if (dump_java) {
        trim_core(TRIM_TYPE_FOR_FULL_DUMP);
    } else {
        trim_core(TRIM_TYPE_JAVA);
    }
}

#endif
