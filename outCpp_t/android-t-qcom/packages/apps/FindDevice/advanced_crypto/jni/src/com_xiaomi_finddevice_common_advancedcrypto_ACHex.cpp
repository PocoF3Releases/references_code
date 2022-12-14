#include "../jni_def/com_xiaomi_finddevice_common_advancedcrypto_ACHex.h"

#include "../../common/native/include/util_common.h"
#include "../../common/native/include/JNIEnvWrapper.h"

#include <ACHex.h>

#include <vector>

JNIEXPORT jstring JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACHex_bin2hex
  (JNIEnv * pRawEnv, jclass, jbyteArray j_bin) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_bin) { return NULL; }

    std::vector<jbyte> bin;

    size_t binLen = env.GetArrayLength(j_bin);
    bin.resize(binLen, 0);

    env.GetByteArrayRegion(j_bin, 0, binLen, addr(bin));

    std::vector<char> hex;
    ACHex::bin2hex((const unsigned char *)addr(bin), binLen * sizeof(jbyte), &hex);

    return env.NewStringUTF(addr(hex));
}


JNIEXPORT jbyteArray JNICALL Java_com_xiaomi_finddevice_common_advancedcrypto_ACHex_hex2bin
  (JNIEnv * pRawEnv, jclass, jstring j_hex) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_hex) { return NULL; }

    std::vector<char> vHex;
    env.getStringUTF(j_hex, &vHex);
    const char * hex = addr(vHex);

    std::vector<unsigned char> bin;
    bool success = ACHex::hex2bin(hex, &bin);

    if (!success) {
        return NULL;
    }

    jsize j_binSize = bin.size();
    jbyteArray j_bin = env.NewByteArray(j_binSize);
    EXCEPTION_RETURN(NULL);
    env.SetByteArrayRegion(j_bin, 0, j_binSize, (jbyte *)addr(bin));

    return j_bin;
}