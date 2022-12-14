/* 
 * Copyright ? 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * Module:        Public Key Cryptography Algorithm           
 * Description:   Public key infrastructure application technology 
 *                application interface of cryptography algorithm.
 * Writer:        zhangguoqiang@TEVG <zhanggq@iwncomm.com>
 * Version:  $Id:algapi.h,v 1.0 2011/12/15 00:24:24 zhangguoqiang Exp $     
 *
 * History: 
 * zhangguoqiang    2011/12/15    v1.0    Created.
 */

#ifndef _ECC_ALG_API_H
#define _ECC_ALG_API_H

#ifdef  __cplusplus
extern "C"
{
#endif

#define ECC_CURVE_192_BITS	0
#define ECC_CURVE_256_BITS	1
#define ECC_CURVE_384_BITS	2
#define ECC_CURVE_512_BITS	3

#define HASH_TYPE_SM3		0
#define HASH_TYPE_SHA256	1
    /*--------------------------------------------------------------------*/
#define SHA256_LEN                          32
#define DIGEST_LEN                          64
#define KD_SHA256_LEN						48
#define HSM3_LEN                            32
    /*--------------------------------------------------------------------*/
#define MAX_MINTLENGTH		19
//-----------Ëã·¨±êÊ¶¶¨Òå-----------
//¶Ô³ÆËã·¨±êÊ¶
#define SGD_SMS4_ECB    0x00000401     
#define SGD_SMS4_CBC    0x00000402      
#define SGD_SMS4_OFB    0x00000404      
#define SGD_SMS4_MAC    0x00000408      
//·Ç¶Ô³ÆËã·¨±êÊ¶
#define SGD_SM2         0x00020000      
#define SGD_SM2_1       0x00020100     
#define SGD_SM2_2       0x00020200      
#define SGD_SM2_3       0x00020400
#define SGD_ECC         0x00040000       
#define SGD_ECDSA       0x00040100     
#define SGD_ECDH        0x00040200      
#define SGD_ECES        0x00040400     
//ÔÓ´ÕËã·¨±êÊ¶
#define SGD_SM3         0x00000001    
#define SGD_SHA256      0x00000004     

    /*--------------------------------------------------------------------*/
//´íÎóÂë¶¨Òå
#define SDR_OK								0x0                    
#define SDR_BASE                            0x01000000              
#define SDR_UNKNOWERR						SDR_BASE + 0x00000001   
#define SDR_NOTSUPPORT						SDR_BASE + 0x00000002  
#define SDR_COMMFAIL                        SDR_BASE + 0x00000003  
#define SDR_HARDFAIL                        SDR_BASE + 0x00000004   
#define SDR_OPENDEVICE						SDR_BASE + 0x00000005   
#define SDR_OPENSESSION						SDR_BASE + 0x00000006  
#define SDR_PARDENY                         SDR_BASE + 0x00000007   
#define SDR_KEYNOTEXIST						SDR_BASE + 0x00000008   
#define SDR_ALGNOTSUPPORT					SDR_BASE + 0x00000009   
#define SDR_ALGMODNOTSUPPORT				SDR_BASE + 0x0000000A  
#define SDR_PKOPERR                         SDR_BASE + 0x0000000B   
#define SDR_SKOPERR                         SDR_BASE + 0x0000000C   
#define SDR_SIGNERR                         SDR_BASE + 0x0000000D   
#define SDR_VERIFYERR                       SDR_BASE + 0x0000000E   
#define SDR_SYMOPERR                        SDR_BASE + 0x0000000F   
#define SDR_STEPERR                         SDR_BASE + 0x00000010   
#define SDR_FILESIZEERR                     SDR_BASE + 0x00000011  
#define SDR_FILENOEXIST						SDR_BASE + 0x00000012  
#define SDR_FILEOFSERR						SDR_BASE + 0x00000013   
#define SDR_KEYTYPEERR						SDR_BASE + 0x00000014
#define SDR_CLOSESESSIONERR					SDR_BASE + 0x00000018
#define SDR_DEVRESMALLOCERR					SDR_BASE + 0x0000001B
#define SDR_CLOSEDEVICEERR					SDR_BASE + 0x0000001D
#define SDR_NODEVICEHANDLE					SDR_BASE + 0x00000024
#define SDR_SESSIONHAVECREATED				SDR_BASE + 0x00000025 
#define SDR_NOSESSIONHANDLE					SDR_BASE + 0x00000026
#define SDR_PARAINPUTERR                    SDR_BASE + 0x00000027
#define SDR_SESSIONRESMALLOCERR				SDR_BASE + 0x00000042
    /*--------------------------------------------------------------------*/
#define ECCref_MAX_BITS                 256
#define ECCref_MAX_LEN                  ((ECCref_MAX_BITS+7) / 8)
    /*--------------------------------------------------------------------*/
//Éè±¸ÐÅÏ¢¶¨Òå
    //typedef struct DeviceInfo_st
    typedef struct
    {
        unsigned char IssuerName [ 40 ]; //Éè±¸Éú²ú³§ÉÌÃû³Æ
        unsigned char DeviceName [ 16 ]; //Éè±¸ÐÍºÅ
        unsigned char DeviceSerial [ 16 ]; //Éè±¸±àºÅ
        unsigned int DeviceVersion; //ÃÜÂëÉè±¸ÄÚ²¿Èí¼þµÄ°æ±¾ºÅ
        unsigned int StandardVersion; //ÃÜÂëÉè±¸Ö§³ÖµÄ½Ó¿Ú¹æ·¶°æ±¾ºÅ
        unsigned int AsymAlgAbility [ 2 ]; //ËùÓÐÖ§³ÖµÄ·Ç¶Ô³ÆËã·¨
        unsigned int SymAlgAbility; //ËùÓÐÖ§³ÖµÄ¶Ô³ÆËã·¨
        unsigned int HashAlgAbility; //ËùÓÐÖ§³ÖµÄÔÓ´ÕËã·¨
        unsigned int BufferSize; //Ö§³ÖµÄ×î´óÎÄ¼þ´æ´¢¿Õ¼ä£¨µ¥Î»×Ö½Ú£©
    } DEVICEINFO;
    /*--------------------------------------------------------------------*/
//ECCË½Ô¿Êý¾Ý½á¹¹¶¨Òå
    //typedef struct ECCrefPrivateKey_st
    typedef struct
    {
        unsigned int bits;
        unsigned char D [ ECCref_MAX_LEN ];
    } ECCrefPrivateKey;
//ECC¹«Ô¿Êý¾Ý½á¹¹¶¨Òå
    typedef struct ECCrefPublicKey_st
    {
        unsigned int bits;
        unsigned char x [ ECCref_MAX_LEN ];
        unsigned char y [ ECCref_MAX_LEN ];
    } ECCrefPublicKey;
//ECC¼ÓÃÜÊý¾Ý½á¹¹¶¨Òå
    typedef struct ECCCipher_st
    {
        unsigned char x [ ECCref_MAX_LEN ];
        unsigned char y [ ECCref_MAX_LEN ];
        unsigned char C [ ECCref_MAX_LEN ];
        unsigned char M [ ECCref_MAX_LEN ];
    } ECCCipher;
//Ç©ÃûÊý¾Ý½á¹¹¶¨Òå
    typedef struct ECCSignature_st
    {
        unsigned char r [ ECCref_MAX_LEN ];
        unsigned char s [ ECCref_MAX_LEN ];
    } ECCSignature;
//ECDHÊý¾Ý½á¹¹¶¨Òå
    typedef struct ECCEcdh_st
    {
        unsigned char keyseed [ ECCref_MAX_LEN ];
    } ECCEcdh;

//Ç©ÃûÊý¾Ý½á¹¹¶¨Òå
    typedef struct ECCrefKeyExch_st
    {
        unsigned char S12 [ ECCref_MAX_LEN ];
        unsigned char Sab [ ECCref_MAX_LEN ];
        unsigned char *key;
    } ECCrefKeyExch;

    typedef struct ECCrefPara_st
    {
        unsigned char P [ ECCref_MAX_LEN ];
        unsigned char A [ ECCref_MAX_LEN ];
        unsigned char B [ ECCref_MAX_LEN ];
        unsigned char Gx [ ECCref_MAX_LEN ];
        unsigned char Gy [ ECCref_MAX_LEN ];
        unsigned char N [ ECCref_MAX_LEN ];
        unsigned int mode;
        unsigned int hash_type;
    } ECCrefPara;
    /*--------------------------------------------------------------------*/
    typedef struct
    {
        unsigned int value [ MAX_MINTLENGTH ];
        int len;
    } MINT;

    typedef struct
    {
        int is_infinite;
        MINT x;
        MINT y;
    } ECC_FpPOINT;

    typedef struct
    {
        MINT P;
        MINT A;
        MINT B;
        ECC_FpPOINT G;
        MINT N;
        int bitslen;
    } ECC_FpCURVE;
    /*------------------------------------------------------------------*/
    /*Éè±¸¾ä±ú*/
    typedef struct DeviceHandle_st
    {
        unsigned int State;
        unsigned int CurveType;
        unsigned int CurrentSessions;
        int Flags;
        int DeviceFd;
        void *CurvePara;
        int Curve_length;
    } DeviceHandle;
    /*»á»°¾ä±ú*/
    typedef struct SessionHandle_st
    {
        unsigned int State;
        unsigned int Flags;
        unsigned int len;
        unsigned char PrivateData [ 32 ];
        unsigned int EccCurveType;
        void* EccConfigPara;
        DeviceHandle* DeviceHandle;
        void* KeyHandle;
        int Curve_length;
        unsigned int g_fixlen;
        int g_hash_type;
        ECC_FpCURVE g_fp_curve;
        ECC_FpPOINT g_GPt_1 [ 520 ];
        MINT g_h;
        int g_w;
    } SessionHandle;
    /*ÇúÏßÄ£Ê½ÀàÐÍ*/
    typedef enum _enum_bits
    {
        ECC_API_OPS_BITS_NONE = 0,
        ECC_API_OPS_BIT160,
        ECC_API_OPS_BIT192,
        ECC_API_OPS_BIT224,
        ECC_API_OPS_BIT256,
        ECC_API_OPS_SM2EXP,
        ECC_API_OPS_SM2REC,
        ECC_API_OPS_BITS_MAX
    } enum_bits;
    /*--------------------------------------------------------------------*/
//Éè±¸¹ÜÀíÀàº¯Êý
//´ò¿ªÉè±¸
    int
    SDF_OpenDevice ( void **phDeviceHandle );
//¹Ø±ÕÉè±¸
    int
    SDF_CloseDevice ( void *hDeviceHandle );
//´´½¨»á»°
    int
    SDF_OpenSession ( void *hDeviceHandle , void **phSessionHandle );
//¹Ø±Õ»á»°
    int
    SDF_CloseSession ( void *hSessionHandle );
//»ñÈ¡Éè±¸ÐÅÏ¢
    int
    SDF_GetDeviceInfo ( void *hSessionHandle , DEVICEINFO *pstDeviceInfo );
//²úÉúËæ»úÊý
    int
    SDF_GenerateRandom ( void *hSessionHandle , unsigned int uiLength ,
            unsigned char *pucRandom );
    /*--------------------------------------------------------------------*/
//ÃÜÔ¿¹ÜÀíÀàº¯Êý
//²úÉúECC·Ç¶Ô³ÆÃÜÔ¿¶Ô²¢Êä³ö
    int
    SDF_GenerateKeyPair_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            unsigned int uiKeyBits , ECCrefPublicKey *pucPublicKey ,
            ECCrefPrivateKey *pucPrivateKey );
    /*--------------------------------------------------------------------*/
