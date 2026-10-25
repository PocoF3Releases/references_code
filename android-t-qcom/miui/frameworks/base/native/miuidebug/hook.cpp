#ifdef NDBG_FOR_ANDROID_N
#define LOG_TAG "MIUINDBG_HOOK"
#include <dlfcn.h>
#include <sys/mman.h>
#include <pthread.h>
#include <signal.h>
#include <sys/syscall.h>
#include <cutils/properties.h>
#include "ndbglog.h"

#define _LIBC_LOGGING_H
struct abort_msg_t {
      size_t size;
        char msg[0];
};

#include "linker.h"

class SoInfo {
public:
    SoInfo() ;
    virtual ~SoInfo() {};
    uintptr_t replace(const char* str, const uintptr_t addr);
private:
    void init();
    void setProtect(void* addr, int port);
    bool        mInit;
    ElfW(Addr)  mBase;
    const char* mStrtab;
    ElfW(Sym)*  mSymtab;
#if defined(USE_RELA)
    typedef ElfW(Rela) rel_t;
#else
    typedef ElfW(Rel) rel_t;
#endif
    rel_t*      mPltRel;
    size_t      mPltRelCount;
    size_t      mPageSize;
#if defined(__aarch64__)
#define R_JUMP_SLOT R_AARCH64_JUMP_SLOT
#else
#define R_JUMP_SLOT R_ARM_JUMP_SLOT
#endif
};


#ifndef ELFW
#if defined(__LP64__)
#define ELFW(what) ELF64_ ## what
#else
#define ELFW(what) ELF32_ ## what
#endif
#endif

SoInfo::SoInfo() {
    mInit = false;
    mBase = 0;
    mStrtab = NULL;
    mSymtab = NULL;
    mPltRel = NULL;
    mPltRelCount = 0;
    mPageSize = sysconf(_SC_PAGESIZE);

    init();
}

typedef void (*SET_SDK_VERSION)(unsigned int);
typedef unsigned int (*GET_SDK_VERSION)(void);

void SoInfo::init() {
    struct soinfo* somain = (struct soinfo*)dlopen(NULL,RTLD_NOW);

    GET_SDK_VERSION getSdkVersion = (GET_SDK_VERSION)dlsym(somain, "android_get_application_target_sdk_version");
    SET_SDK_VERSION setSdkVersion= (SET_SDK_VERSION)dlsym(somain, "android_set_application_target_sdk_version");
    dlclose(somain);

    if (getSdkVersion && setSdkVersion) {
        unsigned int version = getSdkVersion();
        if (version > 23) {
            setSdkVersion(23);
            somain = (struct soinfo*)dlopen(NULL,RTLD_NOW);
            dlclose(somain);
            setSdkVersion(version);
        }
    }

    if (!somain) {
        MILOGE("not found main soinfo!");
        return;
    }

    mBase = somain->load_bias;

    if (!mBase)  {
        MILOGE("not found load_bias");
        return;
    }

    for (ElfW(Dyn)* d = somain->dynamic; d->d_tag != DT_NULL; ++d) {
        switch (d->d_tag) {
        case DT_STRTAB:
            mStrtab = reinterpret_cast<const char*>(mBase + d->d_un.d_ptr);
            break;
        case DT_SYMTAB:
            mSymtab = reinterpret_cast<ElfW(Sym)*>(mBase + d->d_un.d_ptr);
            break;
      case DT_JMPREL:
            mPltRel = reinterpret_cast<rel_t*>(mBase + d->d_un.d_ptr);
            break;
        case DT_PLTRELSZ:
            mPltRelCount = d->d_un.d_val / sizeof(rel_t);
            break;
         }
    }
    if (!mStrtab || !mSymtab || !mPltRel|| !mPltRelCount) {
        return;
    }

    mInit =  true;
}
void SoInfo::setProtect(void* addr, int port) {
    mprotect((void *)((size_t)addr & ~(mPageSize -1)), mPageSize, port);
}

