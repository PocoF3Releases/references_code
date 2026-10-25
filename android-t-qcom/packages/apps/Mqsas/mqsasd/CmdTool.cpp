/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/klog.h>
#include <sys/prctl.h>
#include <time.h>
#include <utils/String8.h>
#include <openssl/aes.h>

#include "CmdTool.h"
#include "utils/Log.h"

using namespace android;

#define LOGV(...)    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...)    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...)    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...)    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...)    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static const int64_t NANOS_PER_SEC = 1000000000;

static uint64_t nanotime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * NANOS_PER_SEC + ts.tv_nsec;
}

bool CmdTool::waitpid_with_timeout_lite(pid_t pid, int timeout_seconds, int* status) {
    sigset_t child_mask, old_mask;
    sigemptyset(&child_mask);
    sigaddset(&child_mask, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &child_mask, &old_mask) == -1) {
        LOGE("mqsasd cmdtool *** sigprocmask failed: %s\n", strerror(errno));
        return false;
    }

    struct timespec ts;
    ts.tv_sec = timeout_seconds;
    ts.tv_nsec = 0;
    int ret = TEMP_FAILURE_RETRY(sigtimedwait(&child_mask, NULL, &ts));
    int saved_errno = errno;
    // Set the signals back the way they were.
    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) == -1) {
        LOGE("mqsasd cmdtool *** sigprocmask failed: %s\n", strerror(errno));
        if (ret == 0) {
            return false;
        }
    }
    if (ret == -1) {
        errno = saved_errno;
        if (errno == EAGAIN) {
            errno = ETIMEDOUT;
        } else {
            LOGE("mqsasd cmdtool *** sigtimedwait failed: %s\n", strerror(errno));
        }
        return false;
    }

    pid_t child_pid = waitpid(pid, status, WNOHANG);
    if (child_pid != pid) {
        if (child_pid != -1) {
            LOGE("mqsasd cmdtool *** Waiting for pid %d, got pid %d instead\n", pid, child_pid);
        } else {
            LOGE("mqsasd cmdtool *** waitpid failed: %s\n", strerror(errno));
        }
        return false;
    }
    return true;
}

int CmdTool::run_command_lite(String16 _action, String16 _param, int timeout){
    String8 action(_action);
    String8 param(_param);
    return run_command_lite(action.string(),param.string(),timeout);
}

int CmdTool::run_command_lite(const char* action, const char* param,const int timeout){
    uint64_t start = nanotime();

    pid_t pid = fork();
    // handle error case
    if (pid < 0) {
        LOGD("mqsasd cmdtool *** fork: %s\n",strerror(errno));
        return pid;
    }

    // handle child case
    if (pid == 0) {

        // make sure the child dies when parent dies
        prctl(PR_SET_PDEATHSIG, SIGKILL);

        // just ignore SIGPIPE, will go down with parent's
        struct sigaction sigact;
        memset(&sigact, 0, sizeof(sigact));
        sigact.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sigact, NULL);

        // split param with ','
        const char* args[1024] = {action};
        char param_temp[1024];
        strcpy(param_temp, param);
        char* each;
        int index = 1;
        each = strtok(param_temp, ",");
        while(each){
            args[index++] = each;
            each = strtok(NULL, ",");
        }
        args[index] = NULL;
        LOGD("mqsasd cmdtool *** (%s): %s\n", action, param);
        execvp(action, (char**)args);
        LOGE("mqsasd cmdtool *** exec(%s): %s\n", action, strerror(errno));
        _exit(-1);
    }

    //handle parent case
    int status;
    bool ret = waitpid_with_timeout_lite(pid, timeout, &status);
    uint64_t elapsed = nanotime() - start;
    if (!ret) {
        if (errno == ETIMEDOUT) {
            LOGE("mqsasd cmdtool *** %s: Timed out after %.3fs (killing pid %d)\n", action,
                   (float) elapsed / NANOS_PER_SEC, pid);
        } else {
            LOGE("mqsasd cmdtool *** %s: Error after %.4fs (killing pid %d)\n", action,
                   (float) elapsed / NANOS_PER_SEC, pid);
        }
        kill(pid, SIGTERM);
        if (!waitpid_with_timeout_lite(pid, 5, NULL)) {
            kill(pid, SIGKILL);
            if (!waitpid_with_timeout_lite(pid, 5, NULL)) {
                LOGE("mqsasd cmdtool *** %s: Cannot kill %d even with SIGKILL.\n", action, pid);
            }
        }
        return -1;
    }

    if (WIFSIGNALED(status)) {
        LOGE("mqsasd cmdtool *** %s: Killed by signal %d\n", action, WTERMSIG(status));
    } else if (WIFEXITED(status) && WEXITSTATUS(status) > 0) {
        LOGE("mqsasd cmdtool *** %s: Exit code %d\n", action, WEXITSTATUS(status));
    }
    LOGD("mqsasd cmdtool [%s: %.3fs elapsed57]\n\n", action, (float)elapsed / NANOS_PER_SEC);

    return 0;
}
