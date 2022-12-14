//
// Created by zengjing on 17-9-14.
//

#define LOG_TAG "ExtMediaBase_JNI"

#include <nativehelper/JNIHelp.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>

using namespace android;

// ----------------------------------------------------------------------------
extern int register_android_media_Xlog(JNIEnv *env);

jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
{
    JNIEnv* env = NULL;
    jint result = -1;
    ALOGE("JNI_OnLoad");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }

    if (register_android_media_Xlog(env) != JNI_OK) {
        ALOGE("ERROR: MiuiXlog native registration failed");
        goto bail;
    }
    result = JNI_VERSION_1_4;

bail:
    return result;
}

