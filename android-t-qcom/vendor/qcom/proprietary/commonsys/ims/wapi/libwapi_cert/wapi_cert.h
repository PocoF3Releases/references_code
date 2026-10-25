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
 * pi yongping 06/06/2016 v1.0 add by piyp
 *
 * Related documents:
 * wapi_cert.h
 *
 * Description:
 *
 */
#ifndef WAPI_CERT_H_
#define WAPI_CERT_H_

#define SERIAL2STR(a) (a)[0], (a)[1], (a)[2], (a)[3]

int
Is_P12_Cert ( const char *user_cert , int user_cert_len );
int
Check_Asue_Asu_Cert ( const unsigned char *user_cert , int user_cert_len ,
        const unsigned char *pri_key , int pri_key_len ,
        const unsigned char *as_cert , int as_cert_len );
int
Parse_P12_Cert ( const unsigned char *inp12 , int len , char *password ,
        unsigned char *usrcert , unsigned short *usrcert_len ,
        unsigned char *privatekey , int *privatekey_len );

int
Get_Asue_Cert_Info ( char *usrcert , int certlen ,char *certinfo);
#endif /* WAPI_CERT_H_ */
