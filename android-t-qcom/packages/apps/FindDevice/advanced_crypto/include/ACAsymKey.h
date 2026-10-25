#ifndef ANDROID_ACASYMKEY_H
#define ANDROID_ACASYMKEY_H

#include <openssl/evp.h>

#define ACASYMKEY_TYPE_NONE EVP_PKEY_NONE
#define ACASYMKEY_TYPE_ECC EVP_PKEY_EC
#define ACASYMKEY_TYPE_DSA EVP_PKEY_DSA

class ACAsymKey {

public:
    ACAsymKey();
    ~ACAsymKey();

    bool initToPublicKey(const unsigned char * X509DER, size_t len);
    bool initToPrivateKey(const unsigned char * PKCS8DER, size_t len);

    operator const EVP_PKEY * () const;
    int type();

private:
    // No way to duplicate an EVP_PKEY.
    ACAsymKey(const ACAsymKey &);
    ACAsymKey & operator=(const ACAsymKey &);

    EVP_PKEY * mEVPPKey;
};

#endif //ANDROID_ACASYMKEY_H
