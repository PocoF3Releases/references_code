#include <jni.h>

#ifndef _Included_com_xiaomi_finddevice_common_advancedcrypto_ACECC
#define _Included_com_xiaomi_finddevice_common_advancedcrypto_ACECC
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jbyteArray JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACECC_ecdsaSignNative
  (JNIEnv *, jclass, jbyteArray, jbyteArray);

#ifdef __cplusplus
}
#endif
#endif
