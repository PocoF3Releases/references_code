#define LOG_TAG "BaseLooper"

#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>
#include <fcntl.h>

#include "jni.h"
#include "core_jni_helpers.h"

#define DELAY_VERSION 1
#define DELAY_NUMS 5
#define BLKIO_DELAY_INDEX 0
#define SWAPIN_DELAY_INDEX 1
#define FREEPAGES_DELAY_INDEX 2
#define CPU_RUNTIME_INDEX    3
#define CPU_RUN_DELAY_INDEX   4

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
        snprintf(path, sizeof(path), "/proc/self/task/%d/delay", gettid());
        thread_delay_fd = open(path, O_RDONLY|O_CLOEXEC);
        pthread_setspecific(pthread_key, (void *)(long)thread_delay_fd);
    }
    return thread_delay_fd;
}

struct delay_struct {
    uint64_t version;
    uint64_t blkio_delay;     /* wait for sync block io completion */
    uint64_t swapin_delay;    /* wait for swapin block io completion */
    uint64_t freepages_delay; /* wait for memory reclaim */
    uint64_t cpu_runtime;
    uint64_t cpu_run_delay;
};

static jlong android_os_BaseLooper_getThreadCpuTime(JNIEnv* env, jobject clazz) {
    struct timespec ts;
    int res = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    if (res != 0) {
        return (jlong)0;
    }

    nsecs_t when = seconds_to_nanoseconds(ts.tv_sec) + ts.tv_nsec;
    return (jlong)nanoseconds_to_milliseconds(when);
}

static void android_os_BaseLooper_getThreadDelay(JNIEnv* env, jclass, jlongArray jdelayArr) {
    int cnt = 0;
    char buf[128] = {};

    if (env->GetArrayLength(jdelayArr) < DELAY_NUMS) {
        ALOGE("Length of the jdelayArr must be great than or equal to %d", DELAY_NUMS);
        return;
    }

    int fd = get_thread_delay_fd();
    if (fd <= 0) {
        return;
    }

    cnt = read(fd, buf, sizeof(buf));

    if (cnt > 0) {
        struct delay_struct d = {};
        memcpy(&d, buf, cnt < (int) sizeof(d) ? cnt : sizeof(d));
        if (d.version >= DELAY_VERSION) {
            int64_t delay[DELAY_NUMS] = {
                    (int64_t) d.blkio_delay,
                    (int64_t) d.swapin_delay,
                    (int64_t) d.freepages_delay,
                    (int64_t) d.cpu_runtime,
                    (int64_t) d.cpu_run_delay,
            };
            env->SetLongArrayRegion(jdelayArr, 0, DELAY_NUMS, delay);
        }
    }
}

static const JNINativeMethod methods[] = {
        {"nGetThreadCpuTime","()J", (void *)android_os_BaseLooper_getThreadCpuTime},
        {"nGetThreadDelay","([J)V", (void *)android_os_BaseLooper_getThreadDelay},
};

int register_android_os_BaseLooper(JNIEnv* env) {
    return RegisterMethodsOrDie(env, "android/os/BaseLooper", methods, NELEM(methods));
}
