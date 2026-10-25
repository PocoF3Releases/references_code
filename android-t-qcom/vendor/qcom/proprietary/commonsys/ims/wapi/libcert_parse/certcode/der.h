/*
 * Copyright (C) 2016-2020 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * This module is wapi cert parse function.
 * this function parse cert.
 *
 * Authors:
 * <iwncomm@iwncomm.com>
 *
 * History:
 *   06/06/2016 v1.0 add by iwncomm
 *
 * Related documents:
 * der.h
 *
 * Description:
 *
 */
#ifndef _DER_H
#define _DER_H

#ifdef  __cplusplus
extern "C"
{
#endif

#define UNIVERSAL		            0x00
#define CTX_SPEC		            0x80
#define SEQUENCE		            16
#define OBJ			                6
#define CTX_TAG                     0x8f

#define INTEGER                     2
#define NEG_INTEGER                 (2+0x100)
#define OCTET_STRING                4
#define BIT_STRING                  3
#define ASN1_NULL                   5
#define SET                         17
#define PRINTABLESTRING             19
#define T61STRING                   20
#define IA5STRING                   22
#define UTCTIME                     23

#define CTX_SPEC_PUB                0x81
#define CTX_CET                     0xa0
#define CTX_EXT		                0x83

    int
    Der_Obj_Len ( int len , int tag , int truct );
    void
    Der_Put_Len ( unsigned char **data , int len );
    void
    Der_Put_Obj ( unsigned char **data , int len , int tag , int truct ,
            int type );
    int
    Der_Obj ( unsigned char *indata , int inlen , unsigned char **outdata );
    int
    Der_Alg ( unsigned char *indata , int inlen , unsigned char **outdata );

    int
    Der_Bit_String ( unsigned char *indata , int inlen ,
            unsigned char **outdata );
    int
    Der_Bytes ( unsigned char *indata , int inlen , int type , int tag ,
            unsigned char **outdata );
    int
    Der_Integer ( unsigned char *indata , int inlen , unsigned char **outdata );

    int
    chart_to_int ( unsigned char *data , int len );
    int
    int_to_char ( unsigned int int_data , unsigned char **data , int *len );

    int
    Gen_Mac_key ( unsigned char *data , int data_len , unsigned char *salt ,
            int salt_len , const char *pass , int passlen , int iter ,
            unsigned char *mac_data , int *len );

    int
    Verify_Mac ( unsigned char *mac_value , int mac_len , unsigned char *data ,
            int data_len , unsigned char *mac_salt , int mac_salt_len ,
            int mac_iter , const char *pass , int passlen );

#ifdef  __cplusplus
}
#endif

#endif /*_DER_H*/
