#include <unistd.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <stdio.h>
#include <thread>
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/properties.h>
#include <jni.h>
#include <nativehelper/JNIHelp.h>

#define BIONIC_SIGNAL_FDTRACK 39
#define PATHMAX 64

static int fdEnableThreshold;
static int fdAbortThreshold;
static int fdCheckInterval;

static int threadAbortThreshold;
static int threadCheckInterval;

static int fdMonitorThreadGenerated = false;
static int threadMonitorThreadGenerated = false;

static std::string FD_PROP_PREFIX = "persist.sys.debug.fd.";
static std::string THREAD_PROP_PREFIX = "persist.sys.debug.thread.";

static int getMaxFd() {
    // Not actually guaranteed to be the max, but close enough for our purposes.
    int fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (fd == -1) {
        ALOGE("failed to open /dev/null: %s", strerror(errno));
        return -1;
    }
    close(fd);
    return fd;
}

static int currentThread() {
    std::string status_path = android::base::StringPrintf("/proc/%d/status", getpid());
    const char* fileName = status_path.c_str();
    FILE* fp = fopen(fileName, "r");
    if (fp == NULL) {
        ALOGE("failed to open status path: %s", strerror(errno));
        return -1;
    }

    char* line = nullptr;
    size_t len = 0;

    while (getline(&line, &len, fp) != -1) {
        char* tab = strchr(line, '\t');
        if (tab == nullptr) {
            continue;
        }
        size_t headerLen = tab - line;
        std::string header = std::string(line, headerLen);
        if (header == "Threads:") {
            int threadNums = atoi(tab + 1);
            free(line);
            fclose(fp);
            return threadNums;
        }
    }
    free(line);
    fclose(fp);
    return -1;
}

static std::string currentProc() {
    std::string cmdline;
    android::base::ReadFileToString("/proc/self/cmdline", &cmdline);
    if (cmdline.back() == '\0') {
        cmdline.pop_back();
    }
    int loc = cmdline.find_last_of('/');
    if (loc == -1) {
        return cmdline;
    } else {
        return cmdline.substr(loc+1);
    }
}

static void triggerAbort() {
    sigval val;
    val.sival_int = 1;
    sigqueue(getpid(), BIONIC_SIGNAL_FDTRACK, val);
}

static void spawnFdMonitorThread() {
    std::thread([]() {
        pthread_setname_np(pthread_self(), "FD Monitor");
        bool enabled = false;
        bool dumped = false;

        while (true) {
            int maxFd = getMaxFd();
            if (maxFd > fdEnableThreshold && !enabled) {
                enabled = true;
                ALOGI("reaching fdtrack-enable-threshold! current fd number: %d. start to record!", maxFd);
                if (dlopen("libfdtrack.so", RTLD_GLOBAL) == nullptr) {
                    ALOGE("failed to dlopen libfdtrack.so, err: %s", strerror(errno));
                    break;
                }
            } else if (maxFd > fdAbortThreshold && !dumped) {
                dumped = true;
                ALOGE("abort due to fd leak! current fd number: %d. check logs for details.", maxFd);
                triggerAbort();
            }
            std::this_thread::sleep_for(std::chrono::seconds(fdCheckInterval));
        }
    }).detach();
}

static void spawnThreadMonitorThread() {
    std::thread([]() {
        pthread_setname_np(pthread_self(), "Thread Monitor");

        while (true) {
            LOG_ALWAYS_FATAL_IF(currentThread() >= threadAbortThreshold,
                    "detect thread leak! current thread number: %d. check file for details.",
                    currentThread());
            std::this_thread::sleep_for(std::chrono::seconds(threadCheckInterval));
        }
    }).detach();
}

/**
 * "extern" means that the function is intended for native invocation
 */
extern "C" void startMonitor() {
    bool monitorEnabled = android::base::GetBoolProperty("persist.mtbf.test", false) ||
            android::base::GetIntProperty("persist.omni.test", 0);
    if (!monitorEnabled) return;

    std::string procName = currentProc();

    bool fdMonitorEnabled = android::base::GetBoolProperty(
            FD_PROP_PREFIX + procName, false);
    bool threadMonitorEnabled = android::base::GetBoolProperty(
            THREAD_PROP_PREFIX + procName, false);

    fdEnableThreshold = android::base::GetIntProperty(
            FD_PROP_PREFIX + procName + ".bottom", 2048, 1, INT_MAX);
    fdAbortThreshold = android::base::GetIntProperty(
            FD_PROP_PREFIX + procName + ".ceiling", 3072, 1, INT_MAX);
    fdCheckInterval = android::base::GetIntProperty(
            FD_PROP_PREFIX + procName + ".interval", 120, 1, INT_MAX);

    threadAbortThreshold = android::base::GetIntProperty(
            THREAD_PROP_PREFIX + procName + ".ceiling", 500, 1, INT_MAX);
    threadCheckInterval = android::base::GetIntProperty(
            THREAD_PROP_PREFIX + procName + ".interval", 120, 1, INT_MAX);

    if (fdMonitorEnabled && !fdMonitorThreadGenerated) {
        spawnFdMonitorThread();
        fdMonitorThreadGenerated = true;
        ALOGI("fd-monitor thread spawned!");
    }
    if (threadMonitorEnabled && !threadMonitorThreadGenerated) {
        spawnThreadMonitorThread();
        threadMonitorThreadGenerated = true;
        ALOGI("thread-monitor thread spawned!");
    }
}/* "extern" section */

/**
 * functions intended for JNI
 */
static void android_app_MonitorTrack_fdtrackAbort(JNIEnv* env, jclass clazz) {
    triggerAbort();
}

static void android_app_MonitorTrack_spawnThreadMonitorThread(JNIEnv* env, jclass clazz, jint abortThreshold, jint interval) {
    threadAbortThreshold = abortThreshold;
    threadCheckInterval = interval;
    spawnThreadMonitorThread();
}

static jstring android_app_MonitorTrack_currentProc(JNIEnv* env, jclass clazz) {
    return env->NewStringUTF(currentProc().c_str());
}/* JNI section */

/*
 * JNI registration
 */
static const JNINativeMethod methods[] = {
        /* name ,signature, funcPtr */
        {"fdtrackAbort", "()V", (void*)android_app_MonitorTrack_fdtrackAbort},
        {"spawnThreadMonitorThread", "(II)V", (void*)android_app_MonitorTrack_spawnThreadMonitorThread},
        {"currentProc", "()Ljava/lang/String;", (void*)android_app_MonitorTrack_currentProc},
};

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return JNI_ERR;
    }

    // Find your class. JNI_OnLoad is called from the correct class loader context for this to work.
    jclass clazz = env->FindClass("android/app/MonitorTrack");
    ALOGE_IF(clazz == NULL,
                        "native registration unable to find class '%s'; aborting...",
                        "android/app/MonitorTrack");
    // Register your class' native methods.
    int result = env->RegisterNatives(clazz, methods, NELEM(methods));
    if (result != JNI_OK) {
        ALOGE("register native methods failed!");
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}
