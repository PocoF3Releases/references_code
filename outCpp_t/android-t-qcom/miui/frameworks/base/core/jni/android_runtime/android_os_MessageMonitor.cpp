#define LOG_TAG "MessageMonitor"

#include <utils/Log.h>
#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>

#include "jni.h"
#include "core_jni_helpers.h"
#include <fcntl.h>

#define DELAY_VERSION_1         1
#define DELAY_VERSION_2         2

#define BLKIO_DELAY_INDEX       0
#define SWAPIN_DELAY_INDEX      1
#define FREEPAGES_DELAY_INDEX   2
#define CPU_RUNTIME_INDEX       3
#define CPU_RUN_DELAY_INDEX     4
#define CPU_UTIME_INDEX         5
#define CPU_STIME_INDEX         6
#define BINDER_INDEX            7
#define SLOW_PATH_INDEX         8
#define DELAY_NUMS              9
#define TASK_DELAY              0
#define MILOG_DELAY             1

using namespace android;

static pthread_once_t initflag = PTHREAD_ONCE_INIT;
static pthread_key_t pthread_key;
static volatile bool key_created = false;
static volatile int critical_task_initialize = -1;

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

static void initialize_critical_task() {
    char path[] = "/proc/mi_log/delay";
    int fd = open(path, O_RDONLY|O_CLOEXEC);
    if (fd < 0) {
        critical_task_initialize = TASK_DELAY;
    } else {
        critical_task_initialize = MILOG_DELAY;
        close(fd);
    }
}

static int get_thread_delay_fd() {
    if (!key_created) {
        pthread_once(&initflag, create_thread_key);
    }
    if (critical_task_initialize < 0) {
        initialize_critical_task();
    }
    int thread_delay_fd = (int)(long)pthread_getspecific(pthread_key);
    if (thread_delay_fd == 0) {
        char path[128] = {};
        if (critical_task_initialize == TASK_DELAY) {
            snprintf(path, sizeof(path), "/proc/self/task/%d/delay", gettid());
        } else if(critical_task_initialize == MILOG_DELAY) {
            snprintf(path, sizeof(path), "/proc/mi_log/delay");
        }
        thread_delay_fd = open(path, O_RDONLY|O_CLOEXEC);
        pthread_setspecific(pthread_key, (void *)(long)thread_delay_fd);
    }
    return thread_delay_fd;
}

struct delay_struct_v1 {
    uint64_t version;
    uint64_t blkio_delay;     /* wait for sync block io completion */
    uint64_t swapin_delay;    /* wait for swapin block io completion */
    uint64_t freepages_delay; /* wait for memory reclaim */
    uint64_t cpu_runtime;
    uint64_t cpu_run_delay;
};

struct delay_struct_v2 {
    uint64_t version;
    uint64_t blkio_delay;     /* wait for sync block io completion */
    uint64_t swapin_delay;    /* wait for swapin block io completion */
    uint64_t freepages_delay; /* wait for memory reclaim */
    uint64_t cpu_runtime;
    uint64_t cpu_run_delay;
    uint64_t utime;
    uint64_t stime;
};

static jlong android_os_MessageMonitor_getThreadCpuTime(JNIEnv* env, jobject clazz) {
    struct timespec ts;
    int res = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    if (res != 0) {
        return (jlong)0;
    }

    nsecs_t when = seconds_to_nanoseconds(ts.tv_sec) + ts.tv_nsec;
    return (jlong)nanoseconds_to_milliseconds(when);
}

static void android_os_MessageMonitor_getThreadDelay(JNIEnv* env, jclass, jlongArray jdelayArr) {
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

    if (cnt >= (ssize_t) sizeof(delay_struct_v1)) {
        uint64_t version = 0;
        int64_t delay[DELAY_NUMS] = {};
        memcpy(&version, buf, sizeof(uint64_t));
        if (version == 1) {
            struct delay_struct_v1 d = {};
            memcpy(&d, buf, sizeof(delay_struct_v1));
            delay[BLKIO_DELAY_INDEX] = (int64_t) d.blkio_delay;
            delay[SWAPIN_DELAY_INDEX] = (int64_t) d.swapin_delay;
            delay[FREEPAGES_DELAY_INDEX] = (int64_t) d.freepages_delay;
            delay[CPU_RUNTIME_INDEX] = (int64_t) d.cpu_runtime;
            delay[CPU_RUN_DELAY_INDEX] = (int64_t) d.cpu_run_delay;
        } else if (version >= 2) {
            struct delay_struct_v2 d = {};
            memcpy(&d, buf, sizeof(delay_struct_v2));
            delay[BLKIO_DELAY_INDEX] = (int64_t) d.blkio_delay / 1000000;
            delay[SWAPIN_DELAY_INDEX] = (int64_t) d.swapin_delay / 1000000;
            delay[FREEPAGES_DELAY_INDEX] = (int64_t) d.freepages_delay / 1000000;
            delay[CPU_RUNTIME_INDEX] = (int64_t) d.cpu_runtime / 1000000;
            delay[CPU_RUN_DELAY_INDEX] = (int64_t) d.cpu_run_delay / 1000000;
            delay[CPU_UTIME_INDEX] = (int64_t) d.utime / 1000000;
            delay[CPU_STIME_INDEX] = (int64_t) d.stime / 1000000;
        }
        if (jlength <= DELAY_NUMS){
            env->SetLongArrayRegion(jdelayArr, 0, jlength, delay);
        }
    }
}

static const JNINativeMethod methods[] = {
        {"nGetThreadCpuTime","()J", (void *)android_os_MessageMonitor_getThreadCpuTime},
        {"nGetThreadDelay","([J)V", (void *)android_os_MessageMonitor_getThreadDelay},
};

static const char* const kClassPathName = "android/os/perfdebug/MessageMonitorImpl";

jboolean register_android_os_MessageMonitor(JNIEnv* env)
{
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class android.os.perfdebug.MessageMonitorImpl");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
