#define LOG_TAG "LIBSEC"
#define ALOGD (...)
#include <utils/Log.h>
#include <jni.h>
#include <utils/misc.h>
#include <nativehelper/JNIHelp.h>
#include <string.h>

#include "include/miui_security_SecurityManager.h"
#include "include/socket_hook.h"
#include "include/hookutils.h"

#ifdef PLATFORM_VERSION_GREATER_THAN_23

template <typename FunctionType>
static void init_function(void* handle, const char* symbol, FunctionType* function) {
    typedef void (*InitFunctionType)(FunctionType*);
    InitFunctionType initFunction = reinterpret_cast<InitFunctionType>(dlsym(handle, symbol));
    if (initFunction != NULL) {
        initFunction(function);
    }
}

#ifdef USE_WRAPPER_QC
#define SOCKET_CLIENT_LIB_NAME "libvendorconn.so"
#define SOCKET_INIT_FUNC_HANDLE_NAME "propClientInitSocket"
#else //USE_WRAPPER_QC
#define SOCKET_CLIENT_LIB_NAME "libnetd_client.so"
#define SOCKET_INIT_FUNC_HANDLE_NAME "netdClientInitSocket"
#endif //USE_WRAPPER_QC

static void init_hook() {
    ALOGI("apply sec hook advanced");
    void* netdClientHandle = dlopen(SOCKET_CLIENT_LIB_NAME, RTLD_NOW);
    if (netdClientHandle == NULL) {
        ALOGE("apply sec hook failed");
        return;
    }
    ptr_t tmp_socket = (ptr_t) ___socket;
    init_function(netdClientHandle, SOCKET_INIT_FUNC_HANDLE_NAME, &tmp_socket);
}

#else //PLATFORM_VERSION_GREATER_THAN_23

static hook_symbol_t HOOK_SYMBOLS[] = { { "socket", (ptr_t) ___socket, (ptr_t) real_socket_ptr }, };

static void init_hook() {
    ALOGI("apply sec hook legacy");
    hook_symbol_recursive(HOOK_SYMBOLS, sizeof(HOOK_SYMBOLS) / sizeof(HOOK_SYMBOLS[0]));
}

#endif //PLATFORM_VERSION_GREATER_THAN_23

void JNICALL miui_security_SecurityManager_hook(JNIEnv *env __unused, jclass cls __unused) {
    init_hook();
}

void JNICALL Java_miui_security_SecurityInjectManager_blockSelfNetwork
  (JNIEnv *env __unused, jclass cls __unused, jboolean enable) {
    ALOGI("blockSelfNetwork:%d", enable);
    block_self_network((int) enable);
}

jboolean JNICALL Java_miui_security_SecurityInjectManager_isNetworkBlocked
  (JNIEnv *env __unused, jclass cls __unused) {
    jboolean ret = (jboolean) is_block_self_network();
    ALOGI("isNetworkBlocked:%d", ret);
    return ret;
}

jboolean JNICALL Java_miui_security_SecurityInjectManager_hookFunctions
  (JNIEnv *env __unused, jclass cls __unused, jlong functionSetPtr, jint size) {
    ALOGI("hookFunctions, size:%d", size);
    hook_symbol_t *symbols = (hook_symbol_t*) functionSetPtr;
    append_hook_symbol_recursive(symbols, size);
    for(int i = 0; i < size; i++) {
        if(!strcmp(symbols[i].symbol, "socket")) {
            set_shandow_socket((int (*)(int, int, int)) symbols[i].ptr);
            break;
        }
    }
    return JNI_TRUE;
}

static JNINativeMethod gSecurityManagerMethods[] = {
    { "hook", "()V", (void*)miui_security_SecurityManager_hook },
};

static JNINativeMethod gSecurityInjectManagerMethods[] = {
    { "blockSelfNetwork", "(Z)V", (void*)Java_miui_security_SecurityInjectManager_blockSelfNetwork },
    { "isNetworkBlocked", "()Z", (void*)Java_miui_security_SecurityInjectManager_isNetworkBlocked },
    { "hookFunctions", "(JI)Z", (void*)Java_miui_security_SecurityInjectManager_hookFunctions },
};

int register_native_methods(JNIEnv* env) {
    int res = jniRegisterNativeMethods(env, "miui/security/SecurityManager",
        gSecurityManagerMethods, NELEM(gSecurityManagerMethods));
    LOG_FATAL_IF(res < 0, "Unable to register native methods in SecurityManager.");

    res = jniRegisterNativeMethods(env, "miui/security/SecurityInjectManager",
        gSecurityInjectManagerMethods, NELEM(gSecurityInjectManagerMethods));
    LOG_FATAL_IF(res < 0, "Unable to register native methods in SecurityInjectManager.");

    return 0;
}

#include <dlfcn.h>
extern "C"  jint JNI_OnLoad(JavaVM* vm, void* reserved __attribute__ ((unused))) {
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_native_methods(env);
    return JNI_VERSION_1_4;
}