uintptr_t SoInfo::replace(const char* str, const uintptr_t addr)
{
    if (!mInit) return 0;

    uintptr_t old_value = 0;
    rel_t* relPtr = mPltRel;
    size_t relCnt = mPltRelCount;

    for (size_t idx = 0; idx < relCnt; ++idx, ++relPtr) {
        ElfW(Word) sym = ELFW(R_SYM)(relPtr->r_info);
        ElfW(Sym)* s = mSymtab + sym;

        if (strcmp(mStrtab + s->st_name, str)) continue;

        ElfW(Word) type = ELFW(R_TYPE)(relPtr->r_info);
        if (type == R_JUMP_SLOT) {
#if defined(USE_RELA)
             uintptr_t addend = relPtr->r_addend;
#else
             uintptr_t addend = 0;
#endif
             uintptr_t* slot_addr = reinterpret_cast<uintptr_t*>(mBase + relPtr->r_offset);

             old_value = *slot_addr;
             setProtect(slot_addr,PROT_READ | PROT_WRITE);
             *slot_addr = addr + addend;
             setProtect(slot_addr,PROT_READ);
             break;
        }
    }
    return old_value;
}

extern "C" void miui_native_debug_process(pid_t pid, pid_t tid);
extern "C" long syscall(long number, ...);
extern "C" long hook_syscall(long number, ...) {
    long ret;
    va_list ap;
    va_start(ap, number);

    if (SYS_tgkill == number) {
        int pid = va_arg(ap,int);
        int tid = va_arg(ap,int);
        int signal = va_arg(ap,int);
        miui_native_debug_process(pid,tid);
        ret = syscall(SYS_tgkill, pid, tid, signal);
    } else {
        ret = syscall(number, ap);
    }
    va_end(ap);
    return (ret);
}


extern "C" int pthread_setname_np(pthread_t, const char*);
extern "C" int hook_pthread_setname_np(pthread_t thread, const char* name) {
    MILOGI("hook hook_pthread_setname_np name=%s", name);
#if defined(__LP64__)
  const char* sigsendersname = "debuggerd64:sig";
#else
  const char* sigsendersname = "debuggerd:sig";
#endif
    if (!strcmp(name,sigsendersname)) {
        SoInfo si;
        if (si.replace("syscall",(const uintptr_t)&hook_syscall)) {
            MILOGI("replace syscall succeed!");
            struct sigaction act;
            act.sa_handler = SIG_DFL;
            sigemptyset(&act.sa_mask);
            sigaddset(&act.sa_mask,SIGCHLD);
            act.sa_flags = SA_NOCLDWAIT;
            sigaction(SIGCHLD, &act, 0);
        } else {
            MILOGI("replace syscall failed!");
        }
    }

    return pthread_setname_np(thread, name);
}

#define COREDUMP_PROP        "sys.miui.ndcd"

#define MIUI_NATIVE_DEBUG_ROOT_DIR          "/data/miuilog/stability/nativecrash"
#ifdef NDBG_LOCAL
#define MIUI_NATIVE_DEBUG_RULES_XML         MIUI_NATIVE_DEBUG_ROOT_DIR"/rules_local.xml"
#else
#define MIUI_NATIVE_DEBUG_RULES_XML         MIUI_NATIVE_DEBUG_ROOT_DIR"/rules.xml"
#endif

extern "C" int sigtimedwait(const sigset_t*, siginfo_t*, const timespec*);
extern "C" int hook_sigtimedwait(const sigset_t* set, siginfo_t* info, const timespec* timeout) {
    MILOGI("hook hook_sigtimedwait");
    int signal;
    struct timespec time;

    /*rules does not exist*/
    if (access(MIUI_NATIVE_DEBUG_RULES_XML, F_OK) == -1) {
        return sigtimedwait(set, info, timeout);
    }

    /*rules does exist, wait more 3s, lsof need more time*/
    time.tv_sec = timeout->tv_sec + 3;
    time.tv_nsec = 0;

    signal = sigtimedwait(set, info, &time);
    if (signal == SIGCHLD)
        return signal;

    /* try 5 times, the timeout is 3s */
    int count = 5;
    char prop[PROPERTY_VALUE_MAX];

    time.tv_sec = 3;
    time.tv_nsec = 0;

    do {
        signal = sigtimedwait(set, info, &time);
        if (signal == SIGCHLD)
            break;

        property_get(COREDUMP_PROP, prop, "off");
    } while((--count > 0) && (!strcmp(prop, "on") || !strcmp(prop, "true") || !strcmp(prop, "yes")));

    return signal;
}

extern "C" void hook_context_do_hook() {
    MILOGI("hook_context_do_hook: start hook");
    if (getppid() != 1) {
        return;
    }

    SoInfo si;
    if (si.replace("pthread_setname_np",(const uintptr_t)&hook_pthread_setname_np)) {
        MILOGI("replace pthread_setname_np succeed!");
    }

    if (si.replace("sigtimedwait",(const uintptr_t)&hook_sigtimedwait)) {
        MILOGI("replace sigtimedwait succeed!");
    }
}
#endif
