/*
 * Copyright (C) 2015-2020 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * Contains function declarations for ECC192 alg
 *
 * Authors:
 * Shaofeng Li
 *
 * History:
 * Shaofeng Li  XX first version for ECC192 alg in IAS project
 * chen weigang  10/09/2015 v1.0 change code style
 *
 * Related documents:
 * -GB/T 15629.11-2003/XG1-2006
 * -ISO/IEC 8802.11:1999,MOD
 *
*/

#ifndef _ALG_ECC_IF_H_
#define _ALG_ECC_IF_H_

#ifdef __cplusplus
extern "C" {
#endif

int ecc_init();
void ecc_exit();

int ecc_sign(void* msg, int len_msg,
        unsigned char* privk_24,
        unsigned char* buf_sign);
int ecc_vrfy(void* msg, int len_msg,
        unsigned char* pubk_48,
        unsigned char* buf_sign);
int ecc_new_key(unsigned char* privk, unsigned char* pubk);
int ecc_ecdh_compute(
        unsigned char* privk_24, unsigned char* pubk_48,
        unsigned char* seed_24);

int iwn_hmac_sha256(unsigned char* text, int text_len,
				unsigned char* key, unsigned key_len,
				unsigned char* digest, unsigned digest_length);
int iwn_kdhmac_sha256(unsigned char* text, unsigned  text_len,
				unsigned char* k, unsigned kl, unsigned char* o, unsigned ol);
int iwn_mhash_sha256(unsigned char *pucData, unsigned int uiDataLength,
				unsigned char *pucHash);

#ifdef __cplusplus
}
#endif

#endif
