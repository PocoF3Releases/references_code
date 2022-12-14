/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#include <stdlib.h>
#include <string.h>

#include <cutils/properties.h>
#ifndef BUILD_FOR_NDK
#include <utils/misc.h>
#endif
#include <utils/Log.h>

#include <jni.h>

#include "Native.h"

namespace miui
{

static int getProperty(JNIEnv *env, jstring key, char *buf){
    const char *k = env->GetStringUTFChars(key, NULL);
    int len = property_get(k, buf, NULL);
    if (len <= 0) {
        buf[0] = 0;
        len = 0;
    }
    env->ReleaseStringUTFChars(key, k);
    return len;
}

static jstring com_miui_internal_os_Native_get(JNIEnv *env, jclass, jstring key)
{
    char buf[PROPERTY_VALUE_MAX];
    if (getProperty(env, key, buf) != 0) {
        return env->NewStringUTF(buf);
    }
    return NULL;
}

static jint com_miui_internal_os_Native_getInt(JNIEnv *env, jclass, jstring key, jint def)
{
    char buf[PROPERTY_VALUE_MAX];
    char *end;
    jint result = def;

    if (getProperty(env, key, buf) != 0) {
        result = strtol(buf, &end, 0);
        if (end == buf) {
            result = def;
        }
    }

    return result;
}

static jlong com_miui_internal_os_Native_getLong(JNIEnv *env, jclass, jstring key, jlong def)
{
    char buf[PROPERTY_VALUE_MAX];
    char *end;
    jlong result = def;

    if (getProperty(env, key, buf) != 0) {
        result = strtoll(buf, &end, 0);
        if (end == buf) {
            result = def;
        }
    }

    return result;
}

static jboolean com_miui_internal_os_Native_getBoolean(JNIEnv *env, jclass, jstring key, jboolean def)
{
    char buf[PROPERTY_VALUE_MAX];
    char *end;
    jboolean result = def;

    int len = getProperty(env, key, buf);
    switch (len) {
        case 1:
            if (buf[0] == '0' || buf[0] == 'n') {
                result = JNI_FALSE;
            } else if (buf[0] == '1' || buf[0] == 'y') {
                result = JNI_TRUE;
            }
            break;
        case 2:
            if (buf[0] == 'n' && buf[1] == 'o') {
                result = JNI_FALSE;
            } else if (buf[0] == 'o' && buf[1] == 'n') {
                result = JNI_TRUE;
            }
            break;
        case 3:
            if (buf[0] == 'o' && buf[1] == 'f' && buf[2] == 'f') {
                result = JNI_FALSE;
            } else if (buf[0] == 'y' && buf[1] == 'e' && buf[2] == 's') {
                result = JNI_TRUE;
            }
            break;
        case 4:
            if (memcmp(buf, "true", 4) == 0) {
                result = JNI_TRUE;
            }
            break;
        case 5:
            if (memcmp(buf, "false", 5) == 0) {
                result = JNI_FALSE;
            }
            break;
    }
    return result;
}

static void com_miui_internal_os_Native_set(JNIEnv *env, jclass, jstring key, jstring val)
{
    int err;
    const char *k, *v;

    k = env->GetStringUTFChars(key, NULL);
    if (val == NULL) {
        v = "";       /* NULL pointer not allowed here */
    } else {
        v = env->GetStringUTFChars(val, NULL);
    }

    err = property_set(k, v);

    env->ReleaseStringUTFChars(key, k);

    if (val != NULL) {
        env->ReleaseStringUTFChars(val, v);
    }

    if (err < 0) {
        env->ThrowNew(env->FindClass("java/lang/RuntimeException"),
                          "failed to set system property");
    }
}

static JavaVM* sVM = NULL;
static jclass sClazz = NULL;
static jmethodID sCallChangeCallbacks;

static void do_report_sysprop_change() {
    if (sVM != NULL && sClazz != NULL) {
        JNIEnv* env;
        if (sVM->GetEnv((void **)&env, JNI_VERSION_1_4) >= 0) {
            env->CallStaticVoidMethod(sClazz, sCallChangeCallbacks);
        }
    }
}

static void com_miui_internal_os_Native_addChangeCallback(JNIEnv *env, jclass, jclass clazz)
{
#ifndef BUILD_FOR_NDK
    // This is called with the Java lock held.
    if (sVM == NULL) {
        env->GetJavaVM(&sVM);
    }
    if (sClazz == NULL) {
        sClazz = (jclass) env->NewGlobalRef(clazz);
        sCallChangeCallbacks = env->GetStaticMethodID(sClazz, "callChangeCallbacks", "()V");
        android::add_sysprop_change_callback(do_report_sysprop_change, -10000);
    }
#endif
}

static JNINativeMethod methods[] = {
    { "getPropertyNative", "(Ljava/lang/String;)Ljava/lang/String;",
      (void*) com_miui_internal_os_Native_get },
    { "getIntPropertyNative", "(Ljava/lang/String;I)I",
      (void*) com_miui_internal_os_Native_getInt },
    { "getLongPropertyNative", "(Ljava/lang/String;J)J",
      (void*) com_miui_internal_os_Native_getLong },
    { "getBooleanPropertyNative", "(Ljava/lang/String;Z)Z",
      (void*) com_miui_internal_os_Native_getBoolean },
    { "setPropertyNative", "(Ljava/lang/String;Ljava/lang/String;)V",
      (void*) com_miui_internal_os_Native_set },
    { "addPropertyChangeCallbackNative", "(Ljava/lang/Class;)V",
      (void*) com_miui_internal_os_Native_addChangeCallback },
};

int RegisterSystemPropertiesNatives(JNIEnv *env, jclass clazz) {
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]))) {
        ALOGE("RegisterNatives fialed for '%s' for reflect methods", JAVA_CLASS_NAME);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

};

