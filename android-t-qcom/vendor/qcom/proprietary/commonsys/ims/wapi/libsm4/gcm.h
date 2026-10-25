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

//gcmÊý¾Ý½á¹¹¶¨Òå
typedef struct 
{
	sm4_encrypt_ctx sm4_ctx;    //sm4ÃÜÔ¿Êý¾Ý
    word64 p[16];              //ÓÃÓÚ±í¸ñ³Ë·¨Ô¤¼ÆËãÊ±´æ´¢Êý¾Ý
    word64 q[16];              //ÓÃÓÚ±í¸ñ³Ë·¨Ô¤¼ÆËãÊ±´æ´¢Êý¾Ý
}gcm_ctx;

#ifdef __cplusplus
extern "C" {
#endif

//±í¸ñ³Ë·¨Ô¤¼ÆËãÊý¾ÝºÍÉèÖÃÃÜÔ¿
int gcm_init_and_key(                     
        const unsigned char key[],          //ÃÜÔ¿Êý¾Ý
        unsigned long key_len,              //ÃÜÔ¿Êý¾Ý³¤¶È(µ¥Î»×Ö½Ú)
        gcm_ctx ctx[1]);                    //gcmÊý¾Ý

//ÇåÀíÊý¾Ý
int gcm_end(                               
        gcm_ctx ctx[1]);                    


//gcmÄ£Ê½¼ÓÃÜ
int gcm_encrypt_message(                   
        const unsigned char iv[],           //³õÊ¼»¯ÏòÁ¿Êý¾Ý
        unsigned long iv_len,               //³õÊ¼»¯ÏòÁ¿Êý¾ÝµÄ³¤¶È(µ¥Î»×Ö½Ú)
        const unsigned char hdr[],          //¸½¼Ó¼ø±ðÊý¾Ý
        unsigned long hdr_len,              //¸½¼Ó¼ø±ðÊý¾ÝµÄ³¤¶È(µ¥Î»×Ö½Ú)
        unsigned char msg[],                //¼ÓÃÜÇ°Ã÷ÎÄÊý¾Ý£¬¼ÓÃÜºóÃÜÎÄÊý¾Ý
        unsigned long msg_len,              //Ã÷ÎÄÊý¾ÝµÄ³¤¶È(µ¥Î»×Ö½Ú)
        unsigned char tag[],                //¼ø±ðÐ£ÑéÊý¾Ý
        unsigned long tag_len,              //¼ø±ðÐ£ÑéÊý¾ÝµÄ³¤¶È(µ¥Î»×Ö½Ú)
        gcm_ctx ctx[1]);                    //gcmÊý¾Ý,ÆäÖÐº¬ÓÐ¼ÓÃÜÃÜÔ¿

//gcmÄ£Ê½½âÃÜ
int gcm_decrypt_message(                    
        const unsigned char iv[],           //³õÊ¼»¯ÏòÁ¿Êý¾Ý
        unsigned long iv_len,               //³õÊ¼»¯ÏòÁ¿Êý¾ÝµÄ³¤¶È(µ¥Î»×Ö½Ú)
        const unsigned char hdr[],          //¸½¼Ó¼ø±ðÊý¾Ý
        unsigned long hdr_len,              //¸½¼Ó¼ø±ðÊý¾ÝµÄ³¤¶È(µ¥Î»×Ö½Ú)
        unsigned char msg[],                //½âÃÜÇ°ÃÜÎÄÊý¾Ý£¬½âÃÜºóÃ÷ÎÄÊý¾Ý
        unsigned long msg_len,              //ÃÜÎÄÊý¾ÝµÄ³¤¶È(µ¥Î»×Ö½Ú)
        unsigned char tag[],                //¼ø±ðÐ£ÑéÊý¾Ý
        unsigned long tag_len,              //¼ø±ðÐ£ÑéÊý¾ÝµÄ³¤¶È(µ¥Î»×Ö½Ú)
        gcm_ctx ctx[1]);                    //gcmÊý¾Ý,ÆäÖÐº¬ÓÐ½âÃÜÃÜÔ¿

#ifdef __cplusplus
}
#endif

#endif //GCM_H
