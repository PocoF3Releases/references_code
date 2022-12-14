#include "jni.h"
#include "core_jni_helpers.h"
#include <utils/Log.h>

namespace android {
int register_com_android_server_am_SystemPressureController(JNIEnv* env);
}
using namespace android;
extern jboolean register_com_miui_server_greeze_GreezeManagerService(JNIEnv* env);

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_com_miui_server_greeze_GreezeManagerService(env);
    register_com_android_server_am_SystemPressureController(env);

    return JNI_VERSION_1_4;
}