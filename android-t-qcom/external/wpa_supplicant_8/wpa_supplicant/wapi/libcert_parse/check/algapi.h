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
//-----------\CBã·¨\B1\EAÊ¶\B6\A8\D2\E5-----------
//\B6Ô³\C6\CBã·¨\B1\EAÊ¶
#define SGD_SMS4_ECB    0x00000401     
#define SGD_SMS4_CBC    0x00000402      
#define SGD_SMS4_OFB    0x00000404      
#define SGD_SMS4_MAC    0x00000408      
//\B7Ç¶Ô³\C6\CBã·¨\B1\EAÊ¶
#define SGD_SM2         0x00020000      
#define SGD_SM2_1       0x00020100     
#define SGD_SM2_2       0x00020200      
#define SGD_SM2_3       0x00020400
#define SGD_ECC         0x00040000       
#define SGD_ECDSA       0x00040100     
#define SGD_ECDH        0x00040200      
#define SGD_ECES        0x00040400     
//\D4Ó´\D5\CBã·¨\B1\EAÊ¶
#define SGD_SM3         0x00000001    
#define SGD_SHA256      0x00000004     

    /*--------------------------------------------------------------------*/
//\B4\ED\CE\F3\C2ë¶¨\D2\E5
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
//\C9è±¸\D0\C5Ï¢\B6\A8\D2\E5
    //typedef struct DeviceInfo_st
    typedef struct
    {
        unsigned char IssuerName [ 40 ]; //\C9è±¸\C9\FA\B2\FA\B3\A7\C9\CC\C3\FB\B3\C6
        unsigned char DeviceName [ 16 ]; //\C9è±¸\D0Íº\C5
        unsigned char DeviceSerial [ 16 ]; //\C9è±¸\B1\E0\BA\C5
        unsigned int DeviceVersion; //\C3\DC\C2\EB\C9è±¸\C4Ú²\BF\C8\ED\BC\FE\B5Ä°æ±¾\BA\C5
        unsigned int StandardVersion; //\C3\DC\C2\EB\C9è±¸Ö§\B3ÖµÄ½Ó¿Ú¹æ·¶\B0æ±¾\BA\C5
        unsigned int AsymAlgAbility [ 2 ]; //\CB\F9\D3\D0Ö§\B3ÖµÄ·Ç¶Ô³\C6\CBã·¨
        unsigned int SymAlgAbility; //\CB\F9\D3\D0Ö§\B3ÖµÄ¶Ô³\C6\CBã·¨
        unsigned int HashAlgAbility; //\CB\F9\D3\D0Ö§\B3Öµ\C4\D4Ó´\D5\CBã·¨
        unsigned int BufferSize; //Ö§\B3Öµ\C4\D7\EE\B4\F3\CEÄ¼\FE\B4æ´¢\BFÕ¼ä£¨\B5\A5Î»\D7Ö½Ú£\A9
    } DEVICEINFO;
    /*--------------------------------------------------------------------*/
//ECCË½Ô¿\CA\FD\BEÝ½á¹¹\B6\A8\D2\E5
    //typedef struct ECCrefPrivateKey_st
    typedef struct
    {
        unsigned int bits;
        unsigned char D [ ECCref_MAX_LEN ];
    } ECCrefPrivateKey;
//ECC\B9\ABÔ¿\CA\FD\BEÝ½á¹¹\B6\A8\D2\E5
    typedef struct ECCrefPublicKey_st
    {
        unsigned int bits;
        unsigned char x [ ECCref_MAX_LEN ];
        unsigned char y [ ECCref_MAX_LEN ];
    } ECCrefPublicKey;
//ECC\BC\D3\C3\DC\CA\FD\BEÝ½á¹¹\B6\A8\D2\E5
    typedef struct ECCCipher_st
    {
        unsigned char x [ ECCref_MAX_LEN ];
        unsigned char y [ ECCref_MAX_LEN ];
        unsigned char C [ ECCref_MAX_LEN ];
        unsigned char M [ ECCref_MAX_LEN ];
    } ECCCipher;
//Ç©\C3\FB\CA\FD\BEÝ½á¹¹\B6\A8\D2\E5
    typedef struct ECCSignature_st
    {
        unsigned char r [ ECCref_MAX_LEN ];
        unsigned char s [ ECCref_MAX_LEN ];
    } ECCSignature;
