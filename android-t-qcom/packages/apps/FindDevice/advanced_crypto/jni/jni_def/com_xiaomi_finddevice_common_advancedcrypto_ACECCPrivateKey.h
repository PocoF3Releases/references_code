#include <jni.h>

#ifndef _Included_com_xiaomi_finddevice_common_advancedcrypto_ACECCPrivateKey
#define _Included_com_xiaomi_finddevice_common_advancedcrypto_ACECCPrivateKey
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACECCPrivateKey_isValidPKCS8Bytes
  (JNIEnv *, jclass, jbyteArray);

#ifdef __cplusplus
}
#endif
#endif
