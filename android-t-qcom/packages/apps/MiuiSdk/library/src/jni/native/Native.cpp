/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#include <stdio.h>

#include <utils/Log.h>

#include "Native.h"
#include "PrimitiveValues.h"
#include "Utilities.h"

namespace miui {

static int initialize(JNIEnv* env) {
    jclass proxyClass = env->FindClass(JAVA_CLASS_NAME);
    if (proxyClass == NULL) {
        ALOGE("Couldn't find class '%s'", JAVA_CLASS_NAME);
        return JNI_FALSE;
    }

    if (RegisterReflectNatives(env, proxyClass) == JNI_FALSE) {
        return JNI_FALSE;
    }

    if (RegisterFileNatives(env, proxyClass) == JNI_FALSE) {
        return JNI_FALSE;
    }

    if (RegisterSystemPropertiesNatives(env, proxyClass) == JNI_FALSE) {
        return JNI_FALSE;
    }

    if (!PrimitiveValues::Init(env)) {
        ALOGE("Couldn't initialize primitive value");
        return JNI_FALSE;
    }

    if (!initUtilities(env)) {
        ALOGE("Couldn't initialize utilities");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

}

jint JNI_OnLoad(JavaVM* vm, void* UNUSED(reserved)) {
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed");
        goto bail;
    }

    if (miui::initialize(env) != JNI_TRUE) {
        ALOGE("initialize failed");
        goto bail;
    }

    result = JNI_VERSION_1_4;

bail:
    return result;
}

