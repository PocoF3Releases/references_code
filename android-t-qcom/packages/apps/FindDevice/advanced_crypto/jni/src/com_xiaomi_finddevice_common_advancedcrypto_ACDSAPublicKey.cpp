#include "../jni_def/com_xiaomi_finddevice_common_advancedcrypto_ACDSAPublicKey.h"

#include "../../common/native/include/util_common.h"
#include "../../common/native/include/JNIEnvWrapper.h"

#include <ACDSA.h>

#include <vector>

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACDSAPublicKey_isValidX509Bytes
  (JNIEnv * pRawEnv, jclass, jbyteArray j_X509Bytes) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_X509Bytes) {
        return JNI_FALSE;
    }

    std::vector<jbyte> X509Bytes;

    size_t X509BytesLen = env.GetArrayLength(j_X509Bytes);
    X509Bytes.resize(X509BytesLen);

    env.GetByteArrayRegion(j_X509Bytes, 0, X509BytesLen, addr(X509Bytes));

    ACDSA::Key key;
    if (key.initToPublicKey((const unsigned char *)addr(X509Bytes), X509BytesLen)) {
        return JNI_TRUE;
    }

    return JNI_FALSE;
}