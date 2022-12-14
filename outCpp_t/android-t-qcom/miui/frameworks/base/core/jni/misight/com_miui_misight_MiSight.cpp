//
// Created by bai on 22-3-16.
//

#define LOG_TAG "MiSightJni"

#include <utils/Log.h>
#include <iostream>
#include "jni.h"
#include <dlfcn.h>
#include <nativehelper/JNIHelp.h>

#define LIB_MISIGHT_FULL_NAME  "libmisight.so"

using namespace android;

// For call libmisight.so
void *handle = NULL;
int (*send_event)(int , const std::string&) = NULL;

static jboolean Java_com_miui_misight_MiSight_nativeSendEvent(JNIEnv* env, jobject clazz, jint eventId, jstring event)
{
    const char *cmdStr = env->GetStringUTFChars(event, NULL);
    ALOGD("MiSight: Java_com_miui_misight_MiSight_nativeSendEvent enter called. '%s'", cmdStr);
    if (!handle) {
        ALOGE("MiSight: nativeSendEvent fail. !handle, because dlopen fail");
        env->ReleaseStringUTFChars(event, cmdStr);
        return JNI_FALSE;
    }

    if (!send_event) {
        ALOGE("MiSight: nativeSendEvent fail. !send_event, because dlsym fail");
        env->ReleaseStringUTFChars(event, cmdStr);
        return JNI_FALSE;
    }

    int ret = send_event(eventId, cmdStr);
    env->ReleaseStringUTFChars(event, cmdStr);
    if (ret < 0) {
        ALOGE("MiSight: nativeSendEvent fail. '%s'", cmdStr);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

static const JNINativeMethod methods[] = {
    { "nativeSendEvent",      "(ILjava/lang/String;)Z",      (void*)Java_com_miui_misight_MiSight_nativeSendEvent },
};

static const char* const kClassPathName = "com/miui/misight/MiSight";

jboolean registerNatives(JNIEnv* env)
{
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "MiSight: Unable to find class android.miui.Shell");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("MiSight: RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{

    jint result = -1;
    JNIEnv* env = NULL;

    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("MiSight: GetEnv failed");
        goto fail;
    }

    if (registerNatives(env) != JNI_TRUE) {
        goto fail;
    }

    result = JNI_VERSION_1_4;

    // For load libmisight.so
    handle = dlopen(LIB_MISIGHT_FULL_NAME, RTLD_NOW);
    if (!handle) {
        ALOGE("MiSight: JNI_OnLoad dlopen fail.");
    } else {
        send_event = (int (*)(int, const std::string&))dlsym(handle, "sendEventMisight");
        if (!send_event) {
            ALOGE("MiSight: JNI_OnLoad dlsym fail.");
        }
    }

fail:
    return result;
}