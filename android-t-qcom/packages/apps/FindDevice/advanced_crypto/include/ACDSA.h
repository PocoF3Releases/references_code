#ifndef ANDROID_ACDSA_H
#define ANDROID_ACDSA_H

#include <openssl/dsa.h>
#include <vector>

class ACDSA {

public:
    class Key {

    public:
        Key();
        ~Key();

        bool initToPublicKey(const unsigned char * X509DER, size_t len);
        bool initToPrivateKey(const unsigned char * PKCS8DER, size_t len);

        Key(const Key &);
        Key & operator=(const Key &);

        operator const DSA * () const;

    private:
        void copy(const Key &);

        DSA * mKey;
    };

    static bool verify(const Key & key, const unsigned char * data, size_t dataLen,
       const unsigned char * sign, size_t signLen, bool * pRst);
};

#endif //ANDROID_ACDSA_H
