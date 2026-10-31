/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        Symmetric Cryptography Algorithm           
* Description:   GCM mode application  of SM4 cryptography algorithm.
* Writer:        wanht@TEVG <wanht@iwncomm.com>
* Version:  $Id:gcm.h,v 1.0 2014/05/05 00:24:24 wanht Exp $     
*
* History: 
* wanht    2014/05/05    v1.0    Created.
*/
#ifndef GCM_H
#define GCM_H

#include "sm4.h"
#include <string.h>

typedef unsigned long long word64;
typedef unsigned int word32;

//gcm\CA\FD\BEÝ½á¹¹\B6\A8\D2\E5
typedef struct 
{
	sm4_encrypt_ctx sm4_ctx;    //sm4\C3\DCÔ¿\CA\FD\BE\DD
    word64 p[16];              //\D3\C3\D3Ú±\ED\B8\F1\B3Ë·\A8Ô¤\BC\C6\CB\E3Ê±\B4æ´¢\CA\FD\BE\DD
    word64 q[16];              //\D3\C3\D3Ú±\ED\B8\F1\B3Ë·\A8Ô¤\BC\C6\CB\E3Ê±\B4æ´¢\CA\FD\BE\DD
}gcm_ctx;

#ifdef __cplusplus
extern "C" {
#endif

//\B1\ED\B8\F1\B3Ë·\A8Ô¤\BC\C6\CB\E3\CA\FD\BEÝº\CD\C9\E8\D6\C3\C3\DCÔ¿
int gcm_init_and_key(                     
        const unsigned char key[],          //\C3\DCÔ¿\CA\FD\BE\DD
        unsigned long key_len,              //\C3\DCÔ¿\CA\FD\BEÝ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        gcm_ctx ctx[1]);                    //gcm\CA\FD\BE\DD

//\C7\E5\C0\ED\CA\FD\BE\DD
int gcm_end(                               
        gcm_ctx ctx[1]);                    


//gcmÄ£Ê½\BC\D3\C3\DC
int gcm_encrypt_message(                   
        const unsigned char iv[],           //\B3\F5Ê¼\BB\AF\CF\F2\C1\BF\CA\FD\BE\DD
        unsigned long iv_len,               //\B3\F5Ê¼\BB\AF\CF\F2\C1\BF\CA\FD\BEÝµÄ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        const unsigned char hdr[],          //\B8\BD\BCÓ¼\F8\B1\F0\CA\FD\BE\DD
        unsigned long hdr_len,              //\B8\BD\BCÓ¼\F8\B1\F0\CA\FD\BEÝµÄ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        unsigned char msg[],                //\BC\D3\C3\DCÇ°\C3\F7\CE\C4\CA\FD\BEÝ£\AC\BC\D3\C3Üº\F3\C3\DC\CE\C4\CA\FD\BE\DD
        unsigned long msg_len,              //\C3\F7\CE\C4\CA\FD\BEÝµÄ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        unsigned char tag[],                //\BC\F8\B1\F0Ð£\D1\E9\CA\FD\BE\DD
        unsigned long tag_len,              //\BC\F8\B1\F0Ð£\D1\E9\CA\FD\BEÝµÄ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        gcm_ctx ctx[1]);                    //gcm\CA\FD\BE\DD,\C6\E4\D6Ðº\AC\D3Ð¼\D3\C3\DC\C3\DCÔ¿

//gcmÄ£Ê½\BD\E2\C3\DC
int gcm_decrypt_message(                    
        const unsigned char iv[],           //\B3\F5Ê¼\BB\AF\CF\F2\C1\BF\CA\FD\BE\DD
        unsigned long iv_len,               //\B3\F5Ê¼\BB\AF\CF\F2\C1\BF\CA\FD\BEÝµÄ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        const unsigned char hdr[],          //\B8\BD\BCÓ¼\F8\B1\F0\CA\FD\BE\DD
        unsigned long hdr_len,              //\B8\BD\BCÓ¼\F8\B1\F0\CA\FD\BEÝµÄ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        unsigned char msg[],                //\BD\E2\C3\DCÇ°\C3\DC\CE\C4\CA\FD\BEÝ£\AC\BD\E2\C3Üº\F3\C3\F7\CE\C4\CA\FD\BE\DD
        unsigned long msg_len,              //\C3\DC\CE\C4\CA\FD\BEÝµÄ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        unsigned char tag[],                //\BC\F8\B1\F0Ð£\D1\E9\CA\FD\BE\DD
        unsigned long tag_len,              //\BC\F8\B1\F0Ð£\D1\E9\CA\FD\BEÝµÄ³\A4\B6\C8(\B5\A5Î»\D7Ö½\DA)
        gcm_ctx ctx[1]);                    //gcm\CA\FD\BE\DD,\C6\E4\D6Ðº\AC\D3Ð½\E2\C3\DC\C3\DCÔ¿

#ifdef __cplusplus
}
#endif

#endif //GCM_H
