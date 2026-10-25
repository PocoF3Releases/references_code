#ifndef _SHA1_H
#define _SHA1_H

#ifdef  __cplusplus
extern "C"
{
#endif

#define SHA_LBLOCK	16
#define SHA_CBLOCK	(SHA_LBLOCK*4)	
#define SHA_LAST_BLOCK  (SHA_CBLOCK-8)
#define SHA_DIGEST_LENGTH 20

    typedef struct SHAstate_st
    {
        unsigned int h0 , h1 , h2 , h3 , h4;
        unsigned int Nl , Nh;
        unsigned int data [ SHA_LBLOCK ];
        unsigned int num;
    } SHA_CTX;

    int
    SHA1_Init ( SHA_CTX *c );
    int
    SHA1_UPDATE ( SHA_CTX *c , const void *data_ , size_t len );
    int
    SHA1_FINAL ( unsigned char *md , SHA_CTX *c );

    unsigned char *
    sha1 ( const unsigned char *buf , unsigned int size ,
            unsigned char *result );
    int
    SHA1_HMAC ( unsigned char* pucData , unsigned int uiDataLength ,
            unsigned char* pucKey , unsigned int uiKeyLength ,
            unsigned char* pucDigest , unsigned int uiDigestLength );

#ifdef  __cplusplus
}
#endif

#endif /*_SHA1_H*/
