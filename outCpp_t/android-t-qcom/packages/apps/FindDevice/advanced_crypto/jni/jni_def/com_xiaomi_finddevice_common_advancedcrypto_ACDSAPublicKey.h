#include <jni.h>

#ifndef _Included_com_xiaomi_finddevice_common_advancedcrypto_ACDSAPublicKey
#define _Included_com_xiaomi_finddevice_common_advancedcrypto_ACDSAPublicKey
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACDSAPublicKey_isValidX509Bytes
  (JNIEnv *, jclass, jbyteArray);

#ifdef __cplusplus
}
#endif
#endif