//ECDH\CA\FD\BEÝ½á¹¹\B6\A8\D2\E5
    typedef struct ECCEcdh_st
    {
        unsigned char keyseed [ ECCref_MAX_LEN ];
    } ECCEcdh;

//Ç©\C3\FB\CA\FD\BEÝ½á¹¹\B6\A8\D2\E5
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
    /*\C9è±¸\BE\E4\B1\FA*/
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
    /*\BBá»°\BE\E4\B1\FA*/
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
    /*\C7\FA\CF\DFÄ£Ê½\C0\E0\D0\CD*/
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
//\C9è±¸\B9\DC\C0\ED\C0àº¯\CA\FD
//\B4\F2\BF\AA\C9è±¸
    int
    SDF_OpenDevice ( void **phDeviceHandle );
//\B9Ø±\D5\C9è±¸
    int
    SDF_CloseDevice ( void *hDeviceHandle );
//\B4\B4\BD\A8\BBá»°
    int
    SDF_OpenSession ( void *hDeviceHandle , void **phSessionHandle );
//\B9Ø±Õ»á»°
    int
    SDF_CloseSession ( void *hSessionHandle );
//\BB\F1È¡\C9è±¸\D0\C5Ï¢
    int
    SDF_GetDeviceInfo ( void *hSessionHandle , DEVICEINFO *pstDeviceInfo );
//\B2\FA\C9\FA\CB\E6\BB\FA\CA\FD
    int
    SDF_GenerateRandom ( void *hSessionHandle , unsigned int uiLength ,
            unsigned char *pucRandom );
    /*--------------------------------------------------------------------*/
//\C3\DCÔ¿\B9\DC\C0\ED\C0àº¯\CA\FD
//\B2\FA\C9\FAECC\B7Ç¶Ô³\C6\C3\DCÔ¿\B6Ô²\A2\CA\E4\B3\F6
    int
    SDF_GenerateKeyPair_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            unsigned int uiKeyBits , ECCrefPublicKey *pucPublicKey ,
            ECCrefPrivateKey *pucPrivateKey );
    /*--------------------------------------------------------------------*/
//\B7Ç¶Ô³\C6\CBã·¨\D4\CB\CB\E3\C0àº¯\CA\FD
//\CDâ²¿\C3\DCÔ¿ECCÇ©\C3\FB
    int
    SDF_ExternalSign_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPrivateKey *pucPrivateKey , unsigned char *pucData ,
            unsigned int uiDataLength , ECCSignature *pucSignature );
//\CDâ²¿\C3\DCÔ¿ECC\D1\E9Ö¤
    int
    SDF_ExternalVerify_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPublicKey *pucPublicKey , unsigned char *pucDataInput ,
            unsigned int uiInputLength , ECCSignature *pucSignature );
//\CDâ²¿\C3\DCÔ¿ECC\BC\D3\C3\DC
    int
    SDF_ExternalEncrytp_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPublicKey *pucPublicKey , unsigned char *pucData ,
            unsigned int uiDataLength , ECCCipher *pucEncData );
//\CDâ²¿\C3\DCÔ¿ECC\BD\E2\C3\DC
    int
    SDF_ExternalDecrypt_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPrivateKey *pucPrivateKey , ECCCipher *pucEncData ,
            unsigned char *pucData , unsigned int *puiDataLength );
    /*--------------------------------------------------------------------*/
//\C5\E4\D6\C3\C7\FA\CF\DF\C0\E0\D0Í¼\B0\B2\CE\CA\FD
    int
    IWN_CurveParaCfg ( void *hSessionHandle , enum_bits CurveType ,
            ECCrefPara *CurvePara );
    /*--------------------------------------------------------------------*/
//SHA256\D4Ó´\D5\CBã·¨
    int
    IWN_Sha256 ( void *hSessionHandle , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucHash ,
            unsigned int *puiHashLength );
//HMAC-SHA256ÕªÒª\BC\C6\CB\E3\CBã·¨
    int
    IWN_HmacSha256 ( void *hSessionHandle , unsigned char *pucKey ,
            unsigned int uiKeyLength , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucHash ,
            unsigned int puiHashLength );
