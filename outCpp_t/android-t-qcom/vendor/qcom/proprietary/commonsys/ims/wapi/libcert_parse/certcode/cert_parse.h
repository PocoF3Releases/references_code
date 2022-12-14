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
    cert_info *serial_no; /*ÐòÁÐºÅ*/
    cert_info *asu_name; /*°ä·¢Õß*/
    cert_info *asue_name; /*³ÖÓÐÕß*/
    cert_info *date_before; /*ÓÐÐ§ÆÚ¿ªÊ¼Ê±¼ä*/
    cert_info *date_after; /*ÓÐÐ§ÈÕÆÚ*/
    cert_info *issuer; /*Ö¤Êé°ä·¢»ú¹¹*/
} certs_info;


void *
get_buffer ( int len );

void *
free_buffer ( void *buffer , int len );

/*×ª»»Ö¤ÊéÊ±¼äÃëÎªÓÃ»§¿É¶Á¸ñÊ½
 *data£ºÖ¤ÊéÊ±¼äÄÚÈÝ
 *length£ºÊ±¼ä³¤¶È
 *err£ºÊ§°ÜÔ­ÒòÂë
 *·µ»ØÖµ£º ±¾µØÊ±ÇøÊ±¼ä£¬ ³É¹¦ £º  0 ×ª»»Ê§°Ü
 */
//static time_t
//Convert_format_time ( unsigned char *data , int length , int *err );

/*¹ýÂËµôÖ¤ÊéÄÚ¿Õ¸ñ
 *char *p:ÊäÈëµÄÖ¤ÊéÄÚÈÝÒÔ¼°Êä³öµÄ¹ýÂËºóµÄÖ¤ÊéÄÚÈÝ
 *int len:ÊäÈëµÄÖ¤Êé³¤¶È
 *·µ»ØÖµ£º Ö¤Êé¹ýÂËºó³¤¶È£¬ ³É¹¦ £º  0 Ö¤ÊéÎª¿Õ
 */
int
Cert_Str_filter ( char *p , int len );

/*ÅÐ¶ÏÖ¤ÊéÊÇ·ñÊÇwapiÀàÖ¤Êé
 *asue_cert£ºÊäÈëÖ¤ÊéÄÚÈÝ
 *asue_len:ÊäÈëµÄÖ¤Êé³¤¶È
 *·µ»ØÖµ£º 1 ³É¹¦  0 ½âÂëÊ§°Ü
 */
int
Is_Wapi_Cert ( unsigned char *asue_cert , int asue_len );

/*¼ì²éP12Ö¤ÊéÄÚÈÝ
 *inp12: ÊäÈëµÄp12ÓÃ»§Ö¤Êé
 *len:ÓÃ»§Ö¤Êé³¤¶È
 *password:Ö¤ÊéÃÜÂë
 *·µ»ØÖµ£º 0 ³É¹¦  -1 ½âÂëÊ§°Ü
 */
int
Check_P12_Data ( const unsigned char *inp12 , int len , char* password );

/*½âÂë°ä·¢ÕßÖ¤Êé£¬Êä³ö½âÂëºóÖ¤Êé
 *asucert£ºÊäÈëµÄ°ä·¢ÕßÖ¤ÊéÄÚÈÝ
 *asu_len:ÊäÈëµÄÖ¤Êé³¤¶È
 *asucert_out: ½âÂëºóµÄ°ä·¢ÕßÖ¤Êé
 *asucert_outlen:½âÂëºóµÄ°ä·¢ÕßÖ¤Êé³¤¶È
 *·µ»ØÖµ£º 0 ³É¹¦  -1 ½âÂëÊ§°Ü
 */
int
Unpack_AsuCert ( const void *asucert , int asu_len ,
        unsigned char *asucert_out , int *asucert_outlen );

/*½âÂëÓÃ»§Ö¤Êé£¬Êä³ö½âÂëºóÖ¤ÊéÒÔ¼°Ë½Ô¿
 *incert£ºÊäÈëµÄÓÃ»§Ö¤ÊéÄÚÈÝ
 *cert_len:ÊäÈëµÄÖ¤Êé³¤¶È
 *usrcert_out: ½âÂëºóµÄÖ¤Êé
 *usrcert_outlen:½âÂëºóµÄÖ¤Êé³¤¶È
 *prikey_out:Ë½Ô¿ÄÚÈÝ
 *prikey_outlen£ºË½Ô¿³¤¶È
 *·µ»ØÖµ£º 0 ³É¹¦  -1 ½âÂëÊ§°Ü
 */
int
Unpack_AsueCert ( const void *incert , int cert_len ,
        unsigned char *usrcert_out , unsigned short *usrcert_outlen );

/*½âÂëÓÃ»§Ë½Ô¿£¬Êä³ö½âÂëË½Ô¿
*prikey: Ë½Ô¿ÄÚÈÝ
*prikey_len:Ë½Ô¿ÄÚÈÝ³¤¶È
*prikey_out:Êä³öµÄ½âÂëË½Ô¿
*prikey_outlen£ºÊä³öµÄ½âÂëË½Ô¿³¤¶È
*·µ»ØÖµ£º 0 ³É¹¦  -1 ½âÂëÊ§°Ü
*/
int Unpack_AsuePrikey(const void *prikey, int prikey_len,
        unsigned char *prikey_out , unsigned short *prikey_outlen );
/*»ñÈ¡Ö¤Êé¹«Ô¿
 *incert£ºÖ¤ÊéÄÚÈÝ
 *incert_len:Ö¤Êé³¤¶È
 *pubkey: Êä³ö¹«Ô¿
 *pubkey_len:¹«Ô¿³¤¶È
*·µ»ØÖµ£º 0 ³É¹¦  -1 »ñÈ¡Ê§°Ü
 */
int
X509_Get_Pubkey ( unsigned char *incert , int incert_len ,
        unsigned char *pubkey , int *pubkey_len );
/*»ñÈ¡Ö¤ÊéË½Ô¿
*inprikey£ºË½Ô¿ÄÚÈÝ
*inprikey_len:Ë½Ô¿³¤¶È
*prikey: Êä³öË½Ô¿
*prikey_len:Ë½Ô¿³¤¶È
*·µ»ØÖµ£º 0 ³É¹¦  -1 »ñÈ¡Ê§°Ü
*/
int 
X509_Get_Prikey( unsigned char *inprikey , int inprikey_len ,
		unsigned char *prikey , int *prikey_len );
/*»ñÈ¡Ö¤Êé×Ö¶ÎÐÅÏ¢
 *incert£ºÖ¤ÊéÄÚÈÝ
 *len:Ö¤Êé³¤¶È
 *type: 0, 1 x509Ö¤Êé
 *struct wapi_certs_info *outinfo:Ö¤Êé×Ö¶ÎÐÅÏ¢½á¹¹Ìå
 *·µ»ØÖµ£º 0 ³É¹¦  -1 ½âÎöÊ§°Ü
 */
int 
X509_Get_Cert_Info(unsigned char *incert , unsigned short len , 
 							certs_info *certinfo );

#endif  /* _CERT_H_ end */
