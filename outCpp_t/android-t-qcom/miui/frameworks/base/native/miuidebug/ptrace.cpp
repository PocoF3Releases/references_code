#define LOG_TAG " MIUINDBG_PTRACE"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include "ndbglog.h"

#if defined(__arm__) ||  defined(__aarch64__)
class PtraceWrapper {
public:
    PtraceWrapper(pid_t pid, pid_t tid);
    virtual ~PtraceWrapper();
    int CallTargetFunction(const char* libName, const char* funcName,
        uintptr_t* paramArray, uintptr_t paramNum,uintptr_t* retVal);
private:
#if defined(__aarch64__)
#define pt_regs         user_pt_regs
#define uregs           regs
#define ARM_pc          pc
#define ARM_cpsr        pstate
#define ARM_lr          regs[30]
#define ARM_sp          sp
#define ARM_r0          regs[0]

#define PTRACE_GETREGS  PTRACE_GETREGSET
#define PTRACE_SETREGS  PTRACE_SETREGSET
#endif
#define STACK_ALIGN     16
#define CPSR_T_MASK     ( 1u << 5 )

    int GetRegs(struct pt_regs * regs);
    int SetRegs(struct pt_regs * regs);
    int ReadData(uint8_t *target, uint8_t *buf, size_t size);
    int WriteData(uint8_t *target, uint8_t *data, size_t size);
    int WaitSignal(int mtimeout);
    int Continue();
    int CallFunc(uintptr_t addr, uintptr_t *params, int num_params);
    int PushToStack(const char* ptr, size_t size,uintptr_t* newsp);
    int GetReturnValue(uintptr_t* retval);
    int PrepareTempStack();
    int ReleseTempStack();
    int SaveContext();
    int RestoreContext();
    int GetMapRange(pid_t pid, const char* name, uintptr_t* start, uintptr_t *end = NULL);
    int GetStack(uintptr_t* start,uintptr_t* end);
    int SetTempStackTop();


    class TempStack {
    public:
        enum STACK_TYPE{
            STACK_TYPE_MAPPED,
            STACK_TYPE_ORIGINAL,
            STACK_TYPE_UNSAFE,
            STACK_TYPE_INVALID,
        };
        uintptr_t GetStackTop() { return mBase + mSize - 0x1000; }
        STACK_TYPE          mType = STACK_TYPE_INVALID;
        uintptr_t           mBase = 0;
#if defined(__aarch64__)
        const uintptr_t     mSize = 0x8000;
#else
        const uintptr_t     mSize = 0x4000;
#endif
        void*               mBackup = (void *)-1;
    };
    pid_t               mPid;
    pid_t               mTid;
    uintptr_t           mDlopen;
    uintptr_t           mDlsym;
    uintptr_t           mDlclose;
    uintptr_t           mReturnAddr;
    TempStack           mTempStack;
    struct pt_regs      mSavedRegs;

};


PtraceWrapper::PtraceWrapper(pid_t pid, pid_t tid) : mPid(0), mTid(0){
#ifdef NDBG_FOR_ANDROID_N
    if (ptrace(PTRACE_ATTACH, tid, 0, 0) != 0) {
        MILOGW("ptrace attach err! errno=%d",errno);
        return;
    }
#endif
    uintptr_t hostLiker = 0, targetLinker = 0;

    GetMapRange(-1, "/system/bin/linker", &hostLiker);
    GetMapRange(pid, "/system/bin/linker", &targetLinker);
    if (!hostLiker || !targetLinker) {
        MILOGE("cannot find linker host=%p,target=%p",(void*)hostLiker,(void*)targetLinker);
        return;
    }

    uintptr_t gap = targetLinker - hostLiker;

    mDlopen = gap + (uintptr_t)dlopen;
    mDlsym = gap + (uintptr_t)dlsym;
    mDlclose = gap + (uintptr_t)dlclose;

    mPid = pid;
    mTid = tid;

    uintptr_t start,end;
    if(GetMapRange(mPid, "libc.so", &start, &end)) {
        MILOGE("cannot find libc.so");
        end = 0;
    }

    mReturnAddr = end;

    if (SaveContext()) {
        mPid = 0;
        mTid = 0;
        MILOGE("SaveContext failed!");
    }
}

