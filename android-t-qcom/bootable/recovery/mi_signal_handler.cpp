#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/prctl.h>
#include <android-base/logging.h>

#include "mi_signal_handler.h"
#include "recovery.h"
#include <iostream>

jmp_buf jb;
static RecoveryUI* ui_ = nullptr;

// see man(2) prctl, specifically the section about PR_GET_NAME
#define MAX_TASK_NAME_LEN (16)

static void log_signal_summary(int signum, const siginfo_t* info, ucontext_t* ucontext) {
    const char* signal_name;
    switch (signum) {
        case SIGILL:    signal_name = "SIGILL";     break;
        case SIGABRT:   signal_name = "SIGABRT";    break;
        case SIGBUS:    signal_name = "SIGBUS";     break;
        case SIGFPE:    signal_name = "SIGFPE";     break;
        case SIGSEGV:   signal_name = "SIGSEGV";    break;
#if defined(SIGSTKFLT)
        case SIGSTKFLT: signal_name = "SIGSTKFLT";  break;
#endif
        case SIGPIPE:   signal_name = "SIGPIPE";    break;
        default:        signal_name = "???";        break;
    }

    char thread_name[MAX_TASK_NAME_LEN + 1]; // one more for termination
    if (prctl(PR_GET_NAME, (unsigned long)thread_name, 0, 0, 0) != 0) {
        strcpy(thread_name, "<name unknown>");
    } else {
        // short names are null terminated by prctl, but the man page
        // implies that 16 byte names are not.
        thread_name[MAX_TASK_NAME_LEN] = 0;
    }

    // "info" will be NULL if the siginfo_t information was not available.
    if (info != NULL) {
        LOG(ERROR) << "Fatal signal " << signum << " (" << signal_name << ") at 0x"
            << reinterpret_cast<uintptr_t>(info->si_addr) << " (code="  <<
            info->si_code << "), " << getpid() << "thread " << gettid() << "(" <<
            thread_name << ")";
        std::cout << "Fatal signal " << signum << " (" << signal_name << ") at 0x"
            << reinterpret_cast<uintptr_t>(info->si_addr) << " (code="  <<
            info->si_code << "), " << getpid() << "thread " << gettid() << "(" <<
            thread_name << ")" << std::endl;
    } else {
        LOG(ERROR) << "Fatal signal " << signum << " (" << signal_name << "), thread "
            << gettid() << " (" << thread_name << ")";
        std::cout << "Fatal signal " << signum << " (" << signal_name << "), thread "
            << gettid() << " (" << thread_name << ")" << std::endl;
    }

#if defined(__aarch64__)
    LOG(ERROR) << "ARM64 PC address :" << (unsigned long)(ucontext->uc_mcontext.pc);
    LOG(ERROR) << "ARM64 LR address :" << (unsigned long)(ucontext->uc_mcontext.regs[30]);
#else
    LOG(ERROR) << "ARM32 PC address :" << (unsigned long)(ucontext->uc_mcontext.arm_pc);
    LOG(ERROR) << "ARM32 LR address :" << (unsigned long)(ucontext->uc_mcontext.arm_lr);
    std::cout << "ARM32 PC address :" << (unsigned long)(ucontext->uc_mcontext.arm_pc) << std::endl;
    std::cout << "ARM32 LR address :" << (unsigned long)(ucontext->uc_mcontext.arm_lr) << std::endl;
#endif
    FinishRecovery(ui_);
}

/*
 * Returns true if the handler for signal "signum" has SA_SIGINFO set.
 */
static bool have_siginfo(int signum) {
    struct sigaction old_action, new_action;

    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = SIG_DFL;
    new_action.sa_flags = SA_RESTART;
    sigemptyset(&new_action.sa_mask);

    if (sigaction(signum, &new_action, &old_action) < 0) {
      LOG(ERROR) << "Failed testing for SA_SIGINFO: " << strerror(errno);
      return false;
    }
    bool result = (old_action.sa_flags & SA_SIGINFO) != 0;

    if (sigaction(signum, &old_action, NULL) == -1) {
      LOG(ERROR) << "Restore failed in test for SA_SIGINFO: " << strerror(errno);
    }
    return result;
}

void debuggerd_signal_handler(int n, siginfo_t* info, void* context) {
    if (!have_siginfo(n)) {
        info = NULL;
    }

    ucontext_t* ucontext = (ucontext_t*)context;
    log_signal_summary(n, info, ucontext);

    /* remove our net so we fault for real when we return */
    signal(n, SIG_DFL);

    switch (n) {
        case SIGABRT:
        case SIGFPE:
        case SIGPIPE:
            break;
        default:    // SIGILL, SIGBUS, SIGSEGV
            break;
    }
}

void debuggerd_init(RecoveryUI* ui) {
    ui_ = ui;
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = debuggerd_signal_handler;
    action.sa_flags = SA_RESTART | SA_SIGINFO;

    // Use the alternate signal stack if available so we can catch stack overflows.
    action.sa_flags |= SA_ONSTACK;

    sigaction(SIGABRT, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
    sigaction(SIGFPE, &action, NULL);
    sigaction(SIGILL, &action, NULL);
    sigaction(SIGPIPE, &action, NULL);
    sigaction(SIGSEGV, &action, NULL);
#if defined(SIGSTKFLT)
    sigaction(SIGSTKFLT, &action, NULL);
#endif
    sigaction(SIGTRAP, &action, NULL);
}
