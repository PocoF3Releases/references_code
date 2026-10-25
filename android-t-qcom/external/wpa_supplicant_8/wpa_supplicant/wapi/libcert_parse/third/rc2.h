#ifndef _RC2_H
#define _RC2_H

#ifdef  __cplusplus
extern "C"
{
#endif

    void
    rc2_encrypt_cbc ( const unsigned short xkey [ 64 ] ,
            const unsigned char *plain , int len , unsigned char *iv ,
            unsigned char *cipher );

    void
    rc2_decrypt_cbc ( const unsigned short xkey [ 64 ] ,
            const unsigned char *cipher , int len , unsigned char *iv ,
            unsigned char *plain );

    void
    rc2_keyschedule ( unsigned short xkey [ 64 ] , const unsigned char *key ,
            int len , int bits );

    int
    p12_rc2_cbc_encrypt ( const unsigned char *indata , int in_len ,
            unsigned char *iv , int iv_len , unsigned char *key , int key_len ,
            unsigned char *outdata , int *out_len );

    int
    p12_rc2_cbc_decrypt ( const unsigned char *indata , int in_len ,
            unsigned char *iv , int iv_len , unsigned char *key , int key_len ,
            unsigned char *outdata , int *out_len );

#ifdef  __cplusplus
}
#endif

#endif /*_RC2_H*/