PtraceWrapper::~PtraceWrapper() {
    if (RestoreContext()) {
        MILOGE("RestoreContext failed!");
    }

#ifdef NDBG_FOR_ANDROID_N
    if (mTid && ptrace(PTRACE_DETACH, mTid, 0, 0) != 0) {
        MILOGW("ptrace detach err! errno=%d",errno);
    }
#endif
}

int PtraceWrapper::GetRegs(struct pt_regs * regs) {
    long ret;

#if defined (__aarch64__)
    uintptr_t regset = 1; //NT_PRSTATUS;
    struct iovec ioVec;

    ioVec.iov_base = regs;
    ioVec.iov_len = sizeof(*regs);

    ret = ptrace(PTRACE_GETREGSET, mTid, regset, &ioVec);
#else
    ret = ptrace(PTRACE_GETREGS, mTid, NULL, regs);
#endif
    if (ret < 0) {
        MILOGD("Can not get register values");
        return -1;
    }
    return 0;
}

int PtraceWrapper::SetRegs(struct pt_regs * regs) {
    long ret;

#if defined (__aarch64__)
    int regset = 1; //NT_PRSTATUS;
    struct iovec ioVec;

    ioVec.iov_base = regs;
    ioVec.iov_len = sizeof(*regs);

    ret = ptrace(PTRACE_SETREGSET, mTid, regset, &ioVec);
#else
    ret = ptrace(PTRACE_SETREGS, mTid, NULL, regs);
#endif

    if (ret < 0) {
        MILOGD("Can not set register values");
        return -1;
    }
    return 0;
}

int PtraceWrapper::ReadData(uint8_t *target, uint8_t *buf, size_t size) {
    long i, count, remain;
    uint8_t *laddr;

    union u {
        uintptr_t val;
        char chars[sizeof(uintptr_t)];
    } d;

    count = size / sizeof(uintptr_t);
    remain = size % sizeof(uintptr_t);

    laddr = buf;

    for (i = 0; i < count; i++) {
        d.val = ptrace(PTRACE_PEEKTEXT, mTid, target, 0);
        memcpy(laddr, d.chars, sizeof(uintptr_t));
        target += sizeof(uintptr_t);
        laddr += sizeof(uintptr_t);
    }

    if (remain > 0) {
        d.val = ptrace(PTRACE_PEEKTEXT, mTid, target, 0);
        memcpy(laddr, d.chars, remain);
    }

    return 0;
}

int PtraceWrapper::WriteData(uint8_t *target, uint8_t *data, size_t size) {
    long i, count, remain;
    uint8_t *laddr;

    union u {
        uintptr_t val;
        char chars[sizeof(uintptr_t)];
    } d;

    count = size / sizeof(uintptr_t);
    remain = size % sizeof(uintptr_t);

    laddr = data;

    for (i = 0; i < count; i++) {
        memcpy(d.chars, laddr, sizeof(uintptr_t));
        ptrace(PTRACE_POKETEXT, mTid, target, (void*)d.val);

        target  += sizeof(uintptr_t);
        laddr += sizeof(uintptr_t);
    }

    if (remain > 0) {
        d.val = ptrace(PTRACE_PEEKTEXT, mTid, target, 0);
        for (i = 0; i < remain; i ++) {
            d.chars[i] = *laddr ++;
        }

        ptrace(PTRACE_POKETEXT, mTid, target, (void*)d.val);
    }

    return 0;
}

