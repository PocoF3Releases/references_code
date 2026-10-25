#include <jni.h>

#ifndef _Included_com_xiaomi_finddevice_common_advancedcrypto_ACDSA
#define _Included_com_xiaomi_finddevice_common_advancedcrypto_ACDSA
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACDSA_dsaVerifyNative
  (JNIEnv *, jclass, jbyteArray, jbyteArray, jbyteArray, jbooleanArray);

#ifdef __cplusplus
}
#endif
#endif
