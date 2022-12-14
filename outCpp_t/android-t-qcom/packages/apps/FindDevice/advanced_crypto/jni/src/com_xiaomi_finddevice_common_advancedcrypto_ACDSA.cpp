#include "../jni_def/com_xiaomi_finddevice_common_advancedcrypto_ACDSA.h"

#include "../common/log.h"
#include "../../common/native/include/util_common.h"
#include "../../common/native/include/JNIEnvWrapper.h"

#include <ACDSA.h>

#include <vector>

#undef MY_LOG_TAG
#define MY_LOG_TAG "ACDSAJNI"

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACDSA_dsaVerifyNative
  (JNIEnv * pRawEnv, jclass, jbyteArray j_data, jbyteArray j_signature, jbyteArray j_publicKeyX509Bytes, jbooleanArray j_rst) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_data || !j_signature || !j_publicKeyX509Bytes) {
        LOGE("dsaVerifyNative: !j_data || !j_signature || !j_publicKeyX509Bytes. ");
        return JNI_FALSE;
    }

    if (!j_rst || env.GetArrayLength(j_rst) < 1) {
        LOGE("dsaVerifyNative: !j_rst || env.GetArrayLength(j_rst) < 1");
        return JNI_FALSE;
    }

    jboolean rstBuf = JNI_FALSE;
    env.SetBooleanArrayRegion(j_rst, 0, 1, &rstBuf);

    std::vector<jbyte> data;
    size_t dataLen = env.GetArrayLength(j_data);
    data.resize(dataLen);
    env.GetByteArrayRegion(j_data, 0, dataLen, addr(data));

    std::vector<jbyte> signature;
    size_t signatureLen = env.GetArrayLength(j_signature);
    signature.resize(signatureLen);
    env.GetByteArrayRegion(j_signature, 0, signatureLen, addr(signature));

    std::vector<jbyte> publicKeyX509Bytes;
    size_t publicKeyX509BytesLen = env.GetArrayLength(j_publicKeyX509Bytes);
    publicKeyX509Bytes.resize(publicKeyX509BytesLen);
    env.GetByteArrayRegion(j_publicKeyX509Bytes, 0, publicKeyX509BytesLen, addr(publicKeyX509Bytes));

    ACDSA::Key key;
    if (!key.initToPublicKey((const unsigned char *)addr(publicKeyX509Bytes), publicKeyX509BytesLen)) {
        LOGE("ecdsaSignNative: failed to init ACDSA::Key. ");
        return JNI_FALSE;
    }

    bool rst = false;
    bool success =
        ACDSA::verify(key,
          (const unsigned char *)addr(data), dataLen,
          (const unsigned char *)addr(signature), signatureLen,
          &rst);

    rstBuf = rst ? JNI_TRUE : JNI_FALSE;
    env.SetBooleanArrayRegion(j_rst, 0, 1, &rstBuf);

    if (!success) {
        LOGE("ecdsaSignNative: failed to verify. ");
    }

    return success ? JNI_TRUE : JNI_FALSE;
}