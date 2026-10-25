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
 * cert_parse.h
 *
 * Description:
 *
 */

#ifndef _CERT_H_
#define _CERT_H_

#define   CERT_BEGIN "-----BEGIN CERTIFICATE-----"
#define   CERT_END "-----END CERTIFICATE-----"

#define   PKEY_BEGIN "-----BEGIN EC PRIVATE KEY-----"
#define   PKEY_END "-----END EC PRIVATE KEY-----"


#define SIGN_LEN            48  
#define CHAR_LENGTH       256

typedef struct _cert_info
{
    unsigned char length;
    unsigned char *data;
} cert_info;

typedef struct _certs_info
{
    cert_info *serial_no; /*\D0\F2\C1Ðº\C5*/
    cert_info *asu_name; /*\B0ä·¢\D5\DF*/
    cert_info *asue_name; /*\B3\D6\D3\D0\D5\DF*/
    cert_info *date_before; /*\D3\D0Ð§\C6Ú¿\AAÊ¼Ê±\BC\E4*/
    cert_info *date_after; /*\D3\D0Ð§\C8\D5\C6\DA*/
    cert_info *issuer; /*Ö¤\CA\E9\B0ä·¢\BB\FA\B9\B9*/
} certs_info;


void *
get_buffer ( int len );

void *
free_buffer ( void *buffer , int len );

/*×ª\BB\BBÖ¤\CA\E9Ê±\BC\E4\C3\EBÎª\D3Ã»\A7\BFÉ¶\C1\B8\F1Ê½
 *data\A3\BAÖ¤\CA\E9Ê±\BC\E4\C4\DA\C8\DD
 *length\A3\BAÊ±\BCä³¤\B6\C8
 *err\A3\BAÊ§\B0\DCÔ­\D2\F2\C2\EB
 *\B7\B5\BB\D8Öµ\A3\BA \B1\BE\B5\D8Ê±\C7\F8Ê±\BCä£¬ \B3É¹\A6 \A3\BA  0 ×ª\BB\BBÊ§\B0\DC
 */
//static time_t
//Convert_format_time ( unsigned char *data , int length , int *err );

/*\B9\FD\C2Ëµ\F4Ö¤\CA\E9\C4Ú¿Õ¸\F1
 *char *p:\CA\E4\C8\EB\B5\C4Ö¤\CA\E9\C4\DA\C8\DD\D2Ô¼\B0\CA\E4\B3\F6\B5Ä¹\FD\C2Ëº\F3\B5\C4Ö¤\CA\E9\C4\DA\C8\DD
 *int len:\CA\E4\C8\EB\B5\C4Ö¤\CAé³¤\B6\C8
 *\B7\B5\BB\D8Öµ\A3\BA Ö¤\CA\E9\B9\FD\C2Ëºó³¤¶È£\AC \B3É¹\A6 \A3\BA  0 Ö¤\CA\E9Îª\BF\D5
 */
int
Cert_Str_filter ( char *p , int len );

/*\C5Ð¶\CFÖ¤\CA\E9\CAÇ·\F1\CA\C7wapi\C0\E0Ö¤\CA\E9
 *asue_cert\A3\BA\CA\E4\C8\EBÖ¤\CA\E9\C4\DA\C8\DD
 *asue_len:\CA\E4\C8\EB\B5\C4Ö¤\CAé³¤\B6\C8
 *\B7\B5\BB\D8Öµ\A3\BA 1 \B3É¹\A6  0 \BD\E2\C2\EBÊ§\B0\DC
 */
int
Is_Wapi_Cert ( unsigned char *asue_cert , int asue_len );

/*\BC\EC\B2\E9P12Ö¤\CA\E9\C4\DA\C8\DD
 *inp12: \CA\E4\C8\EB\B5\C4p12\D3Ã»\A7Ö¤\CA\E9
 *len:\D3Ã»\A7Ö¤\CAé³¤\B6\C8
 *password:Ö¤\CA\E9\C3\DC\C2\EB
 *\B7\B5\BB\D8Öµ\A3\BA 0 \B3É¹\A6  -1 \BD\E2\C2\EBÊ§\B0\DC
 */
int
Check_P12_Data ( const unsigned char *inp12 , int len , char* password );

/*\BD\E2\C2\EB\B0ä·¢\D5\DFÖ¤\CAé£¬\CA\E4\B3\F6\BD\E2\C2\EB\BA\F3Ö¤\CA\E9
 *asucert\A3\BA\CA\E4\C8\EB\B5Ä°ä·¢\D5\DFÖ¤\CA\E9\C4\DA\C8\DD
 *asu_len:\CA\E4\C8\EB\B5\C4Ö¤\CAé³¤\B6\C8
 *asucert_out: \BD\E2\C2\EB\BA\F3\B5Ä°ä·¢\D5\DFÖ¤\CA\E9
 *asucert_outlen:\BD\E2\C2\EB\BA\F3\B5Ä°ä·¢\D5\DFÖ¤\CAé³¤\B6\C8
 *\B7\B5\BB\D8Öµ\A3\BA 0 \B3É¹\A6  -1 \BD\E2\C2\EBÊ§\B0\DC
 */