//·Ç¶Ô³ÆËã·¨ÔËËãÀàº¯Êý
//Íâ²¿ÃÜÔ¿ECCÇ©Ãû
    int
    SDF_ExternalSign_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPrivateKey *pucPrivateKey , unsigned char *pucData ,
            unsigned int uiDataLength , ECCSignature *pucSignature );
//Íâ²¿ÃÜÔ¿ECCÑéÖ¤
    int
    SDF_ExternalVerify_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPublicKey *pucPublicKey , unsigned char *pucDataInput ,
            unsigned int uiInputLength , ECCSignature *pucSignature );
//Íâ²¿ÃÜÔ¿ECC¼ÓÃÜ
    int
    SDF_ExternalEncrytp_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPublicKey *pucPublicKey , unsigned char *pucData ,
            unsigned int uiDataLength , ECCCipher *pucEncData );
//Íâ²¿ÃÜÔ¿ECC½âÃÜ
    int
    SDF_ExternalDecrypt_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPrivateKey *pucPrivateKey , ECCCipher *pucEncData ,
            unsigned char *pucData , unsigned int *puiDataLength );
    /*--------------------------------------------------------------------*/
//ÅäÖÃÇúÏßÀàÐÍ¼°²ÎÊý
    int
    IWN_CurveParaCfg ( void *hSessionHandle , enum_bits CurveType ,
            ECCrefPara *CurvePara );
    /*--------------------------------------------------------------------*/
