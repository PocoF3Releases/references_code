/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#ifndef _MIUI_NATIVE_H_
#define _MIUI_NATIVE_H_

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#if defined(WIN32) || defined(_WIN32)
#define UNUSED(x)
#else
#define UNUSED(x) UNUSED_##x __attribute__((unused))
#endif

#define LOG_TAG "com_miui_internal_os_Native-JNI"

#include <jni.h>

namespace miui {

#define JAVA_CLASS_NAME "com/miui/internal/os/Native"

int RegisterReflectNatives(JNIEnv *env, jclass clazz);

int RegisterFileNatives(JNIEnv *env, jclass clazz);

int RegisterSystemPropertiesNatives(JNIEnv *env, jclass clazz);

}

#endif//_MIUI_NATIVE_H_

