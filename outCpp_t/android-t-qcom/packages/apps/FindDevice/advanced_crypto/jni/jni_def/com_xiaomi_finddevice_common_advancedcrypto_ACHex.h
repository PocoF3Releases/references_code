#include <jni.h>

#ifndef _Included_com_xiaomi_finddevice_common_advancedcrypto_ACHex
#define _Included_com_xiaomi_finddevice_common_advancedcrypto_ACHex
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jstring JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACHex_bin2hex
  (JNIEnv *, jclass, jbyteArray);


JNIEXPORT jbyteArray JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACHex_hex2bin
  (JNIEnv *, jclass, jstring);

#ifdef __cplusplus
}
#endif
#endif
