//2015/11/17 ÐÞ¸Ä
#ifndef _CRYP_ALG_H_
#define _CRYP_ALG_H_

#ifdef  __cplusplus
extern "C"
{
#endif

#include "typedef_1.h"
#include "algapi.h"

#define SM2_P256     0
#define ECC_P192     1

    int
    cryplib_init ( SessionHandle **session , int curve_mode );
    void
    cryplib_exit ( SessionHandle *session );
    int
    create_sm2key_bin ( p8 k );
    int
    create_ecc192key_bin ( p8 k );
    int
    Set_Pubkey_X509 ( unsigned char *in_data , int len ,
            unsigned char **outdata );
    int
    Set_Pubkey_192_X509 ( unsigned char *in_data , int len ,
            unsigned char **outdata );
    int
    Sign_X509 ( unsigned char *key , unsigned char *indata , int inlen ,
            unsigned char *puc_id , int id_len , unsigned char **sign_data ,
            int *sign_len , unsigned char **outdata );
    int
    Vrfy_X509 ( unsigned char *key , unsigned char *indata , int inlen ,
            unsigned char *puc_id , int id_len , unsigned char *sign_data );
    int
    Sign_192_X509 ( unsigned char *key , unsigned char *indata , int inlen ,
            unsigned char **sign_data , int *sign_len ,
            unsigned char **outdata );
    int
    Vrfy_192_X509 ( unsigned char *key , unsigned char *indata , int inlen ,
            unsigned char *sign_data );

// ÑéÖ¤ÓÃ»§Ö¤ÊéÊÇ·ñÎª¸ÃCA°ä·¢ ÊÇ·µ»ØTRUE ²»ÊÇ·µ»ØFALSE;
    int
    IWN_Check_UserCert_by_CACert ( const unsigned char *pUserCert , // ÓÃ»§Ö¤ÊéÊý¾Ý;
            long usercertlen , // ÓÃ»§Ö¤ÊéÊý¾Ý³¤¶È;
            const unsigned char *pCACert , // CAÖ¤ÊéÊý¾Ý;
            long cacertlen , // CAÖ¤ÊéÊý¾Ý³¤¶È;
            int curve_mode ); // ÇúÏßÄ£Ê½

// ÑéÖ¤ÓÃ»§Ö¤ÊéÊÇ·ñÓë¸ÃË½Ô¿Åä¶Ô ÊÇ·µ»ØTRUE ²»ÊÇ·µ»ØFALSE;
    int
    IWN_Match_Pub_Pri_key ( unsigned char *pUserCert , // ÓÃ»§Ö¤ÊéÊý¾Ý;
            long usercertlen , // ÓÃ»§Ö¤ÊéÊý¾Ý³¤¶È;
            unsigned char *pPriKey , // ÓÃ»§Ö¤ÊéË½Ô¿Êý¾Ý;
            long prikeylen , int // ÓÃ»§Ö¤ÊéË½Ô¿Êý¾Ý³¤¶È;
            curve_mode ); // ÇúÏßÄ£Ê½

#ifdef  __cplusplus
}
#endif

#endif// _CRYP_ALG_H_
