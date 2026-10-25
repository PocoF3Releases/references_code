#include "../jni_def/com_xiaomi_finddevice_common_advancedcrypto_ACAsymKeyFactory.h"

#include "../common/log.h"
#include "../../common/native/include/util_common.h"
#include "../../common/native/include/JNIEnvWrapper.h"

#include <ACAsymKey.h>

#include <vector>

#undef MY_LOG_TAG
#define MY_LOG_TAG "ACAsymKeyFactoryJNI"

#define KEY_TYPE_UNKNOWN "ACKeyUnknown"
#define KEY_TYPE_AC_ECC_PRIVATE "ACECCPrivate"
#define KEY_TYPE_AC_DSA_PUBLIC "ACDSAPublic"
// and more ...

JNIEXPORT jstring JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACAsymKeyFactory_getPublicKeyType
  (JNIEnv * pRawEnv, jclass, jbyteArray j_X509Bytes) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_X509Bytes) {
        LOGE("getPublicKeyType: !j_X509Bytes. ");
        return NULL;
    }

    const char * type = KEY_TYPE_UNKNOWN;
    do {
        std::vector<jbyte> X509Bytes;

        size_t X509BytesLen = env.GetArrayLength(j_X509Bytes);
        X509Bytes.resize(X509BytesLen);

        env.GetByteArrayRegion(j_X509Bytes, 0, X509BytesLen, addr(X509Bytes));

        ACAsymKey key;
        if (!key.initToPublicKey((const unsigned char *)addr(X509Bytes), X509Bytes.size())) {
            LOGE("getPublicKeyType: failed to init ACAsymKey. ");
            break;
        }

        if (key.type() == ACASYMKEY_TYPE_DSA) {
            type = KEY_TYPE_AC_DSA_PUBLIC;
        }

    } while (false);

    return env.NewStringUTF(type);
}


JNIEXPORT jstring JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACAsymKeyFactory_getPrivateKeyType
  (JNIEnv * pRawEnv, jclass, jbyteArray j_PKCS8Bytes) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_PKCS8Bytes) {
        LOGE("getPrivateKeyType: !j_PKCS8Bytes. ");
        return NULL;
    }

    const char * type = KEY_TYPE_UNKNOWN;
    do {
      std::vector<jbyte> PKCS8Bytes;

      size_t PKCS8BytesLen = env.GetArrayLength(j_PKCS8Bytes);
      PKCS8Bytes.resize(PKCS8BytesLen);

      env.GetByteArrayRegion(j_PKCS8Bytes, 0, PKCS8BytesLen, addr(PKCS8Bytes));

      ACAsymKey key;
      if (!key.initToPrivateKey((const unsigned char *)addr(PKCS8Bytes), PKCS8Bytes.size())) {
          LOGE("getPrivateKeyType: failed to init ACAsymKey. ");
          break;
      }

      if (key.type() == ACASYMKEY_TYPE_ECC) {
          type = KEY_TYPE_AC_ECC_PRIVATE;
      }

    } while (false);

    return env.NewStringUTF(type);
}