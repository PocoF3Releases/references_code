#include <nativehelper/JNIHelp.h>
#include <log/log.h>

extern int register_com_xiaomi_NetworkBoost_NetworkAccelerateSwitch(JNIEnv* env);

JavaVM *gJavaVM = NULL;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
{

    jint result = -1;
    JNIEnv* env = NULL;

    ALOGI("on Load native networkboostjni");
    gJavaVM = vm;
    /**
     * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/functions.html
     * SINCE JDK/JRE 1.6:
     * #define JNI_VERSION_1_6 0x00010006
     */
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        ALOGE("ERROR: networkboostjni GetEnv failed");
        goto fail;
    }

    if (register_com_xiaomi_NetworkBoost_NetworkAccelerateSwitch(env) < 0) {
        ALOGE("ERROR: networkboostjni register_com_xiaomi_NetworkBoost_NetworkAccelerateSwitch failed");
        goto fail;
    }
    result = JNI_VERSION_1_6;

fail:

    return result;
}
