#define LOG_TAG "BinderMonitor"

#include <inttypes.h>
#include <utils/Log.h>
#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>

#include "jni.h"
#include "core_jni_helpers.h"
#include <fcntl.h>

#define DELAY_VERSION_1         1

#define BINDER_PID_INDEX 0
#define BINDER_TARGET_PID_INDEX 1
#define BINDER_TARGET_TID_INDEX 2
#define DELAY_TARGET_BLKIO_INDEX 3
#define DELAY_TARGET_SWAPIN_INDEX 4
#define DELAY_TARGET_FREEPAGES_INDEX 5
#define DELAY_TARGET_RUNNING_TIME_INDEX 6
#define DELAY_TARGET_RUNNABLE_TIME_INDEX 7
#define DELAY_TARGET_CPU_UTIME_INDEX 8
#define DELAY_TARGET_CPU_STIME_INDEX 9
#define DELAY_TARGET_BINDER_THREAD_FULL 10
#define DELAY_NUMS 11

using namespace android;

static pthread_once_t initflag = PTHREAD_ONCE_INIT;
static pthread_key_t pthread_key;
static volatile bool key_created = false;

static void destroy_thread_delay_fd(void *data) {
    int fd = (int)(long)data;
    if (fd > 0) {
        close(fd);
    }
}

static void create_thread_key() {
    int rc = pthread_key_create(&pthread_key, destroy_thread_delay_fd);
    if (rc == 0) {
        key_created = true;
    } else {
        key_created = false;
    }
}

static int get_thread_delay_fd() {
    if (!key_created) {
        pthread_once(&initflag, create_thread_key);
    }
    int thread_delay_fd = (int)(long)pthread_getspecific(pthread_key);
    if (thread_delay_fd == 0) {
        char path[128] = {};
        snprintf(path, sizeof(path), "/proc/mi_log/binder_delay");
        thread_delay_fd = open(path, O_RDWR|O_CLOEXEC);
        if (thread_delay_fd < 0) {
            ALOGE("err open binder_delay errno=%d", errno);
        }

        pthread_setspecific(pthread_key, (void *)(long)thread_delay_fd);
    }
    return thread_delay_fd;
}

struct binder_delay_struct {
    uint64_t version;
    pid_t pid;                    /* calling binder pid */
    pid_t binder_target_pid;      /* remote binder pid */
    char binder_target_comm[16];  /* remote binder process name */
    pid_t binder_target_tid;      /* remote binder tid */
    uint64_t blkio_delay;         /* wait for sync block io completion */
    uint64_t swapin_delay;        /* wait for swapin block io completion */
    uint64_t freepages_delay;     /* wait for memory reclaim */
    uint64_t cpu_runtime;
    uint64_t cpu_run_delay;
    uint64_t cpu_utime;
    uint64_t cpu_stime;
    uint64_t binder_target_thread_full;
};

static void android_os_BinderMonitor_GetBinderDelay(JNIEnv* env, jclass, jlongArray jdelayArr, jobjectArray jdelayInfo) {
    ssize_t cnt = 0;
    char buf[256] = {};
    int32_t jlength = env->GetArrayLength(jdelayArr);

    if (jlength <= 0 ) {
        return;
    }

    int fd = get_thread_delay_fd();
    if (fd <= 0) {
        return;
    }

    cnt = read(fd, buf, sizeof(buf));
    if (cnt >= (ssize_t) sizeof(binder_delay_struct)) {
        uint64_t version = 0;
        int64_t delay[DELAY_NUMS] = {};
        memcpy(&version, buf, sizeof(uint64_t));
        struct binder_delay_struct d = {};
        memcpy(&d, buf, sizeof(binder_delay_struct));
        delay[BINDER_PID_INDEX] = (pid_t) d.pid;
        delay[BINDER_TARGET_PID_INDEX] = (pid_t) d.binder_target_pid;
        delay[BINDER_TARGET_TID_INDEX] = (pid_t) d.binder_target_tid;
        delay[DELAY_TARGET_BLKIO_INDEX] = (int64_t) d.blkio_delay / 1000000;
        delay[DELAY_TARGET_SWAPIN_INDEX] = (int64_t) d.swapin_delay / 1000000;
        delay[DELAY_TARGET_FREEPAGES_INDEX] = (int64_t) d.freepages_delay / 1000000;
        delay[DELAY_TARGET_RUNNING_TIME_INDEX] = (int64_t) d.cpu_runtime / 1000000;
        delay[DELAY_TARGET_RUNNABLE_TIME_INDEX] = (int64_t) d.cpu_run_delay / 1000000;
        delay[DELAY_TARGET_CPU_UTIME_INDEX] = (int64_t) d.cpu_utime / 1000000;
        delay[DELAY_TARGET_CPU_STIME_INDEX] = (int64_t) d.cpu_stime / 1000000;
        delay[DELAY_TARGET_BINDER_THREAD_FULL] = d.binder_target_thread_full;

        if (jlength <= DELAY_NUMS){
            env->SetLongArrayRegion(jdelayArr, 0, jlength, delay);
            env->SetObjectArrayElement(jdelayInfo, 0, env->NewStringUTF(d.binder_target_comm));
        }
    }
    return;
}

static const JNINativeMethod methods[] = {
        {"nGetBinderDelay","([J[Ljava/lang/String;)V", (void *)android_os_BinderMonitor_GetBinderDelay},
};

static const char* const kClassPathName = "android/os/perfdebug/BinderMonitorImpl";

jboolean register_android_os_BinderMonitor(JNIEnv* env)
{
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class android.os.perfdebug.BinderMonitorImpl");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
