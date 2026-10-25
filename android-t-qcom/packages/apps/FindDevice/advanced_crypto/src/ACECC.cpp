#include <ACECC.h>
#include <ACAsymKey.h>
#include <ACUtil.h>

#include "../common/log.h"
#include "../../common/native/include/util_common.h"

#include <openssl/ecdsa.h>
#include <openssl/bn.h>

#undef MY_LOG_TAG
#define MY_LOG_TAG "ACECC"

ACECC::Key::Key() : mKey(NULL) {}
ACECC::Key::~Key() {
    if (mKey != NULL) {
        EC_KEY_free(mKey);
        mKey = NULL;
    }
}

ACECC::Key::operator const EC_KEY * () const {
    return mKey;
}

ACECC::Key::Key(const ACECC::Key & other) : mKey(NULL) {
    copy(other);
}

ACECC::Key & ACECC::Key::operator=(const ACECC::Key & other) {
    copy(other);
    return *this;
}

void ACECC::Key::copy(const ACECC::Key & other) {
    if (mKey != NULL) {
        EC_KEY_free(mKey);
    }

    mKey = other.mKey;
    if (mKey != NULL) {
        EC_KEY_up_ref(mKey);
    }
}

bool ACECC::Key::initToPublicKey(const unsigned char * X509DER, size_t len) {
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
    mKey = EVP_PKEY_get1_EC_KEY((EVP_PKEY *)(const EVP_PKEY *)key);

    ACUtil::logThreadErrorsHere();

    return mKey != NULL;
}

bool ACECC::Key::initToPrivateKey(const unsigned char * PKCS8DER, size_t len) {
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
    mKey = EVP_PKEY_get1_EC_KEY((EVP_PKEY *)(const EVP_PKEY *)key);

    ACUtil::logThreadErrorsHere();

    return mKey != NULL;
}

bool ACECC::sign(const ACECC::Key & key, const unsigned char * data, size_t len,
    std::vector<unsigned char> *pSign) {

    pSign->resize(0);

    if ((const EC_KEY *)key == NULL ) {
        LOGE("sign: NULL key. ");
        return false;
    }

    if (len == 0) {
        LOGW("sign: empty content. ");
        return true;
    }


    ECDSA_SIG * rawSign = NULL;
    bool success = false;

    do {
        rawSign = ECDSA_do_sign(data, len, (EC_KEY *)(const EC_KEY *)key);
        if (!rawSign) { break; }

        int sign_len = i2d_ECDSA_SIG(rawSign, NULL);
        if (!sign_len) { break; }

        pSign->resize(sign_len);
        unsigned char * first = addr(*pSign);

        if (i2d_ECDSA_SIG(rawSign, &first) != sign_len) {
            break;
        }

        success = true;
    } while(false);

    if (rawSign != NULL) {
        ECDSA_SIG_free(rawSign);
    }

    ACUtil::logThreadErrorsHere();

    return success;
}