//KD-HMAC-SHA256\C3\DCÔ¿\B5\BC\B3\F6\CBã·¨
    int
    IWN_KdHmacSha256 ( void *hSessionHandle , unsigned char *pucKey ,
            unsigned int uiKeyLength , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucHash ,
            unsigned int *puiHashLength );
    /*--------------------------------------------------------------------*/
//ECDH\C3\DCÔ¿\BD\BB\BB\BB\CBã·¨
    int
    IWN_ExternalEcdhKeyAgreement ( void *hSessionHandle ,
            ECCrefPrivateKey *pucPrivateKey , ECCrefPublicKey *pucPublicKey ,
            ECCEcdh *agrKeySeed );
//ECCÊ¹\D3\C3\CDâ²¿Ë½Ô¿\CA\FD\D7\D6Ç©\C3\FB\CBã·¨
    int
    IWN_ExternalSignHash_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPrivateKey *pucExpPrivateKey , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucID ,
            unsigned int uiIDLength , ECCSignature *pucSignature );
//ECCÊ¹\D3\C3\CDâ²¿\B9\ABÔ¿\CA\FD\D7\D6Ç©\C3\FB\D1\E9Ö¤\CBã·¨
    int
    IWN_ExternalVerifyHash_ECC ( void *hSessionHandle , unsigned int uiAlgID ,
            ECCrefPublicKey *pucExpPublicKey , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucID ,
            unsigned int uiIDLength , ECCSignature *pucSignature );
//ECCÊ¹\D3\C3\CDâ²¿\B9\ABÔ¿\BC\D3\C3\DC\CBã·¨
    int
    IWN_ExternalEncrytp_ECC ( void *hSessionHandle ,
            ECCrefPublicKey *pucPublicKey , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucEncData ,
            unsigned int *uiEncDataLength );
//ECCÊ¹\D3\C3\CDâ²¿Ë½Ô¿\BD\E2\C3\DC\CBã·¨
    int
    IWN_ExternalDecrypt_ECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucPrivateKey , unsigned char * pucEncData ,
            unsigned int uiEncDataLength , unsigned char *pucData ,
            unsigned int *puiDataLength );
    /*--------------------------------------------------------------------*/
//SM3\D4Ó´\D5\CBã·¨
    int
    IWN_SM3 ( void *hSessionHandle , unsigned char *pucData ,
            unsigned int uiDataLength , unsigned char *pucHash ,
            unsigned int *puiHashLength );
//HMAC-SM3ÕªÒª\BC\C6\CB\E3\CBã·¨
    int
    IWN_HMAC_SM3 ( void *hSessionHandle , unsigned char* pucData ,
            unsigned int uiDataLength , unsigned char* pucKey ,
            unsigned int uiKeyLength , unsigned char* pucDigest ,
            unsigned int uiDigestLength );
//KD-HMAC-SM3\C3\DCÔ¿\B5\BC\B3\F6\CBã·¨
    int
    IWN_KD_HMAC_SM3 ( void *hSessionHandle , unsigned char* pucData ,
            unsigned int uiDataLength , unsigned char* pucKey ,
            unsigned int uiKeyLength , unsigned char* pucOut ,
            unsigned int uiOutLength );
    /*--------------------------------------------------------------------*/
//SM2\B7\A2\C6\F0\B7\BD\C9\FA\B3\C9\C1\D9Ê±\B9\ABË½Ô¿\CA\FD\BE\DD
    int
    IWN_SM2_GenerateAgreementDataWithECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucSponsorTmpPrivateKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey );
//SM2\B7\A2\C6ð·½¸\F9\BE\DDÐ­\C9Ì²\CE\CA\FD\C9\FA\B3É»á»°\C3\DCÔ¿
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
//SM2\CF\ECÓ¦\B7\BD2\B8\F6\BDÓ¿\DA
//SM2\CF\ECÓ¦\B7\BD\C9\FA\B3\C9\C1\D9Ê±\B9\ABË½Ô¿\CA\FD\BE\DD
    int
    IWN_SM2_GenerateResponseTmpKeyWithECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucResponseTmpPrivateKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey );
