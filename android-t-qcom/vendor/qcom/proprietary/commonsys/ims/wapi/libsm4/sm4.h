/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        Symmetric Cryptography Algorithm           
* Description:   application  of SM4 cryptography algorithm.
* Writer:        zhangguoqiang@TEVG <zhanggq@iwncomm.com>
* Version:  $Id:sm4.h,v 1.0 2012/07/04 00:24:24 zhangguoqiang Exp $     
*
* History: 
* zhangguoqiang    2012/07/04    v1.0    Created.
*/

#ifndef _SM4_H
#define _SM4_H

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdlib.h>

/*=====================================================================*/
#define SM4_MODES

#define SM4_ENCRYPT
#define SM4_DECRYPT

#define SM4_BLOCK_SIZE  16

#define RK_LENGTH 32
#define SM4_RETURN int

#define SM4_RETURN_SUCCESS 0
#define SM4_RETURN_FAILURE 1
/*=====================================================================*/
typedef union
{
	unsigned int l;
	unsigned char b[4];
}sm4_inf;

typedef struct
{
	unsigned int rk[RK_LENGTH];
	sm4_inf inf;
}sm4_encrypt_ctx;

typedef struct
{
	unsigned int rk[RK_LENGTH];
	sm4_inf inf;
}sm4_decrypt_ctx;
/*=====================================================================*/
/*SM4¼ÓÃÜº¯Êý*/
#if defined(SM4_ENCRYPT)

SM4_RETURN sm4_encrypt_key(const unsigned char *key, int key_len, sm4_encrypt_ctx cx[1]);
SM4_RETURN sm4_encrypt(const unsigned char *in, unsigned char *out, const sm4_encrypt_ctx cx[1]);

#endif

/*SM4½âÃÜº¯Êý*/
#if defined(SM4_ENCRYPT)

SM4_RETURN sm4_decrypt_key(const unsigned char *key, int key_len, sm4_decrypt_ctx cx[1]);
SM4_RETURN sm4_decrypt(const unsigned char *in, unsigned char *out, const sm4_decrypt_ctx cx[1]);

#endif

#if defined(SM4_MODES)
#include "sm4modes.h"
#endif
/*=====================================================================*/

#if defined(__cplusplus)
}
#endif

#endif