int PtraceWrapper::WaitSignal(int mtimeout) {
    for (;;) {
        int status;
        pid_t n = waitpid(mTid, &status, __WALL | WNOHANG);
        if (n < 0) {
            if (errno == EAGAIN) continue;
            MILOGE("waitpid failed: %s\n", strerror(errno));
            return -1;
        } else if (n > 0) {
            if (WIFSTOPPED(status)) {
                return WSTOPSIG(status);
            } else {
                MILOGE("unexpected waitpid response: n=%d, status=%08x\n", n, status);
                return -1;
            }
        }

        /* not ready yet */
        usleep(100*1000);
        mtimeout -= 100;
        if (mtimeout < 0) {
            MILOGD("time out waiting for tid=%d to die\n", mTid);
            return -1;
        }
    }
}

int PtraceWrapper::Continue() {
    int signal,retry = 3;

    do {
        if (ptrace(PTRACE_CONT, mTid, NULL, NULL) < 0) {
            MILOGE("PTRACE_CONT failed!");
            return -1;
        }
        signal = WaitSignal(1000);
    }while (signal != SIGSEGV && retry--);

    if (signal != SIGSEGV) {
        MILOGW("unexpected signal=%d", signal);
        if (signal == SIGILL) {
            struct pt_regs regs;
            GetRegs(&regs);
            MILOGE("pc=%p",(void*)regs.ARM_pc);
        }
        return -1;
    }

    return 0;
}

int PtraceWrapper::CallFunc(uintptr_t addr, uintptr_t *params, int num_params) {
    int i;
#if defined(__arm__)
    int paramRegsNum = 4;
#elif defined(__aarch64__)
    int paramRegsNum = 8;
#endif
    struct pt_regs regs;

    if (GetRegs(&regs)) {
        MILOGE("GetRegs failed!");
        return -1;
    }

    for (i = 0; i < num_params && i < paramRegsNum; i++) {
        regs.uregs[i] = params[i];
    }

    if (i < num_params) {
        regs.ARM_sp -= (num_params - i) * sizeof(uintptr_t) ;
        regs.ARM_sp = (regs.ARM_sp / STACK_ALIGN ) * STACK_ALIGN;
        WriteData((uint8_t *)regs.ARM_sp, (uint8_t *)&params[i], (num_params - i) * sizeof(uintptr_t));
    }

    regs.ARM_pc = addr;
    if (regs.ARM_pc & 1) {
        /* thumb */
        regs.ARM_pc &= (~1u);
        regs.ARM_cpsr |= CPSR_T_MASK;
    } else {
        /* arm */
        regs.ARM_cpsr &= ~CPSR_T_MASK;
    }
    regs.ARM_lr = mReturnAddr;

    if (SetRegs(&regs) == -1 || Continue() == -1) {
        MILOGE("SetRegs or Continue failed!!!");
        return -1;
    }
    return 0;
}

int PtraceWrapper::PushToStack(const char* ptr, size_t size,uintptr_t* newsp) {
    int ret;
    struct pt_regs regs;
    uintptr_t arm_sp = 0;

    ret = GetRegs(&regs);
    if (!ret) {
        arm_sp = regs.ARM_sp;
        arm_sp -= size;
        arm_sp -= arm_sp % STACK_ALIGN;
        WriteData((uint8_t *)arm_sp, (uint8_t *)ptr, size);
        regs.ARM_sp= arm_sp;
        ret = SetRegs(&regs);
        *newsp = arm_sp;
    }

    return ret;
}

int PtraceWrapper::GetReturnValue(uintptr_t* retval) {
    if (!retval) {
        return 0;
    }

    struct pt_regs regs;

    if (GetRegs(&regs) == -1) {
        return -1;
    }

    *retval = regs.ARM_r0;
    if ((uintptr_t)regs.ARM_pc != mReturnAddr) {
        MILOGE("GetReturnValue pc=%p,lr=%p",(void*)regs.ARM_pc,(void*)regs.ARM_lr);
        return -1;
    }

    return 0;
}

