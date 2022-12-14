#include "../jni_def/com_xiaomi_finddevice_common_advancedcrypto_ACECCPrivateKey.h"

#include "../../common/native/include/util_common.h"
#include "../../common/native/include/JNIEnvWrapper.h"

#include <ACECC.h>

#include <vector>

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACECCPrivateKey_isValidPKCS8Bytes
  (JNIEnv * pRawEnv, jclass , jbyteArray j_PKCS8Bytes) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_PKCS8Bytes) {
        return JNI_FALSE;
    }

    std::vector<jbyte> PKCS8Bytes;

    size_t PKCS8BytesLen = env.GetArrayLength(j_PKCS8Bytes);
    PKCS8Bytes.resize(PKCS8BytesLen);

    env.GetByteArrayRegion(j_PKCS8Bytes, 0, PKCS8BytesLen, addr(PKCS8Bytes));

    ACECC::Key key;
    if (key.initToPrivateKey((const unsigned char *)addr(PKCS8Bytes), PKCS8BytesLen)) {
        return JNI_TRUE;
    }

    return JNI_FALSE;
}
