#include <stdio.h>

#include "../common/log.h"
#include "../../common/native/include/util_common.h"

#include <ACAsymKey.h>
#include <ACHex.h>
#include <ACECC.h>
#include <ACDSA.h>

#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>

#include <algorithm>

void test() {
    std::vector<unsigned char> bin;
    std::vector<char> str;

    ACHex::bin2hex((const unsigned char *)"\xAA\xBB\xCC\xFF\x00\x99\xA3", 7, &str);
    printf("%s\n", addr(str));

    printf("%d\n", ACHex::hex2bin("CCBBAAFF99A300", &bin));
    ACHex::bin2hex(addr(bin), bin.size(), &str);
    printf("%s\n", addr(str));

    printf("%d\n", ACHex::hex2bin("CCBBAAFF99A30", &bin));
    printf("%d\n", ACHex::hex2bin("CCBBAAFF99A30Q", &bin));

    std::vector<unsigned char> eccPriKeyPKCS8;
    ACHex::hex2bin("304d020100301306072a8648ce3d020106082a8648ce3d0301070433303102010104202d48954b58d01e5bf428bf4e97c740972be7694548fb37a357bd79fb87456972a00a06082a8648ce3d030107",
        &eccPriKeyPKCS8);
    ACAsymKey eccPriKey;
    printf("%d\n", eccPriKey.initToPrivateKey(addr(eccPriKeyPKCS8), eccPriKeyPKCS8.size()));
    printf("%d\n", eccPriKey.type());

    std::vector<unsigned char> dsaPubKeyX509;
    ACHex::hex2bin("308201b73082012c06072a8648ce3804013082011f02818100fd7f53811d75122952df4a9c2eece4e7f611b7523cef4400c31e3f80b6512669455d402251fb593d8d58fabfc5f5ba30f6cb9b556cd7813b801d346ff26660b76b9950a5a49f9fe8047b1022c24fbba9d7feb7c61bf83b57e7c6a8a6150f04fb83f6d3c51ec3023554135a169132f675f3ae2b61d72aeff22203199dd14801c70215009760508f15230bccb292b982a2eb840bf0581cf502818100f7e1a085d69b3ddecbbcab5c36b857b97994afbbfa3aea82f9574c0b3d0782675159578ebad4594fe67107108180b449167123e84c281613b7cf09328cc8a6e13c167a8b547c8d28e0a3ae1e2bb3a675916ea37f0bfa213562f1fb627a01243bcca4f1bea8519089a883dfe15ae59f06928b665e807b552564014c3bfecf492a0381840002818069de44a8ff316f9d8047489601631f7b6733b5b7aa496dee3d6e6f0ef026c957bf077c92d873b69cf68d7ff2c3da53cb53bc4a9802ffcc598f7c8742ffd37c62a113ef38030afa9609671047c7e8d6cabc149f8f2f6a5a07b5b231777b475370e263b9651a64bd301996d338ce7bebc5bdf13741122b27d1dd14491db84e6f95",
            &dsaPubKeyX509);
    ACAsymKey dsaPubKey;
    printf("%d\n", dsaPubKey.initToPublicKey(addr(dsaPubKeyX509), dsaPubKeyX509.size()));
    printf("%d\n", dsaPubKey.type());

    ACAsymKey badKey;
    printf("%d\n", badKey.initToPrivateKey((const unsigned char *)"\0x11\0x22", 2));
    printf("%d\n", badKey.type());

    ACECC::Key eccPriKeyTyped;
    printf("%d\n", eccPriKeyTyped.initToPrivateKey(addr(eccPriKeyPKCS8), eccPriKeyPKCS8.size()));
    printf("%p\n", (const EC_KEY *)eccPriKeyTyped);

    ACECC::Key eccPriKeyTyped2;
    eccPriKeyTyped2 = eccPriKeyTyped;
    printf("%p\n", (const EC_KEY *)eccPriKeyTyped2);

    ACECC::Key eccPriKeyTypedBad;
    printf("%d\n", eccPriKeyTypedBad.initToPrivateKey((const unsigned char *)"\0x11\0x22", 2));
    printf("%p\n", (const EC_KEY *)eccPriKeyTypedBad);

    printf("%d\n", ACECC::sign(eccPriKeyTyped, (const unsigned char *)"\0x11\0x22\0x33", 3, &bin));
    ACHex::bin2hex(addr(bin), bin.size(), &str);
    printf("%s\n", addr(str));

    ACDSA::Key dsaPubKeyTyped;
    printf("%d\n", dsaPubKeyTyped.initToPublicKey(addr(dsaPubKeyX509), dsaPubKeyX509.size()));
    printf("%p\n", (const DSA *)dsaPubKeyTyped);

    ACDSA::Key dsaPubKeyTyped2;
    dsaPubKeyTyped2 = dsaPubKeyTyped;
    printf("%p\n", (const DSA *)dsaPubKeyTyped2);

    ACDSA::Key dsaPubKeyTypedBad;
    printf("%d\n", dsaPubKeyTypedBad.initToPublicKey((const unsigned char *)"\0x11\0x22", 2));
    printf("%p\n", (const DSA *)dsaPubKeyTypedBad);

    bool verifyRst;
    printf("%d\n", ACDSA::verify(dsaPubKeyTyped, (const unsigned char *)"\0x11\0x22\0x33", 3,
        (const unsigned char *)"\0x11\0x22", 2, &verifyRst));
    printf("%d\n", verifyRst);
}

