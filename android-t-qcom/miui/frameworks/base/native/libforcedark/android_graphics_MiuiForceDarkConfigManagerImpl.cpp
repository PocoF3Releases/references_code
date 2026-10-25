#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "MiuiForceDarkConfig_JNI"
#include "MiuiForceDarkConfigImpl.h"
#include <utils/Log.h>
#include <jni.h>
#include <nativehelper/JNIHelp.h>

using namespace android;

static jlong nativeInit(JNIEnv* env, jclass clazz, jobject configManagerObj) {
    MiuiForceDarkConfigImpl* miuiForceDarkConfig = MiuiForceDarkConfigImpl::getInstance();
    miuiForceDarkConfig->incStrong(0);
    return reinterpret_cast<jlong>(miuiForceDarkConfig);
}

static void setConfig(JNIEnv* env, jobject clazz, jlong configPtr, jfloat density, jint mainRule,
                      jint secondaryRule, jint tertiaryRule) {
    reinterpret_cast<MiuiForceDarkConfigImpl*>(configPtr)->setConfig(density, mainRule, secondaryRule,
                tertiaryRule);
}

static const JNINativeMethod methods[] = {
        {"nativeInit",
         "(Landroid/graphics/MiuiForceDarkConfigManagerImpl;)J",
         (void*)nativeInit},
        {"nativeSetConfig","(JFIII)V", (void *)setConfig},
};

static int registerNativeMethods(JNIEnv* env, const char* className,
    const JNINativeMethod* gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'", className);
        return JNI_ERR;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        ALOGE("RegisterNatives failed for '%s'", className);
        return JNI_ERR;
    }
    return JNI_OK;
}

int register_android_graphics_MiuiForceDarkConfigManagerImpl(JNIEnv *env) {
    return registerNativeMethods(env, "android/graphics/MiuiForceDarkConfigManagerImpl",
                                 methods, NELEM(methods));
}