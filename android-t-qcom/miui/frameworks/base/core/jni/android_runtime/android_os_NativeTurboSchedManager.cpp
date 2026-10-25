#define LOG_TAG "NativeTurboSchedManagerJni"
#include <utils/Log.h>
#include <linux/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include "jni.h"
#include "core_jni_helpers.h"

struct metis_data {
    unsigned int pid;
    unsigned int boost_sec;
};

#define METIS_CMD_FUNC_TASK_BOOST _IOW('U', 0x07, struct metis_data)

static jint nativeOpenDevice(JNIEnv* env, jobject, jstring devName)
{
    const char *rawDevName = env->GetStringUTFChars(devName, NULL);
    int handle = open(rawDevName, O_RDWR);

    if (handle < 0) {
        ALOGE("open %s failed!\n", rawDevName);
    }

    env->ReleaseStringUTFChars(devName, rawDevName);
    return handle;
}

static void nativeCloseDevice(JNIEnv*, jobject, jint handle)
{
    if (handle >= 0) {
        close(handle);
    }
}

static void nativeIoctlDevice(JNIEnv*, jobject, jint handle, jint cmd)
{
    if (handle >= 0) {
        ioctl(handle, cmd, 0);
    }
}

static void nativeTaskBoost(JNIEnv*, jobject, jint handle, jint pid, jint boost_sec)
{
    struct metis_data data;
    data.pid = pid;
    data.boost_sec = boost_sec;

    if (handle >= 0) {
        ioctl(handle, METIS_CMD_FUNC_TASK_BOOST, &data);
    }
}

static const char *classPathName = "android/os/NativeTurboSchedManager";

static const JNINativeMethod sMethods[] = {
    {"nativeOpenDevice", "(Ljava/lang/String;)I", (void *)nativeOpenDevice},
    {"nativeCloseDevice", "(I)V", (void *)nativeCloseDevice},
    {"nativeIoctlDevice", "(II)V", (void *)nativeIoctlDevice},
    {"nativeTaskBoost", "(III)V", (void *)nativeTaskBoost},
};

int register_android_os_NativeTurboSchedManager(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, classPathName, sMethods, NELEM(sMethods));
}

