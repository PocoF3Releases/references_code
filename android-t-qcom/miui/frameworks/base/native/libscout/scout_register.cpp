#include "jni.h"
#include "core_jni_helpers.h"
#include <utils/Log.h>

using namespace android;
extern jboolean register_com_miui_server_stability_ScoutDisplayMemoryManager(JNIEnv* env);
extern jboolean register_com_miui_server_stability_ScoutLibraryTestManager(JNIEnv* env);

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return JNI_ERR;
    }

    if (!register_com_miui_server_stability_ScoutDisplayMemoryManager(env)) {
        ALOGE("register_com_miui_server_stability_ScoutDisplayMemoryManager failed!");
        return JNI_ERR;
    }

    if (!register_com_miui_server_stability_ScoutLibraryTestManager(env)) {
        ALOGE("register_com_miui_server_stability_ScoutLibraryTestManager failed!");
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
