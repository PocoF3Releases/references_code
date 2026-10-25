#ifndef LOG_TAG
#define LOG_TAG "JNI_GREEZE"
#endif

#include "jni.h"
#include "core_jni_helpers.h"
#include <stdio.h>
#include <string.h>
#include <linux/socket.h>
#include <linux/netlink.h>
#include <netinet/in.h>

#include "utils/Log.h"


#ifdef HAS_MILLET_H
#include "../../../cmds/millet/include/millet.h"
#include "../../../cmds/millet/include/milletApi.h"
#endif

using namespace android;

#ifdef HAS_MILLET_H
static millet_sock m_sock;

static int32_t create_bind_socket() {
    if (create_millet_sock(&m_sock) == MILLET_ERR) {
	    ALOGE("jni create socket failed!");
	    return MILLET_ERR;
    }
    return MILLET_OK;
}

static void init_msock() {
    ALOGI("jni init static m_sock");
    create_bind_socket();
}

#endif



static void com_miui_server_greeze_GreezeManagerService_AddConcernedUid
        (JNIEnv *env, jclass, jint uid) {
#ifdef HAS_MILLET_H
    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = PKG_TYPE;
    data.mod.u_priv.pkg.cmd = ADD_UID;
    data.mod.u_priv.pkg.uid = uid;
    if (millet_sendmsg(&m_sock, PKG_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }
#endif
}

static void com_miui_server_greeze_GreezeManagerService_DelConcernedUid(JNIEnv *env, jclass, jint uid) {
#ifdef HAS_MILLET_H
    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = PKG_TYPE;
    data.mod.u_priv.pkg.cmd = DEL_UID;
    data.mod.u_priv.pkg.uid = uid;
    if (millet_sendmsg(&m_sock, PKG_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }
#endif
}

static void com_miui_server_greeze_GreezeManagerService_ClearConcernedUid
        (JNIEnv *env, jclass) {
#ifdef HAS_MILLET_H
    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = PKG_TYPE;
    data.mod.u_priv.pkg.cmd = CLEAR_ALL_UID;
    if (millet_sendmsg(&m_sock, PKG_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }
#endif
}

static void com_miui_server_greeze_GreezeManagerService_QueryBinder
        (JNIEnv *env, jclass, jint uid) {
#ifdef HAS_MILLET_H
    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = BINDER_ST_TYPE;
    data.mod.u_priv.binder_st.uid = uid;
    if (millet_sendmsg(&m_sock, BINDER_ST_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }
#endif
}

static void com_miui_server_greeze_GreezeManagerService_LoopOnce
        (JNIEnv *env, jclass) {
    ALOGE("LoopOnce start");
#ifdef HAS_MILLET_H
    if (m_sock.sock == -1 || m_sock.in_use == 0)
        init_msock();

    if (m_sock.sock == -1) {
        ALOGE("create & bind socket fail");
        return;
    }

    struct millet_userconf data;
    data.msg_type = MSG_TO_KERN;
    data.owner = HANDSHK_TYPE;
    if (millet_sendmsg(&m_sock, HANDSHK_TYPE, &data) != MILLET_OK) {
        ALOGE("send msg error");
    }
    ALOGE("LoopOnce end");
#endif
}

static const JNINativeMethod methods[] = {
        {"nAddConcernedUid",   "(I)V", (void *) com_miui_server_greeze_GreezeManagerService_AddConcernedUid},
        {"nDelConcernedUid",   "(I)V", (void *) com_miui_server_greeze_GreezeManagerService_DelConcernedUid},
        {"nClearConcernedUid", "()V",  (void *) com_miui_server_greeze_GreezeManagerService_ClearConcernedUid},
        {"nQueryBinder",       "(I)V", (void *) com_miui_server_greeze_GreezeManagerService_QueryBinder},
        {"nLoopOnce",          "()V", (void *) com_miui_server_greeze_GreezeManagerService_LoopOnce},
};

static const char* const kClassPathName = "com/miui/server/greeze/GreezeManagerService";

jboolean register_com_miui_server_greeze_GreezeManagerService(JNIEnv* env)
{
    ALOGE(" start register_com_miui_server_greeze_GreezeManagerService");
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class com.miui.server.greeze.GreezeManagerService");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    ALOGE(" finish register_com_miui_server_greeze_GreezeManagerService");
    return JNI_TRUE;
}
