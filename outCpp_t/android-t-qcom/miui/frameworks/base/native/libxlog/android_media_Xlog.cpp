//
// Created by zengjing on 17-9-14.
//
//#define LOG_NDEBUG 0
#define LOG_TAG "MiuiAudioRecord_JNI"

#include <utils/Log.h>
#include <jni.h>
#include <string>
#include <android_runtime/AndroidRuntime.h>
#include <nativehelper/JNIHelp.h>
#include <stdlib.h>
#include "xlog_utils.h"

using namespace android;

static const char* const kClassPathName = "android/media/MiuiXlog";

static jint android_media_Xlog_xlogSend(JNIEnv *env, jobject, jbyteArray msg) {
    xlog_init();
    int msglen = (*env).GetArrayLength (msg);
    int len    = 512; // fixed value, ugly but subject to the buffer size in xlog.c:(
    if (msglen >= len)
        msglen = len -1; // reserve the last byte for '\0'
    char* buffer = (char*)malloc(sizeof(char)*len);
    if(nullptr == buffer) {
        ALOGE("alloc memory fail !");
        return -1;
    }
    memset(buffer, 0, sizeof(char)*len);
    (*env).GetByteArrayRegion (msg, 0, msglen, (jbyte*)buffer);
    int result = xlog_send_java(buffer);
    if(buffer) {
        free(buffer);
        buffer = nullptr;
    }
    xlog_deinit();
    return result;
}

static const JNINativeMethod gMethods[] = {
    { "native_xlogSend", "([B)I", (void *)android_media_Xlog_xlogSend },
};

int register_android_media_Xlog(JNIEnv *env) {
    ALOGE("register_android_media_Xlog");
    return AndroidRuntime::registerNativeMethods(env, kClassPathName, gMethods, NELEM(gMethods));
}
