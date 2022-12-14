#include "../jni_def/com_xiaomi_finddevice_common_advancedcrypto_ACECC.h"

#include "../common/log.h"
#include "../../common/native/include/util_common.h"
#include "../../common/native/include/JNIEnvWrapper.h"

#include <ACECC.h>

#include <vector>

#undef MY_LOG_TAG
#define MY_LOG_TAG "ACECCJNI"

JNIEXPORT jbyteArray JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACECC_ecdsaSignNative
  (JNIEnv * pRawEnv, jclass, jbyteArray j_data, jbyteArray j_privateKeyPKCS8Bytes) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_data || !j_privateKeyPKCS8Bytes) {
        LOGE("ecdsaSignNative: !j_data || !j_privateKeyPKCS8Bytes. ");
        return NULL;
    }

    std::vector<jbyte> data;
    size_t dataLen = env.GetArrayLength(j_data);
    data.resize(dataLen);
    env.GetByteArrayRegion(j_data, 0, dataLen, addr(data));

    std::vector<jbyte> privateKeyPKCS8Bytes;
    size_t privateKeyPKCS8BytesLen = env.GetArrayLength(j_privateKeyPKCS8Bytes);
    privateKeyPKCS8Bytes.resize(privateKeyPKCS8BytesLen);
    env.GetByteArrayRegion(j_privateKeyPKCS8Bytes, 0, privateKeyPKCS8BytesLen, addr(privateKeyPKCS8Bytes));

    ACECC::Key key;
    if (!key.initToPrivateKey((const unsigned char *)addr(privateKeyPKCS8Bytes), privateKeyPKCS8BytesLen)) {
        LOGE("ecdsaSignNative: failed to init ACECC::Key. ");
        return NULL;
    }

    std::vector<unsigned char> signature;
    if (!ACECC::sign(key, (const unsigned char *)addr(data), dataLen, &signature)) {
        LOGE("ecdsaSignNative: failed to sign. ");
        return NULL;
    }

    jsize j_signatureSize = signature.size();
    jbyteArray j_signature = env.NewByteArray(j_signatureSize);
    EXCEPTION_RETURN(NULL);
    env.SetByteArrayRegion(j_signature, 0, j_signatureSize, (jbyte *)addr(signature));
    return j_signature;
}