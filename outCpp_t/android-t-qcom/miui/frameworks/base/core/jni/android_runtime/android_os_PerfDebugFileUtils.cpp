#define LOG_TAG "FileUtils"

#include <inttypes.h>
#include <utils/Log.h>
#include <sys/time.h>
#include <stdint.h>
#include <pthread.h>

#include "jni.h"
#include "core_jni_helpers.h"
#include <fcntl.h>

#define EXCEPTION_LOG_MAX 1024

using namespace android;

static pthread_once_t initflag = PTHREAD_ONCE_INIT;
static pthread_key_t pthread_key;
static volatile bool key_created = false;

static void destroy_exception_log_fd(void *data) {
    int fd = (int)(long)data;
    if (fd > 0) {
        close(fd);
    }
}

static void create_thread_key() {
    int rc = pthread_key_create(&pthread_key, destroy_exception_log_fd);
    if (rc == 0) {
        key_created = true;
    } else {
        key_created = false;
    }
}

static int get_exception_log_fd(bool ismainthread) {
    if (!key_created) {
        pthread_once(&initflag, create_thread_key);
    }
    int exception_log_fd = (int)(long)pthread_getspecific(pthread_key);
    if (exception_log_fd == 0) {
        char path[128] = {};
        snprintf(path, sizeof(path), "/dev/mi_exception_log");
        exception_log_fd = open(path, O_RDWR|O_CLOEXEC|O_NONBLOCK);
        if (exception_log_fd < 0) {
            ALOGE("err open mi_exception_log errno=%d", errno);
        }
        if(ismainthread) {
            pthread_setspecific(pthread_key, (void *)(long)exception_log_fd);
        }
    }
    return exception_log_fd;
}

static void android_os_PerfDebugFileUtils_writeExceptionLog(JNIEnv* env, jclass, jstring jExceptionInfo) {
    int pid = getpid();
    int tid = gettid();
    bool ismainthread = (pid == tid);
    int fd = get_exception_log_fd(ismainthread);
    if (fd <= 0) {
        ALOGE("err write to mi_exception_log");
        return;
    }
    int length = env->GetStringUTFLength(jExceptionInfo);
    if (length > EXCEPTION_LOG_MAX) {
        ALOGE("can not write too long string to mi_exception_log");
        return;
    }
    const char* buf = env->GetStringUTFChars(jExceptionInfo, NULL);
    if (buf == NULL) {
        ALOGE("can not write null string to mi_exception_log");
        return;
    }

    write(fd, buf, length);
    if(!ismainthread && fd > 0) {
        close(fd);
    }
}

static const JNINativeMethod methods[] = {
    {"nWriteExceptionLog","(Ljava/lang/String;)V", (void *)android_os_PerfDebugFileUtils_writeExceptionLog},
};

static const char* const kClassPathName = "android/os/perfdebug/FileUtils";

jboolean register_android_os_PerfDebugFileUtils(JNIEnv* env)
{
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class android.os.perfdebug.FileUtils");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