//SHA256ÔÓ´ÕËã·¨
    int
    IWN_Sha256 ( void *hSessionHandle , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucHash ,
            unsigned int *puiHashLength );
//HMAC-SHA256ÕªÒª¼ÆËãËã·¨
    int
    IWN_HmacSha256 ( void *hSessionHandle , unsigned char *pucKey ,
            unsigned int uiKeyLength , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucHash ,
            unsigned int puiHashLength );
//KD-HMAC-SHA256ÃÜÔ¿µ¼³öËã·¨
    int
    IWN_KdHmacSha256 ( void *hSessionHandle , unsigned char *pucKey ,
            unsigned int uiKeyLength , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucHash ,
            unsigned int *puiHashLength );
    /*--------------------------------------------------------------------*/
//ECDHÃÜÔ¿½»»»Ëã·¨
    int
    IWN_ExternalEcdhKeyAgreement ( void *hSessionHandle ,
            ECCrefPrivateKey *pucPrivateKey , ECCrefPublicKey *pucPublicKey ,
            ECCEcdh *agrKeySeed );
//ECCÊ¹ÓÃÍâ²¿Ë½Ô¿Êý×ÖÇ©ÃûËã·¨
    int
    IWN_ExternalSignHash_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPrivateKey *pucExpPrivateKey , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucID ,
            unsigned int uiIDLength , ECCSignature *pucSignature );