int PtraceWrapper::PrepareTempStack() {
    int ret;
    char stackName[16];
    uintptr_t start, end;

    if (mTid != mPid) {
        sprintf(stackName, "[stack:%d]", mTid);
    } else {
        strcpy(stackName,"[stack]");
    }
    ret = GetMapRange(mPid, stackName, &start, &end);

    if (ret || !start || !end) {
        MILOGW("GetMapRange failed,TempStack is unsafe!");
        mTempStack.mType = TempStack::STACK_TYPE_UNSAFE;
        mTempStack.mBase = mSavedRegs.ARM_sp - mTempStack.mSize;
        return 0;
    }

    if ((uintptr_t)mSavedRegs.ARM_sp < end
        && (uintptr_t)mSavedRegs.ARM_sp > start + mTempStack.mSize) {
        mTempStack.mBase = start;
        mTempStack.mType = TempStack::STACK_TYPE_ORIGINAL;
    } else {
        uint8_t miniStack[1024];

        mTempStack.mBase = start;

        ReadData((uint8_t *)mTempStack.GetStackTop() - sizeof(miniStack), miniStack, sizeof(miniStack));

        const char* libName = "libc.so";
        const char* funcName = "mmap";
        uintptr_t mapBase;
        uintptr_t params[6] = {
            0,
            mTempStack.mSize,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
            (uintptr_t)-1,
            0
        };

        ret = CallTargetFunction(libName, funcName, params, sizeof(params), &mapBase);

        if (ret || mapBase ==(uintptr_t)-1) {
            MILOGW("target mmap failed ,so we can only use the orginal stack!");
            mTempStack.mBase = start;
            mTempStack.mType = TempStack::STACK_TYPE_ORIGINAL;
            mTempStack.mBackup = mmap(NULL, mTempStack.mSize, PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
            if (mTempStack.mBackup == (void *)-1) {
                MILOGW("mmap backup stack failed, original stack may be modified!");
            } else {
                ReadData((uint8_t *)start, (uint8_t *)mTempStack.mBackup, mTempStack.mSize);
            }
        } else {
            mTempStack.mBase = mapBase;
            mTempStack.mType = TempStack::STACK_TYPE_MAPPED;
            MILOGI("using mapped stack!");
        }
        WriteData((uint8_t *)mTempStack.GetStackTop() - sizeof(miniStack),miniStack, sizeof(miniStack));
    }
    return 0;
}

int PtraceWrapper::ReleseTempStack() {
    int ret = 0;

    if (mTempStack.mType == TempStack::STACK_TYPE_MAPPED) {
        const char* libName = "libc.so";
        const char* funcName = "munmap";

        uintptr_t params[2] = {
            mTempStack.mBase,
            mTempStack.mSize
        };
        ret = CallTargetFunction(libName, funcName, params, sizeof(params), NULL);
        if (ret) {
            MILOGE("target munmap failed!");
        }
    } else if (mTempStack.mType == TempStack::STACK_TYPE_ORIGINAL) {
        if (mTempStack.mBackup != (void *)-1) {
            ret = WriteData((uint8_t *)mTempStack.mBackup, (uint8_t *)mTempStack.mBase, mTempStack.mSize);
            if (ret) {
                MILOGW("restore stack failed!");
            }
            munmap(mTempStack.mBackup, mTempStack.mSize);
            mTempStack.mBackup = (void *)-1;
        }
    } else {
        MILOGE("invalid stack type!");
        ret = -1;
    }
    return ret;
}

int PtraceWrapper::SaveContext() {
    int ret = GetRegs(&mSavedRegs);
    if (!ret) {
       ret = PrepareTempStack();
    }
    return ret;
}
int PtraceWrapper::RestoreContext() {
    int ret = ReleseTempStack();
    ret |= SetRegs(&mSavedRegs);
    return ret;
}

int PtraceWrapper::GetMapRange(pid_t pid, const char* name, uintptr_t* start, uintptr_t *end) {
    FILE *fp;
    char *pch;
    char filename[32];
    char line[1024];
    int ret = -1;

    if (start) *start = 0;

    if (end) *end = 0;

    if (pid < 0) {
         strncpy(filename, "/proc/self/maps",sizeof(filename));
    } else {
         snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }

    fp = fopen(filename, "r");

    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, name)) {
                if (start) {
                    *start = (uintptr_t)strtoull(line, NULL, 16);
                }
                if (end) {
                    pch = strchr(line,'-');
                    if (pch) {
                        *end = (uintptr_t)strtoull(pch+1, NULL, 16);
                    }
                }
                ret = 0;
                break;
            }
        }
        fclose(fp);
    }
    return ret;
}