//SM2\CF\ECÓ¦\B7\BD\B8\F9\BE\DDÐ­\C9Ì²\CE\CA\FD\C9\FA\B3É»á»°\C3\DCÔ¿
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
//SM2\CF\ECÓ¦\B7\BD1\B8\F6\BDÓ¿\DA
//SM2\CF\ECÓ¦\B7\BD\B8\F9\BE\DDÐ­\C9Ì²\CE\CA\FD\C9\FA\B3É»á»°\C3\DCÔ¿\BA\CD\C1\D9Ê±\B9\ABÔ¿\CA\FD\BE\DD
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
//SM2Í¨\B9\FD\D3Ã»\A7ID\C9\FA\B3\C9\D3Ã»\A7\CA\FD\BE\DD
    int
    IWN_GenerateUserInfo ( void *hSessionHandle ,
            ECCrefPublicKey *pucPublicKey , unsigned char *pucID ,
            unsigned int uiIDLength , unsigned char* UserInfo ,
            unsigned int *uiUserInfoLength );

//SM2\B7\A2\C6\F0\B7\BD\C9\FA\B3\C9\C1\D9Ê±\B9\ABË½Ô¿\CA\FD\BE\DD
    int
    IWN_GenerateAgreementDataWithECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucSponsorTmpPrivateKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey );
//SM2\B7\A2\C6ð·½¸\F9\BE\DDÐ­\C9Ì²\CE\CA\FD\C9\FA\B3É»á»°\C3\DCÔ¿
    int
    IWN_GenerateKeyWithECC ( void *hSessionHandle , unsigned int uiKeyBits ,
            unsigned char *pucZa , unsigned int uiZaLength ,
            unsigned char *pucZb , unsigned int uiZbLength ,
            ECCrefPrivateKey *pucSponsorPrivateKey ,
            ECCrefPrivateKey *pucSponsorTmpPrivateKey ,
            ECCrefPublicKey *pucResponsePublicKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey ,
            ECCrefKeyExch *pucKeyExch );
//SM2\CF\ECÓ¦\B7\BD2\B8\F6\BDÓ¿\DA
//SM2\CF\ECÓ¦\B7\BD\C9\FA\B3\C9\C1\D9Ê±\B9\ABË½Ô¿\CA\FD\BE\DD
    int
    IWN_GenerateResponseTmpKeyWithECC ( void *hSessionHandle ,
            ECCrefPrivateKey *pucResponseTmpPrivateKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey );
//SM2\CF\ECÓ¦\B7\BD\B8\F9\BE\DDÐ­\C9Ì²\CE\CA\FD\C9\FA\B3É»á»°\C3\DCÔ¿
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
//SM2\CF\ECÓ¦\B7\BD1\B8\F6\BDÓ¿\DA
//SM2\CF\ECÓ¦\B7\BD\B8\F9\BE\DDÐ­\C9Ì²\CE\CA\FD\C9\FA\B3É»á»°\C3\DCÔ¿\BA\CD\C1\D9Ê±\B9\ABÔ¿\CA\FD\BE\DD
    int
    IWN_GenerateAgreementDataAndKeyWithECC ( void *hSessionHandle ,
            unsigned int uiKeyBits , unsigned char *pucZa ,
            unsigned int uiZaLength , unsigned char *pucZb ,
            unsigned int uiZbLength , ECCrefPrivateKey *pucResponsePrivateKey ,
            ECCrefPublicKey *pucSponsorPublicKey ,
            ECCrefPublicKey *pucSponsorTmpPublicKey ,
            ECCrefPublicKey *pucResponseTmpPublicKey ,
            ECCrefKeyExch *pucKeyExch );
//ECC F(p)\D3\F2\C7\FA\CF\DF\D3\D0Ð§\D0\D4\D1\E9Ö¤
    int
    IWN_CurveVerify ( enum_bits CurveType , ECCrefPara *CurvePara );

//\B4ò¿ª»á»°\B2\A2\C5\E4\D6\C3\C7\FA\CFß²\CE\CA\FD
    int
    IWN_OpenSession ( void **phSessionHandle , ECCrefPara *pucEccPara );
//\B9Ø±Õ»á»°
    int
    IWN_CloseSession ( void *hSessionHandle );

#ifdef  __cplusplus
}
#endif

#endif /*_ECC_ALG_API_H*/

