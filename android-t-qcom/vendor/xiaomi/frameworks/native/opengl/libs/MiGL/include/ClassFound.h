#include <nativehelper/JNIHelp.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/String8.h>
#include <cutils/log.h>

namespace android {
namespace migl {
static jclass gActivityThread_class;
static jmethodID gActivityThread_currentPackageName;
static String8 currentPackageName;
int initActivityThreadClass() {
    int ret = 1;
    jobject temp;
    jclass clazz;
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (env ==  nullptr) {
        ret = 0;
        goto func_ret;
    }
    clazz = env->FindClass("android/app/ActivityThread");
    if (clazz == NULL) {
        ALOGE("find ActivityThread class failed");
        ret = 0;
        goto func_ret;
    }
    temp = env->NewGlobalRef(clazz);
    if (temp == NULL) {
        ALOGE("new ActivityThread ref failed");
        ret = 0;
        goto func_ret;
    }
    gActivityThread_class = static_cast<jclass>(temp);
    gActivityThread_currentPackageName = env->GetStaticMethodID(gActivityThread_class, "currentPackageName",
                                                                "()Ljava/lang/String;");
    if (gActivityThread_currentPackageName == NULL) {
        ALOGE("find currentPackageName method failed");
        ret = 0;
        goto func_ret;
    }
func_ret:
    return ret;
}
String8 getCurrentPackageName() {
    if (currentPackageName.isEmpty()) {
        JNIEnv* env = AndroidRuntime::getJNIEnv();
        jstring jstr = (jstring) env->CallStaticObjectMethod(gActivityThread_class, gActivityThread_currentPackageName);
        if (jstr != nullptr) {
            const char* cstr = env->GetStringUTFChars(jstr, nullptr);
            if (cstr != nullptr) {
                currentPackageName.setTo(cstr);
            }
        }
    }
    return currentPackageName;
}
}
}

