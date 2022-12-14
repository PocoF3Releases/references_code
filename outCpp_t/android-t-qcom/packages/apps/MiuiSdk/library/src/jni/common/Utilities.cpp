/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#define LOG_TAG "com_miui_Common-JNI"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include "Utilities.h"

namespace miui {

char* generateShorty(const char* signature, int *pLength) {
    char* shorty = 0;
    int length = 1;

    for (const char *p = signature + 1; *p != ')'; ++p, ++length) {
        if (*p == '[') {
            while (*++p == '[');
        }
        if (*p == 'L') {
            while (*++p != ';');
        }
    }

    shorty = new char[length + 1];
    length = 1;

    for (signature++; *signature != ')'; ++signature, ++length) {
        if (*signature == '[') {
            shorty[length] = '[';
            while (*++signature == '[');
            if (*signature == 'L') {
                while (*++signature != ';');
            }
        } else if (*signature == 'L') {
            shorty[length] = 'L';
            while (*++signature != ';');
        } else {
            shorty[length] = *signature;
        }
    }

    ++signature;
    shorty[0] = *signature;
    shorty[length] = 0;

    if (pLength != 0) {
        *pLength = length;
    }
    return shorty;
}

void throwNew(JNIEnv *env, const char *className, const char *fmt, ...) {
    char buffer[1024];
    va_list ap;
    jclass clazz;

    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap);
    va_end(ap);
    env->ExceptionClear();
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        throwNew(env, "java/lang/ClassNotFoundException", "Couldn't find class %s", className);
    }
    env->ThrowNew(clazz, buffer);
}

void throwNew(JNIEnv *env, jclass clazz, const char *fmt, ...) {
    char buffer[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer) - 1, fmt, ap);
    va_end(ap);
    env->ExceptionClear();
    env->ThrowNew(clazz, buffer);
}


static int sjvm_type = JVM_UNKOWN;
static jmethodID sObject_hashCode = 0;

int getRuntimeType() {
    return sjvm_type;
}

int getPlatformVersion() {
    static int _version = 0;
    if (_version == 0) {
        char szversion[PROPERTY_VALUE_MAX];
        property_get("ro.build.version.sdk", szversion, "0");
        _version = atoi(szversion);
    }

    return _version;
}

static const char* vm_prop_names[] = {
    "persist.sys.dalvik.vm.lib.2", //in android L
    "persist.sys.dalvik.vm.lib", //in 4.4
};

static const char vm_default[] = "unknown";

bool initUtilities(JNIEnv *env) {

    sjvm_type = getPlatformVersion() > 19 ? JVM_ART : JVM_DALVIK;

    char default_library[PROPERTY_VALUE_MAX];
    for (size_t i = 0; i < sizeof(vm_prop_names) / sizeof(vm_prop_names[0]); i ++) {
        property_get(vm_prop_names[i], default_library, vm_default);
        if (strncmp(default_library, "libart", 6) == 0) {
            sjvm_type = JVM_ART;
        } else if (strncmp(default_library, "libdvm", 6) == 0) {
            sjvm_type = JVM_DALVIK;
        } else {
            continue;
        }
        break;
    }

    jclass clazz = env->FindClass("java/lang/Object");
    sObject_hashCode = env->GetMethodID(clazz, "hashCode", "()I");

    return true;
}

int jobject2HashCode(JNIEnv *env, jobject obj) {
    return obj ? env->CallIntMethod(obj, sObject_hashCode) : 0;
}


}