//ECCÊ¹ÓÃÍâ²¿¹«Ô¿Êý×ÖÇ©ÃûÑéÖ¤Ëã·¨
    int
    IWN_ExternalVerifyHash_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPublicKey *pucExpPublicKey , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucID ,
            unsigned int uiIDLength , ECCSignature *pucSignature );
//ECCÊ¹ÓÃÍâ²¿¹«Ô¿¼ÓÃÜËã·¨
    int
    IWN_ExternalEncrytp_ECC ( void *hSessionHandle ,
            ECCrefPublicKey *pucPublicKey , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucEncData ,
            unsigned int *uiEncDataLength );
//ECCÊ¹ÓÃÍâ²¿Ë½Ô¿½âÃÜËã·¨
    int
    IWN_ExternalDecrypt_ECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucPrivateKey , unsigned char * pucEncData ,
            unsigned int uiEncDataLength , unsigned char *pucData ,
            unsigned int *puiDataLength );
    /*--------------------------------------------------------------------*/
//SM3ÔÓ´ÕËã·¨
    int
    IWN_SM3 ( void *hSessionHandle , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucHash ,
            unsigned int *puiHashLength );
//HMAC-SM3ÕªÒª¼ÆËãËã·¨
    int
    IWN_HMAC_SM3 ( void *hSessionHandle , unsigned char* pucData ,
            unsigned int uiDataLength , unsigned char* pucKey ,
            unsigned int uiKeyLength , unsigned char* pucDigest ,
            unsigned int uiDigestLength );
//KD-HMAC-SM3ÃÜÔ¿µ¼³öËã·¨
    int
    IWN_KD_HMAC_SM3 ( void *hSessionHandle , unsigned char* pucData ,
            unsigned int uiDataLength , unsigned char* pucKey ,
            unsigned int uiKeyLength , unsigned char* pucOut ,
            unsigned int uiOutLength );
    /*--------------------------------------------------------------------*/
//SM2·¢Æð·½Éú³ÉÁÙÊ±¹«Ë½Ô¿Êý¾Ý
    int
    IWN_SM2_GenerateAgreementDataWithECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucSponsorTmpPrivateKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey );
//SM2·¢Æð·½¸ù¾ÝÐ­ÉÌ²ÎÊýÉú³É»á»°ÃÜÔ¿
    int
    IWN_SM2_GenerateKeyWithECC ( void *hSessionHandle , unsigned int uiKeyBits ,
            unsigned char *pucSponsorID , unsigned int uiSponsorIDLength ,
            unsigned char *pucResponseID , unsigned int uiResponseIDLength ,
            ECCrefPrivateKey *pucSponsorPrivateKey ,
            ECCrefPrivateKey *pucSponsorTmpPrivateKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey ,
            ECCrefPublicKey *pucResponsePublicKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey ,
            ECCrefKeyExch *pucKeyExch );
//SM2ÏìÓ¦·½2¸ö½Ó¿Ú
//SM2ÏìÓ¦·½Éú³ÉÁÙÊ±¹«Ë½Ô¿Êý¾Ý
    int
    IWN_SM2_GenerateResponseTmpKeyWithECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucResponseTmpPrivateKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey );
//SM2ÏìÓ¦·½¸ù¾ÝÐ­ÉÌ²ÎÊýÉú³É»á»°ÃÜÔ¿
    int
    IWN_SM2_GenerateResponseWithECC ( void *hSessionHandle ,
            unsigned int uiKeyBits , unsigned char *pucSponsorID ,
            unsigned int uiSponsorIDLength , unsigned char *pucResponseID ,
            unsigned int uiResponseIDLength ,
            ECCrefPrivateKey *pucResponsePrivateKey ,
            ECCrefPublicKey *pucSponsorPublicKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey ,
            ECCrefPrivateKey *pucResponseTmpPrivateKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey ,
            ECCrefKeyExch *pucKeyExch );