int
Unpack_AsuCert ( const void *asucert , int asu_len ,
        unsigned char *asucert_out , int *asucert_outlen );

/*\BD\E2\C2\EB\D3Ã»\A7Ö¤\CAé£¬\CA\E4\B3\F6\BD\E2\C2\EB\BA\F3Ö¤\CA\E9\D2Ô¼\B0Ë½Ô¿
 *incert\A3\BA\CA\E4\C8\EB\B5\C4\D3Ã»\A7Ö¤\CA\E9\C4\DA\C8\DD
 *cert_len:\CA\E4\C8\EB\B5\C4Ö¤\CAé³¤\B6\C8
 *usrcert_out: \BD\E2\C2\EB\BA\F3\B5\C4Ö¤\CA\E9
 *usrcert_outlen:\BD\E2\C2\EB\BA\F3\B5\C4Ö¤\CAé³¤\B6\C8
 *prikey_out:Ë½Ô¿\C4\DA\C8\DD
 *prikey_outlen\A3\BAË½Ô¿\B3\A4\B6\C8
 *\B7\B5\BB\D8Öµ\A3\BA 0 \B3É¹\A6  -1 \BD\E2\C2\EBÊ§\B0\DC
 */
int
Unpack_AsueCert ( const void *incert , int cert_len ,
        unsigned char *usrcert_out , unsigned short *usrcert_outlen );

/*\BD\E2\C2\EB\D3Ã»\A7Ë½Ô¿\A3\AC\CA\E4\B3\F6\BD\E2\C2\EBË½Ô¿
*prikey: Ë½Ô¿\C4\DA\C8\DD
*prikey_len:Ë½Ô¿\C4\DA\C8Ý³\A4\B6\C8
*prikey_out:\CA\E4\B3\F6\B5Ä½\E2\C2\EBË½Ô¿
*prikey_outlen\A3\BA\CA\E4\B3\F6\B5Ä½\E2\C2\EBË½Ô¿\B3\A4\B6\C8
*\B7\B5\BB\D8Öµ\A3\BA 0 \B3É¹\A6  -1 \BD\E2\C2\EBÊ§\B0\DC
*/
int Unpack_AsuePrikey(const void *prikey, int prikey_len,
        unsigned char *prikey_out , unsigned short *prikey_outlen );
/*\BB\F1È¡Ö¤\CAé¹«Ô¿
 *incert\A3\BAÖ¤\CA\E9\C4\DA\C8\DD
 *incert_len:Ö¤\CAé³¤\B6\C8
 *pubkey: \CA\E4\B3\F6\B9\ABÔ¿
 *pubkey_len:\B9\ABÔ¿\B3\A4\B6\C8
*\B7\B5\BB\D8Öµ\A3\BA 0 \B3É¹\A6  -1 \BB\F1È¡Ê§\B0\DC
 */
int
X509_Get_Pubkey ( unsigned char *incert , int incert_len ,
        unsigned char *pubkey , int *pubkey_len );
/*\BB\F1È¡Ö¤\CA\E9Ë½Ô¿
*inprikey\A3\BAË½Ô¿\C4\DA\C8\DD
*inprikey_len:Ë½Ô¿\B3\A4\B6\C8
*prikey: \CA\E4\B3\F6Ë½Ô¿
*prikey_len:Ë½Ô¿\B3\A4\B6\C8
*\B7\B5\BB\D8Öµ\A3\BA 0 \B3É¹\A6  -1 \BB\F1È¡Ê§\B0\DC
*/
int 
X509_Get_Prikey( unsigned char *inprikey , int inprikey_len ,
		unsigned char *prikey , int *prikey_len );
/*\BB\F1È¡Ö¤\CA\E9\D7Ö¶\CE\D0\C5Ï¢
 *incert\A3\BAÖ¤\CA\E9\C4\DA\C8\DD
 *len:Ö¤\CAé³¤\B6\C8
 *type: 0, 1 x509Ö¤\CA\E9
 *struct wapi_certs_info *outinfo:Ö¤\CA\E9\D7Ö¶\CE\D0\C5Ï¢\BDá¹¹\CC\E5
 *\B7\B5\BB\D8Öµ\A3\BA 0 \B3É¹\A6  -1 \BD\E2\CE\F6Ê§\B0\DC
 */
int 
X509_Get_Cert_Info(unsigned char *incert , unsigned short len , 
 							certs_info *certinfo );

#endif  /* _CERT_H_ end */