int main() {


    test();
    printf("DONE!\n");




//    const char * bin, * str;
//    size_t len;
//
//    str = ACHex::bin2hex("\xAA\xBB\xCC\xFF\x00\x99\xA3", 7);
//    printf("%s\n", str);
//    ACHex::freeString(str);
//
//    bin = ACHex::hex2bin("CCBBAAFF99A300", &len);
//    str = ACHex::bin2hex(bin, len);
//    printf("%s\n", str);
//    ACHex::freeBin(bin);
//    ACHex::freeString(str);
//
//    printf("%d\n", ACHex::hex2bin("CCBBAAFF99A30", &len));
//    printf("%d\n", ACHex::hex2bin("CCBBAAFF99A30Q", &len));

//    size_t keyBinSize = 0;
//    const char * keyBin =
//        ACHex::hex2bin("304d020100301306072a8648ce3d020106082a8648ce3d0301070433303102010104202d48954b58d01e5bf428bf4e97c740972be7694548fb37a357bd79fb87456972a00a06082a8648ce3d030107",
//        &keyBinSize);
//
//    BIO * mem = BIO_new_mem_buf((void *)keyBin, keyBinSize);
//    PKCS8_PRIV_KEY_INFO * p8info = d2i_PKCS8_PRIV_KEY_INFO_bio(mem, NULL);
//    EVP_PKEY * pkey = EVP_PKCS82PKEY(p8info);
//    EC_KEY * pECK = EVP_PKEY_get1_EC_KEY(pkey);
//
//    const char * digest = "a00291e8229d191815f2cbd2aa49d3b783585deae8012ca5941f011ceb9eb119";
//    size_t digestLen = 0;
//    const char * digestBin = ACHex::hex2bin(digest, &digestLen);
//    std::reverse((char *)digestBin, (char *)digestBin + digestLen);
//
//    printf("%d\n", digestLen);
//
//    ECDSA_SIG * sign = ECDSA_do_sign((const unsigned char *)digestBin, digestLen, pECK);
//
//    char buf [256] = {};
//    const char * s = NULL;
//    BN_bn2bin(sign->r, (unsigned char *)buf);
//    s = ACHex::bin2hex(buf, BN_num_bytes(sign->r));
//    printf("%s\n", s);
//    ACHex::freeString(s);
//    BN_bn2bin(sign->s, (unsigned char *)buf);
//    s = ACHex::bin2hex(buf, BN_num_bytes(sign->s));
//    printf("%s\n", s);
//    ACHex::freeString(s);
//
//    ECDSA_SIG_free(sign);
//    ACHex::freeBin(digestBin);
//    EC_KEY_free(pECK);
//    EVP_PKEY_free(pkey);
//    PKCS8_PRIV_KEY_INFO_free(p8info);
//    BIO_free(mem);
//    ACHex::freeBin(keyBin);
//
//
//    keyBinSize = 0;
//    keyBin =
//            ACHex::hex2bin("308201b73082012c06072a8648ce3804013082011f02818100fd7f53811d75122952df4a9c2eece4e7f611b7523cef4400c31e3f80b6512669455d402251fb593d8d58fabfc5f5ba30f6cb9b556cd7813b801d346ff26660b76b9950a5a49f9fe8047b1022c24fbba9d7feb7c61bf83b57e7c6a8a6150f04fb83f6d3c51ec3023554135a169132f675f3ae2b61d72aeff22203199dd14801c70215009760508f15230bccb292b982a2eb840bf0581cf502818100f7e1a085d69b3ddecbbcab5c36b857b97994afbbfa3aea82f9574c0b3d0782675159578ebad4594fe67107108180b449167123e84c281613b7cf09328cc8a6e13c167a8b547c8d28e0a3ae1e2bb3a675916ea37f0bfa213562f1fb627a01243bcca4f1bea8519089a883dfe15ae59f06928b665e807b552564014c3bfecf492a0381840002818069de44a8ff316f9d8047489601631f7b6733b5b7aa496dee3d6e6f0ef026c957bf077c92d873b69cf68d7ff2c3da53cb53bc4a9802ffcc598f7c8742ffd37c62a113ef38030afa9609671047c7e8d6cabc149f8f2f6a5a07b5b231777b475370e263b9651a64bd301996d338ce7bebc5bdf13741122b27d1dd14491db84e6f95",
//            &keyBinSize);
//
//    mem = BIO_new_mem_buf((void *)keyBin, keyBinSize);
//    pkey = d2i_PUBKEY_bio(mem, NULL);
//    DSA * pDSA = EVP_PKEY_get1_DSA(pkey);
//
//    printf("%s\n", ERR_error_string(ERR_get_error(), NULL));
//
//
//    DSA_free(pDSA);
//    EVP_PKEY_free(pkey);
//    BIO_free(mem);
//    ACHex::freeBin(keyBin);


    //OpenSSL_add_all_algorithms();



//    EVP_PKEY_free(key);

}