//SM2ÏìÓ¦·½1¸ö½Ó¿Ú
//SM2ÏìÓ¦·½¸ù¾ÝÐ­ÉÌ²ÎÊýÉú³É»á»°ÃÜÔ¿ºÍÁÙÊ±¹«Ô¿Êý¾Ý
    int
    IWN_SM2_GenerateAgreementDataAndKeyWithECC ( void *hSessionHandle ,
            unsigned int uiKeyBits , unsigned char *pucSponsorID ,
            unsigned int uiSponsorIDLength , unsigned char *pucResponseID ,
            unsigned int uiResponseIDLength ,
            ECCrefPrivateKey *pucResponsePrivateKey ,
            ECCrefPublicKey *pucSponsorPublicKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey ,
            ECCrefKeyExch *pucKeyExch );
//SM2Í¨¹ýÓÃ»§IDÉú³ÉÓÃ»§Êý¾Ý
    int
    IWN_GenerateUserInfo ( void *hSessionHandle ,
            ECCrefPublicKey *pucPublicKey , unsigned char *pucID ,
            unsigned int uiIDLength , unsigned char* UserInfo ,
            unsigned int *uiUserInfoLength );

//SM2·¢Æð·½Éú³ÉÁÙÊ±¹«Ë½Ô¿Êý¾Ý
    int
    IWN_GenerateAgreementDataWithECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucSponsorTmpPrivateKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey );
//SM2·¢Æð·½¸ù¾ÝÐ­ÉÌ²ÎÊýÉú³É»á»°ÃÜÔ¿
    int
    IWN_GenerateKeyWithECC ( void *hSessionHandle , unsigned int uiKeyBits ,
            unsigned char *pucZa , unsigned int uiZaLength ,
            unsigned char *pucZb , unsigned int uiZbLength ,
            ECCrefPrivateKey *pucSponsorPrivateKey ,
            ECCrefPrivateKey *pucSponsorTmpPrivateKey ,
            ECCrefPublicKey *pucResponsePublicKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey ,
            ECCrefKeyExch *pucKeyExch );
//SM2ÏìÓ¦·½2¸ö½Ó¿Ú
//SM2ÏìÓ¦·½Éú³ÉÁÙÊ±¹«Ë½Ô¿Êý¾Ý
    int
    IWN_GenerateResponseTmpKeyWithECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucResponseTmpPrivateKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey );
//SM2ÏìÓ¦·½¸ù¾ÝÐ­ÉÌ²ÎÊýÉú³É»á»°ÃÜÔ¿
    int
    IWN_GenerateResponseWithECC ( void *hSessionHandle ,
            unsigned int uiKeyBits , unsigned char *pucZa ,
            unsigned int uiZaLength , unsigned char *pucZb ,
            unsigned int uiZbLength , ECCrefPrivateKey *pucResponsePrivateKey ,
            ECCrefPublicKey *pucSponsorPublicKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey ,
            ECCrefPrivateKey *pucResponseTmpPrivateKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey ,
            ECCrefKeyExch *pucKeyExch );
//SM2ÏìÓ¦·½1¸ö½Ó¿Ú
//SM2ÏìÓ¦·½¸ù¾ÝÐ­ÉÌ²ÎÊýÉú³É»á»°ÃÜÔ¿ºÍÁÙÊ±¹«Ô¿Êý¾Ý
    int
    IWN_GenerateAgreementDataAndKeyWithECC ( void *hSessionHandle ,
            unsigned int uiKeyBits , unsigned char *pucZa ,
            unsigned int uiZaLength , unsigned char *pucZb ,
            unsigned int uiZbLength , ECCrefPrivateKey *pucResponsePrivateKey ,
            ECCrefPublicKey *pucSponsorPublicKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey ,
            ECCrefKeyExch *pucKeyExch );
//ECC F(p)ÓòÇúÏßÓÐÐ§ÐÔÑéÖ¤
    int
    IWN_CurveVerify ( enum_bits CurveType , ECCrefPara *CurvePara );

//´ò¿ª»á»°²¢ÅäÖÃÇúÏß²ÎÊý
    int
    IWN_OpenSession ( void **phSessionHandle , ECCrefPara *pucEccPara );
//¹Ø±Õ»á»°
    int
    IWN_CloseSession ( void *hSessionHandle );

#ifdef  __cplusplus
}
#endif

#endif /*_ECC_ALG_API_H*/

