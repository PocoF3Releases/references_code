#include "jni.h"
#include "core_jni_helpers.h"
#include <utils/Log.h>

extern jboolean register_android_content_res_AssetManagerImpl(JNIEnv* env);
// extern jboolean register_com_miui_server_greeze_GreezeManagerService(JNIEnv* env);
extern jboolean register_android_os_MessageMonitor(JNIEnv* env);
extern jboolean register_android_os_BinderMonitor(JNIEnv* env);
extern jboolean register_android_os_PerfDebugFileUtils(JNIEnv* env);
extern int register_android_os_NativeMiuiMemReclaimer(JNIEnv* env);
extern int register_android_os_NativeTurboSchedManager(JNIEnv* env);

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_android_os_MessageMonitor(env);
    register_android_os_BinderMonitor(env);
    register_android_os_PerfDebugFileUtils(env);
    // register_com_miui_server_greeze_GreezeManagerService(env);
    register_android_content_res_AssetManagerImpl(env);
    register_android_os_NativeMiuiMemReclaimer(env);
    register_android_os_NativeTurboSchedManager(env);

    return JNI_VERSION_1_4;
}
