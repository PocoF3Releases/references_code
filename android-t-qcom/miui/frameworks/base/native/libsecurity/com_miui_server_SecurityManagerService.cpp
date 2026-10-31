#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "utils/Log.h"
#include <utils/misc.h>

#include <nativehelper/JNIHelp.h>
#include "jni.h"
#include "process.h"
#include "register.h"

namespace android {

static void com_miui_server_SecurityManagerService_nativeKillPackageProcesses(JNIEnv *env, jclass clazz, jint uid, jstring pkg) {
    if (pkg == NULL) {
        jniThrowNullPointerException(env, "pkg is null");
        return;
    }

    const char *pkgName = env->GetStringUTFChars(pkg, NULL);

    kill_processes_with_uid(uid, pkgName);
    env->ReleaseStringUTFChars(pkg, pkgName);
}

static jboolean com_miui_server_SecurityManagerService_nativeIsReleased(JNIEnv *env, jobject obj)
{
#ifdef MIUI_RELEASED
    return JNI_TRUE;
#endif
    return JNI_FALSE;
}

// -----------------------------------------------------------------------------------------------------

static JNINativeMethod gSecurityManagerServiceMethods[] = {
    { "nativeKillPackageProcesses", "(ILjava/lang/String;)V", (void*)com_miui_server_SecurityManagerService_nativeKillPackageProcesses },
    { "nativeIsReleased", "()Z", (void*) com_miui_server_SecurityManagerService_nativeIsReleased },
};

int register_com_miui_server_SecurityManagerService(JNIEnv* env) {
    int res = jniRegisterNativeMethods(env, "com/miui/server/SecurityManagerService", gSecurityManagerServiceMethods, NELEM(gSecurityManagerServiceMethods));
    LOG_FATAL_IF(res < 0, "Unable to register native methods.");
    (void)res;
    return 0;
}

} /* namespace android */

using namespace android;

extern "C"  jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_com_miui_server_SecurityManagerService(env);

    register_MiuiActivityHelper(env);
    return JNI_VERSION_1_4;
}
