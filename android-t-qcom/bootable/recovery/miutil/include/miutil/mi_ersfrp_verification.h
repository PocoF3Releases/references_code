#ifndef _MI_ERSFRP_VERIFICATION_H_
#define _MI_ERSFRP_VERIFICATION_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RSANUMBYTES     256         /* 2048 bit key length */
#define MIN_MSGBYTES    38          /* Defined by MI, length shoud more than 28*(4/3) */
#define RSANUMWORDS (RSANUMBYTES / sizeof(uint32_t))

typedef struct RSAPublicKey {
    int len;                  /* Length of n[] in number of uint32_t */
    uint32_t n0inv;           /* -1 / n[0] mod 2^32 */
    uint32_t n[RSANUMWORDS];  /* modulus as little endian array */
    uint32_t rr[RSANUMWORDS]; /* R^2 as little endian array */
    int exponent;             /* 3 or 65537 */
} RSAPublicKey;

enum {
    CRYPT_OK = 0,                /* Result OK */
    CRYPT_VERIFY_ERROR,          /* Verify Error */
    CRYPT_WRONG_INPUT,           /* Wrong Input */
    CRYPT_WRONG_MSG,             /* Wrong message to be compare */
    CRYPT_WRONG_FORMAT,          /* Wrong signature format */
    CRYPT_BUFFER_OVERFLOW        /* Buffer overflow error in base64 encode */
};

int rsa_decrypt(const unsigned char *sig,   unsigned long siglen,
        unsigned char *msg,  unsigned long msglen);

#ifdef __cplusplus
}
#endif

#endif // CONSTRAINEDCRYPTO_RSA_H_
