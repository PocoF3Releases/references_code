#ifndef __POWERKEEPER_MILLET_QUERY
#define __POWERKEEPER_MILLET_QUERY
#include <nativehelper/JNIHelp.h>
#include "jni.h"

#define LOG_TAG "PowerKeeper-JNI"
#include <limits.h>
#include <android_runtime/AndroidRuntime.h>
#include <cstring>
#include "android_helper.h"
#include <dlfcn.h>

#ifdef HAVE_MILLET_H
#include "millet.h"
#include "milletApi.h"
#endif

#include <android/log.h>

using namespace android;
#ifndef NELEM
#define NELEM(x0 ((int)(sizeof(x)/sizeof((x)[0])))
#endif

#define LOGDD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#ifdef HAVE_MILLET_H
static millet_sock recv_sock;
#endif

int com_miui_powerkeeper_Millet_init(JNIEnv *env, jobject clazz) {
    //load so library
#ifndef HAVE_MILLET_H
    LOGDD("jni millet no kernel millet.h");
    return -2;
#else
    if (create_millet_sock(&recv_sock) == MILLET_ERR) {
        LOGDD("MILLET_USER: create listen sock failed %s\n",
                strerror(errno));
        return MILLET_ERR;
    }
    LOGDD("com_miui_powerkeeper_Millet_init OK");
    return MILLET_OK;
#endif
}

static int com_miui_powerkeeper_Millet_queryBinderState(JNIEnv *env, jobject clazz, jint uid) {
#ifndef HAVE_MILLET_H
    return -1;
#else
    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = BINDER_ST_TYPE;
    data.mod.u_priv.binder_st.uid = uid;
    if (millet_sendmsg(&recv_sock, BINDER_ST_TYPE, &data) == MILLET_OK) {
        printf("com_miui_powerkeeper_Millet_queryBinderState OK uid =%d\n", uid);
    }
    else {
        LOGDD("com_miui_powerkeeper_Millet_queryBinderState ERR uid =%d\n", uid);
        return -1;
    }
    return 0;
#endif
}

int com_miui_powerkeeper_Millet_setPackageUid(JNIEnv *env, jobject clazz, jint uid, jint cmd) {
#ifndef HAVE_MILLET_H
    return -1;
#else
    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = PKG_TYPE;
    data.mod.u_priv.pkg.uid = uid;
    enum PKG_CMD CMD = ((cmd == -2) ? CLEAR_ALL_UID : ((cmd == -1) ? DEL_UID : ADD_UID));
    data.mod.u_priv.pkg.cmd = CMD;
    if (millet_sendmsg(&recv_sock, PKG_TYPE, &data) == MILLET_OK) {
        printf("com_miui_powerkeeper_Millet_setPackageUid OK uid =%d\n", uid);
    }
    else {
        LOGDD("com_miui_powerkeeper_Millet_setPackageUid  ERR uid =%d\n", uid);
        return -1;
    }
    return 0;
#endif
}

int com_miui_powerkeeper_Millet_handshake(JNIEnv *env, jobject clazz) {
#ifndef HAVE_MILLET_H
    return -1;
#else
    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = HANDSHK_TYPE;
    if (millet_sendmsg(&recv_sock, HANDSHK_TYPE, &data) == MILLET_OK) {
        printf("com_miui_powerkeeper_Millet_handshake OK\n");
    }
    else {
        LOGDD("com_miui_powerkeeper_Millet_handshake  ERR\n");
        return -1;
    }
    return 0;
#endif
}

static const JNINativeMethod methods[] = {
    {"nativeInit", "()I", (void *)com_miui_powerkeeper_Millet_init},
    {"nativeQuery", "(I)I", (void *)com_miui_powerkeeper_Millet_queryBinderState},
    {"nativeSet", "(II)I", (void *)com_miui_powerkeeper_Millet_setPackageUid},
    {"nativeHandShake", "()I", (void *)com_miui_powerkeeper_Millet_handshake}
};

static const char* const kClassPathName = "com/miui/powerkeeper/controller/FrozenAppController";

int register_com_miui_powerkeeper_Millet(JNIEnv *env) {
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class Millet");

    if (env->RegisterNatives(clazz, methods, NELEM(methods)) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return -1;
    }
    return 0;
}
#endif
