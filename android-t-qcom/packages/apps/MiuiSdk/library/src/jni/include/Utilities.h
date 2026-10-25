/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#ifndef _MIUI_UTILITIES_H_
#define _MIUI_UTILITIES_H_

#include <jni.h>

namespace miui {

// Generates shorty from signature of a method.
// The generated shorty should be release by using "delete []".
char* generateShorty(const char* signature, int *pLength);

// throw new exceptions
void throwNew(JNIEnv *env, const char *className, const char *fmt, ...);
void throwNew(JNIEnv *env, jclass clazz, const char *fmt, ...);

enum {
    JVM_UNKOWN = -1,
    JVM_DALVIK = 0,
    JVM_ART
};

int getRuntimeType();

bool initUtilities(JNIEnv *env);
int  jobject2HashCode(JNIEnv *env, jobject obj);

int getPlatformVersion();
}

#endif//_MIUI_UTILITIES_H_

