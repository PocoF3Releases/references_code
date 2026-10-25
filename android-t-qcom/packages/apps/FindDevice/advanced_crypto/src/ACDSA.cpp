#include <ACDSA.h>
#include <ACAsymKey.h>
#include <ACUtil.h>

#include "../common/log.h"

#undef MY_LOG_TAG
#define MY_LOG_TAG "ACDSA"

ACDSA::Key::Key() : mKey(NULL) {}
ACDSA::Key::~Key() {
    if (mKey != NULL) {
        DSA_free(mKey);
        mKey = NULL;
    }
}

ACDSA::Key::operator const DSA * () const {
    return mKey;
}

ACDSA::Key::Key(const ACDSA::Key & other) : mKey(NULL) {
    copy(other);
}

ACDSA::Key & ACDSA::Key::operator=(const ACDSA::Key & other) {
    copy(other);
    return *this;
}

void ACDSA::Key::copy(const ACDSA::Key & other) {
    if (mKey != NULL) {
        DSA_free(mKey);
        mKey = NULL;
    }

    if (other.mKey) {
        EVP_PKEY * pkey = EVP_PKEY_new();
        EVP_PKEY_set1_DSA(pkey, other.mKey);
        mKey = EVP_PKEY_get1_DSA(pkey);
        EVP_PKEY_free(pkey);
    }
}

bool ACDSA::Key::initToPublicKey(const unsigned char * X509DER, size_t len) {
    if (mKey) {
        LOGE("initToPublicKey: already initialized. ");
        return false;
    }

    ACAsymKey key;
    if (!key.initToPublicKey(X509DER, len)) {
        LOGE("initToPublicKey: failed to init ACAsymKey. ");
        return false;
    }

    // assume not change the key.
    mKey = EVP_PKEY_get1_DSA((EVP_PKEY *)(const EVP_PKEY *)key);

    ACUtil::logThreadErrorsHere();

    return mKey != NULL;
}

bool ACDSA::Key::initToPrivateKey(const unsigned char * PKCS8DER, size_t len) {
    if (mKey) {
        LOGE("initToPrivateKey: already initialized. ");
        return false;
    }

    ACAsymKey key;
    if (!key.initToPrivateKey(PKCS8DER, len)) {
        LOGE("initToPrivateKey: failed to init ACAsymKey. ");
        return false;
    }

    // assume not change the key.
    mKey = EVP_PKEY_get1_DSA((EVP_PKEY *)(const EVP_PKEY *)key);

    ACUtil::logThreadErrorsHere();

    return mKey != NULL;
}

bool ACDSA::verify(const Key & key, const unsigned char * data, size_t dataLen,
           const unsigned char * sign, size_t signLen, bool * pRst) {

    *pRst = false;

    // assume not change the key.
    int rst = DSA_verify(0, data, dataLen, sign, signLen,
        (DSA *)(const DSA *)key);

    ACUtil::logThreadErrorsHere();

    if (rst == 1) { *pRst = true; }

    return rst >= 0;
}
