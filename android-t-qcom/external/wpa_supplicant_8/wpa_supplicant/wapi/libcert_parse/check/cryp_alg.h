//2015/11/17 \D0Þ¸\C4
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

// \D1\E9Ö¤\D3Ã»\A7Ö¤\CA\E9\CAÇ·\F1Îª\B8\C3CA\B0ä·¢ \CAÇ·\B5\BB\D8TRUE \B2\BB\CAÇ·\B5\BB\D8FALSE;
    int
    IWN_Check_UserCert_by_CACert ( const unsigned char *pUserCert , // \D3Ã»\A7Ö¤\CA\E9\CA\FD\BE\DD;
            long usercertlen , // \D3Ã»\A7Ö¤\CA\E9\CA\FD\BEÝ³\A4\B6\C8;
            const unsigned char *pCACert , // CAÖ¤\CA\E9\CA\FD\BE\DD;
            long cacertlen , // CAÖ¤\CA\E9\CA\FD\BEÝ³\A4\B6\C8;
            int curve_mode ); // \C7\FA\CF\DFÄ£Ê½

// \D1\E9Ö¤\D3Ã»\A7Ö¤\CA\E9\CAÇ·\F1\D3\EB\B8\C3Ë½Ô¿\C5\E4\B6\D4 \CAÇ·\B5\BB\D8TRUE \B2\BB\CAÇ·\B5\BB\D8FALSE;
    int
    IWN_Match_Pub_Pri_key ( unsigned char *pUserCert , // \D3Ã»\A7Ö¤\CA\E9\CA\FD\BE\DD;
            long usercertlen , // \D3Ã»\A7Ö¤\CA\E9\CA\FD\BEÝ³\A4\B6\C8;
            unsigned char *pPriKey , // \D3Ã»\A7Ö¤\CA\E9Ë½Ô¿\CA\FD\BE\DD;
            long prikeylen , int // \D3Ã»\A7Ö¤\CA\E9Ë½Ô¿\CA\FD\BEÝ³\A4\B6\C8;
            curve_mode ); // \C7\FA\CF\DFÄ£Ê½

#ifdef  __cplusplus
}
#endif

#endif// _CRYP_ALG_H_
