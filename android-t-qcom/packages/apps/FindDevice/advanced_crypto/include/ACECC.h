#ifndef ANDROID_ACECC_H
#define ANDROID_ACECC_H

#include <openssl/ec.h>
#include <vector>

class ACECC {

public:
    class Key {

    public:
        Key();
        ~Key();

        bool initToPublicKey(const unsigned char * X509DER, size_t len);
        bool initToPrivateKey(const unsigned char * PKCS8DER, size_t len);

        Key(const Key &);
        Key & operator=(const Key &);

        operator const EC_KEY * () const;

    private:
        void copy(const Key &);

        EC_KEY * mKey;
    };

    static bool sign(const Key & key, const unsigned char * data, size_t len,
       std::vector<unsigned char> * pSign);
};

#endif
