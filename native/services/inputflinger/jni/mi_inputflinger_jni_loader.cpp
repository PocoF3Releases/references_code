#define LOG_TAG "MiInputflinger_JNI"

#include <nativehelper/JNIHelp.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>

using namespace android;

extern int register_com_android_server_input_MiInputManager(JNIEnv *env);

jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
{
    JNIEnv* env = NULL;
    jint result = -1;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed");
        goto bail;
    }
    if (register_com_android_server_input_MiInputManager(env) != JNI_OK) {
        ALOGE("ERROR: MiInputflinger JNI native registration failed");
        goto bail;
    }
    ALOGD("JNI_OnLoad success");
    result = JNI_VERSION_1_6;
bail:
    return result;
}