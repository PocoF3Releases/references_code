#include <jni.h>

#ifndef _Included_com_xiaomi_finddevice_common_advancedcrypto_ACAsymKeyFactory
#define _Included_com_xiaomi_finddevice_common_advancedcrypto_ACAsymKeyFactory
#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT jstring JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACAsymKeyFactory_getPublicKeyType
  (JNIEnv *, jclass, jbyteArray);


JNIEXPORT jstring JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACAsymKeyFactory_getPrivateKeyType
  (JNIEnv *, jclass, jbyteArray);

#ifdef __cplusplus
}
#endif
#endif