int PtraceWrapper::GetStack(uintptr_t* start,uintptr_t* end) {
    char stack_name[16];

    if (mTid != mPid) {
        sprintf(stack_name, "[stack:%d]", mTid);
    } else {
        strcpy(stack_name,"[stack]");
    }

    *start = 0;
    *end  =0;

    GetMapRange(mPid, stack_name, start, end);

    if (!(*start) || !(*end) || *(end)-*(start) < 4096) {
        return -1;
    }
    return 0;

}
int PtraceWrapper::SetTempStackTop() {
    struct pt_regs regs;

    int ret = GetRegs(&regs);
    if (!ret) {
        regs.ARM_sp = mTempStack.GetStackTop();
        ret = SetRegs(&regs);
    }
    return ret;
}

int PtraceWrapper::CallTargetFunction(const char* libName, const char* funcName,
            uintptr_t* paramArray, uintptr_t paramNum,uintptr_t* retVal)
{
#define _CHECK_(_cond) \
        do { \
            if (_cond) { \
                MILOGE("failed return value line= %d!",__LINE__); \
                goto err; \
            } \
        } while (0)

    int ret = -1;

    if (!mTid || !mPid) {
        MILOGE("no attached target!");
        return ret;
    }

    if (SetTempStackTop()) {
        MILOGE("SetTempStackTop failed!");
        return ret;
    }

    uintptr_t soHandle = 0, targetFunc = 0;
    uintptr_t dlParams[2];

    dlParams[1] = RTLD_NOW | RTLD_GLOBAL;
    _CHECK_(PushToStack(libName,strlen(libName)+1,&dlParams[0]));
    _CHECK_(CallFunc(mDlopen,dlParams,2));
    _CHECK_(GetReturnValue(&soHandle));
    _CHECK_(soHandle == 0);

    dlParams[0] = soHandle;
    _CHECK_(PushToStack(funcName,strlen(funcName)+1,&dlParams[1]));
    _CHECK_(CallFunc(mDlsym,dlParams,2));
    _CHECK_(GetReturnValue(&targetFunc));
    _CHECK_(targetFunc == 0);

    if(0 == strcmp(funcName,"dump_all_threads")) {
        uintptr_t param;
        _CHECK_(PushToStack((const char*)&mSavedRegs,sizeof(mSavedRegs), &param));
        _CHECK_(CallFunc(targetFunc,&param,1));
    } else {
        _CHECK_(CallFunc(targetFunc,paramArray,paramNum));
    }

    _CHECK_(GetReturnValue(retVal));

    ret = 0;
err:
    if (soHandle) {
        dlParams[0] = soHandle;
        if (CallFunc(mDlclose,dlParams,1)) {
            MILOGW("close libs failed!");
        }
    }
    return ret;
}

extern "C" int ptrace_call(pid_t pid,
            pid_t tid,
            const char* so_path,
            const char* func_name,
            uintptr_t* params,
            uintptr_t param_num,
            uintptr_t* retval)
{
    PtraceWrapper pw = PtraceWrapper(pid,tid);
    int ret = pw.CallTargetFunction(so_path,func_name,params,param_num,retval);
    return ret;
}

#else
extern "C" int ptrace_call(pid_t pid,
                    pid_t tid,
                    const char* so_path,
                    const char* func_name,
                    uintptr_t* params,
                    uintptr_t param_num,
                    uintptr_t* retval)
{
    printf("call %d,%d,%s,%s,%p,%zu,%p",pid,tid,so_path,func_name,params,param_num,retval);
    return -1;
}
#endif

