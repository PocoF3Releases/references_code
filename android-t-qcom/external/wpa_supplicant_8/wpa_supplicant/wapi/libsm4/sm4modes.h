/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        Symmetric Cryptography Algorithm           
* Description:   mode application  of SM4 cryptography algorithm.
* Writer:        zhanggq@TEVG <zhanggq@iwncomm.com>
* Version:  $Id:sm4modes.h,v 1.0 2012/07/05 00:24:24 zhanggq Exp $     
*
* History: 
* zhanggq    2012/07/05    v1.0    Created.
*/

#ifndef _SM4MODES_H
#define _SM4MODES_H

#if defined(SM4_MODES)

SM4_RETURN sm4_ecb_encrypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, const sm4_encrypt_ctx cx[1]);

SM4_RETURN sm4_ecb_decrypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, const sm4_decrypt_ctx cx[1]);

SM4_RETURN sm4_cbc_encrypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, unsigned char *iv, const sm4_encrypt_ctx cx[1]);

SM4_RETURN sm4_cbc_decrypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, unsigned char *iv, const sm4_decrypt_ctx cx[1]);

SM4_RETURN sm4_cfb_encrypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, unsigned char *iv, const sm4_encrypt_ctx cx[1]);

SM4_RETURN sm4_cfb_decrypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, unsigned char *iv, const sm4_encrypt_ctx cx[1]);

#define sm4_ofb_encrypt sm4_ofb_crypt
#define sm4_ofb_decrypt sm4_ofb_crypt

SM4_RETURN sm4_ofb_crypt(const unsigned char *ibuf, unsigned char *obuf,
                    int len, unsigned char *iv, const sm4_encrypt_ctx cx[1]);

void sm4_ctr128_encrypt(const unsigned char *in, unsigned char *out,
						 const unsigned long length, const sm4_encrypt_ctx cx[1],
						 unsigned char ivec[SM4_BLOCK_SIZE],
						 unsigned char ecount_buf[SM4_BLOCK_SIZE],
						 unsigned int *num);

SM4_RETURN sm4_cbc_mac(const unsigned char *ibuf, int len, unsigned char *omac, 
					   unsigned char *iv, const sm4_encrypt_ctx cx[1]);

SM4_RETURN sm4_wapi_cbc_mac(const unsigned char *ibuf, int len, unsigned char *omac, 
					        unsigned char *iv, const sm4_encrypt_ctx cx[1]);

#endif

#endif /* _SM4MODES_H */
