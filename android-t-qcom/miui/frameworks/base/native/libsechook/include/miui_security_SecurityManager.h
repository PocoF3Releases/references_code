#include <jni.h>

#ifndef _Included_com_android_internal_os_ZygoteMiuiInjector
#define _Included_com_android_internal_os_ZygoteMiuiInjector
#ifdef __cplusplus
extern "C" {
#endif

int register_native_methods(JNIEnv* env);

jint JNI_OnLoad(JavaVM* vm, void* reserved);

#ifdef __cplusplus
}
#endif
#endif
