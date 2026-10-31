#include <ACAsymKey.h>
#include <ACUtil.h>

#include <openssl/x509.h>

#undef MY_LOG_TAG
#define MY_LOG_TAG "ACAsymKey"

ACAsymKey::ACAsymKey() : mEVPPKey(NULL) {}
ACAsymKey::~ACAsymKey() {
    if (mEVPPKey) {
        EVP_PKEY_free(mEVPPKey);
        mEVPPKey = NULL;
    }
}

bool ACAsymKey::initToPublicKey(const unsigned char * X509DER, size_t len) {
    if (mEVPPKey) {
        LOGE("initToPublicKey: already initialized. ");
        return false;
    }

    BIO * mem = NULL;

    do {
        mem = BIO_new_mem_buf((void *)X509DER, len);
        if (!mem) { break; }

        mEVPPKey = d2i_PUBKEY_bio(mem, NULL);
        if (!mEVPPKey) { break; }
    } while (false);

    if (mem) { BIO_free(mem); }

    ACUtil::logThreadErrorsHere();

    return mEVPPKey != NULL;
}

bool ACAsymKey::initToPrivateKey(const unsigned char * PKCS8DER, size_t len) {

    if (mEVPPKey) {
        LOGE("initToPrivateKey: already initialized. ");
        return false;
    }

    BIO * mem = NULL;
    PKCS8_PRIV_KEY_INFO * p8info = NULL;

    do {
        mem = BIO_new_mem_buf((void *)PKCS8DER, len);
        if (!mem) { break; }

        p8info = d2i_PKCS8_PRIV_KEY_INFO_bio(mem, NULL);
        if (!p8info) { break; }

        mEVPPKey = EVP_PKCS82PKEY(p8info);
        if (!mEVPPKey) { break; }

    } while (false);

    if (p8info) { PKCS8_PRIV_KEY_INFO_free(p8info); }
    if (mem) { BIO_free(mem); }

    ACUtil::logThreadErrorsHere();

    return mEVPPKey != NULL;
}

ACAsymKey::operator const EVP_PKEY * () const {
    return mEVPPKey;
}

int ACAsymKey::type() {
    if (mEVPPKey == NULL) return ACASYMKEY_TYPE_NONE;
    return EVP_PKEY_type(mEVPPKey->type);
}
