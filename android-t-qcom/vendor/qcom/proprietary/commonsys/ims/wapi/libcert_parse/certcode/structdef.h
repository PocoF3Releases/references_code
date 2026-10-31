//2015/11/17 ÐÞ¸Ä
#ifndef _STRUCTDEF_H
#define _STRUCTDEF_H

#ifdef  __cplusplus
extern "C"
{
#endif

#pragma pack(1)

//´íÎóÂë¶¨Òå
#define CTP_FALSE							0x0
#define CTP_OK								0x1                    
#define ERROR_BASE                          0x11000000              
#define ERROR_ALGPARAMNOEXIST				ERROR_BASE + 0x00000001
#define ERROR_ISSUERNOEXIST					ERROR_BASE + 0x00000002  
#define ERROR_SUBJECTNOEXIST                ERROR_BASE + 0x00000003  
#define ERROR_EXTENSIONSNOEXIST             ERROR_BASE + 0x00000004  
#define ERROR_RESERVENOEXIST				ERROR_BASE + 0x00000005 

#define ERROR_ATTRIBUTENOEXIST              ERROR_BASE + 0x00000006 
#define ERROR_LOCALKEYNOEXIST               ERROR_BASE + 0x00000007
#define ERROR_FRIENDNAMENOEXIST             ERROR_BASE + 0x00000008

    typedef enum
    {
        VERSION = 0,
        SERIALNUMBER,
        SIGNATURE,
        ISSUER,
        VALIDITY,
        SUBJECT,
        SUBJECTPUBLICKEYINFO,
        EXTENSIONS,
        SIGNATUREALGORITHM,
        SIGNATUREVALUE
    } X509_Info;

    typedef enum
    {
        DC, C, O, OU, CN
    } ISS_SUB_Type;
//cert struct
    typedef struct _ASN1_TYPE
    {
        int length;
        unsigned char *data;
    } ASN1_Version , ASN1_CertificateSerialNumber , ASN1_AlgorithmIdentifier ,
            ASN1_Validity , ASN1_SubjectPublicKeyInfo , ASN1_BITS_STRING ,
            ASN1_Name , ASN1_Extensions , ASN1_MacAlg , ASN1_MacValue ,
            ASN1_Salt , ASN1_Iter , ASN1_P7DataID , ASN1_P7EnDataID ,
            ASN1_Rc2Info , ASN1_CertData , ASN1_8ShroudedKeyBagID ,
            ASN1_3DesInfo , ASN1_PrikeyData , AUTH_SAFE_BAG_DATA ,
            P12_MAC_Info_DATA , ASN1_AttrInfo , ASN1_Check_Info , ASN1_Sig_Info;

    typedef struct _ISS_SUB_INFO
    {
        ASN1_Name dc_info;
        ASN1_Name c_info;
        ASN1_Name o_info;
        ASN1_Name ou_info;
        ASN1_Name cn_info;
    } ISS_SUB_Info;

    typedef struct _ASN1_OBJ_INFO
    {
        const char *explain;
        int length;
        unsigned char data [ 20 ];
    } ASN1_Obj_Info;

    typedef struct _X509_CERT
    {
        ASN1_Version Version;
        ASN1_CertificateSerialNumber Serialnumber;
        ASN1_AlgorithmIdentifier Signature;
        ASN1_Name Issuer;
        ASN1_Validity validity;
        ASN1_Name Subject;
        ASN1_SubjectPublicKeyInfo subjectPublicKeyInfo;
        ASN1_Extensions extensions;
        ASN1_AlgorithmIdentifier signatureAlgorithm;
        ASN1_BITS_STRING signatureValue; //20151013 ASN1_BIT_STRING ¸ÄÎªASN1_BITS_STRING
    } X509_Cert;

    typedef struct _CHECK_X509_CERT
    {
        ASN1_Check_Info check_info;
        ASN1_Sig_Info sig_info;
    } CHECK_X509_Cert;

    typedef struct _P12_MAC_Info
    {
        ASN1_MacAlg mac_alg;
        ASN1_MacValue mac_value;
        ASN1_Salt mac_salt;
        ASN1_Iter iterations; //Ä¬ÈÏÖµÎª 1
    } P12_MAC_Info;

    typedef struct _AUTH_SAFE_BAG
    {
        ASN1_P7EnDataID p7endata_id;
        ASN1_P7DataID p7data_id_1;
        ASN1_Rc2Info rc2info;
        ASN1_CertData cert_data;
        ASN1_P7DataID p7data_id_2;
        ASN1_8ShroudedKeyBagID shroudedkeybag_id;
        ASN1_3DesInfo desinfo;
        ASN1_PrikeyData prikey_data;
        ASN1_AttrInfo attrinfo;
    } AUTH_SAFE_BAG;

    typedef struct _P12_CERT
    {
        ASN1_Version version;
        ASN1_P7DataID p7data_id;
        AUTH_SAFE_BAG_DATA safe_bag_data;
        P12_MAC_Info_DATA mac_info_data;
    } P12_Cert;

//privatekey struct
    typedef struct _ASN1_PK_TYPE
    {
        int length;
        unsigned char *data;
    } ASN1_PK_VERSION , ASN1_PK_PKINFO , ASN1_PK_ALGORITHMID ,
            ASN1_PK_PUBKEYINFO , ASN1_PK_RESERVE , ASN1_PK_KEYINFO ,
            ASN1_PK_ALGORITHMINFO;
    typedef struct _X509_PRIVATEKEY
    {
        ASN1_PK_VERSION Version;
        ASN1_PK_PKINFO PrivateKey;
        ASN1_PK_ALGORITHMID AlgorithmID;
        ASN1_PK_PUBKEYINFO PublicKey;
        ASN1_PK_RESERVE Reserve;
    } X509_PRIVATEKEY;

    typedef struct _P8_PRIVATEKEY
    {
        ASN1_PK_VERSION Version;
        ASN1_PK_ALGORITHMINFO AlgorithmInfo;
        ASN1_PK_KEYINFO KeyInfo;
    } P8_PRIVATEKEY;

    typedef struct _P12_PRIVATEKEY
    {
        ASN1_PK_VERSION Version;
        ASN1_PK_KEYINFO KeyInfo;
        ASN1_PK_ALGORITHMINFO AlgorithmInfo;
        ASN1_PK_PUBKEYINFO PublicKey;
    } P12_PRIVATEKEY;

#pragma pack()

    int
    GetInfoLength ( const unsigned char *p , unsigned char *pos );

    int
    UnPack_X509 ( unsigned char *In , int len , X509_Cert *ctx );
    int
    UnPack_CHECK_X509 ( unsigned char *In , int len ,
            CHECK_X509_Cert *check_ctx );
    int
    UnPack_P12 ( unsigned char *In , int len , unsigned char *version ,
            int *version_len , unsigned char *p7data_id , int *p7data_id_len ,
            unsigned char *safe_bag , int *safe_bag_len ,
            unsigned char *mac_info , int *mac_info_len , P12_Cert *p12_ctx );
    int
    UnPack_AUTH_SAFE_BAG ( unsigned char *In , int len ,
            AUTH_SAFE_BAG *auth_safe_bag );
    void
    End_X509 ( X509_Cert *ctx );
    void
    End_CHECK_X509 ( CHECK_X509_Cert *check_ctx );
    void
    End_P12 ( P12_Cert *p12_ctx );
    void
    End_AUTH_SAFE_BAG ( AUTH_SAFE_BAG *auth_safe_bag );
    int
    GetVersion_X509 ( X509_Cert *ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetSerialnumber_X509 ( X509_Cert *ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetSignature_algorithm_X509 ( X509_Cert *ctx , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetSignature_parameters_X509 ( X509_Cert *ctx , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetBeforeTime_X509 ( X509_Cert *ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetAfterTime_X509 ( X509_Cert *ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetPubkey_algorithm_X509 ( X509_Cert *ctx , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetPubkey_parameters_X509 ( X509_Cert *ctx , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetPubkey_Der_Info_X509 ( X509_Cert *ctx , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetPubkey_Info_X509 ( X509_Cert *ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetsignatureAlgorithm_algorithm_X509 ( X509_Cert *ctx , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetsignatureAlgorithm_parameters_X509 ( X509_Cert *ctx ,
            unsigned char *out , int *outlen , unsigned char *tag );
    int
    GetsignatureValue_X509 ( X509_Cert *ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );

    int
    Get_Sets_Count_X509 ( X509_Cert *ctx , X509_Info type , int *count );
    int
    GetName_Sets_TypeAndValue_X509 ( X509_Cert *ctx , X509_Info type ,
            const int num , unsigned char *out_type , int *outlen_type ,
            unsigned char *tag_type , unsigned char *out_value ,
            int *outlen_value , unsigned char *tag_value );
    int
    GetExtension_Sets_TypeAndValue_X509 ( X509_Cert *ctx , const int num ,
            unsigned char *out_extnID , int *outlen_extnID ,
            unsigned char *tag_extnID , unsigned char *out_extnValue ,
            int *outlen_extnValue , unsigned char *tag_extnValue );
    int
    ChangeIdentifyToString_X509 ( unsigned char *in , int len , int *out ,
            int *outlen );

    int
    Get_X509_Check_Info ( CHECK_X509_Cert *check_ctx , unsigned char *out ,
            int *outlen );
    int
    Get_X509_Check_Sig_Info ( CHECK_X509_Cert *check_ctx , unsigned char *out ,
            int *outlen );
    /*----------------------------------------------------------------------------------------------*/
    int
    GetVersion_P12 ( P12_Cert *p12_ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetP7DataID_P12 ( P12_Cert *p12_ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetP7EnDataID_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetP7DataID_1_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetRc2Alg_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetRc2Salt_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetRc2Iter_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetCertData_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetP7DataID_2_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    Get8ShroudedKeyBagID_P12 ( AUTH_SAFE_BAG *auth_safe_bag ,
            unsigned char *out , int *outlen , unsigned char *tag );
    int
    Get3DesAlg_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    Get3DesSalt_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    Get3DesIter_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetPrikeyData_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    Get_Attr_Count_P12 ( AUTH_SAFE_BAG *auth_safe_bag , int *count );
    int
    GetName_Attr_TypeAndValue_P12 ( AUTH_SAFE_BAG *auth_safe_bag ,
            const int num , unsigned char *out_type , int *outlen_type ,
            unsigned char *tag_type , unsigned char *out_value ,
            int *outlen_value , unsigned char *tag_value );
    int
    GetLocalKeyID_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetLocalKeyData_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetFriendNameID_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetFriendNameData_P12 ( AUTH_SAFE_BAG *auth_safe_bag , unsigned char *out ,
            int *outlen , unsigned char *tag );
    int
    GetMacAlg_P12 ( P12_Cert *p12_ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetMacValue_P12 ( P12_Cert *p12_ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetMacSalt_P12 ( P12_Cert *p12_ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );
    int
    GetMacIter_P12 ( P12_Cert *p12_ctx , unsigned char *out , int *outlen ,
            unsigned char *tag );

    int
    Unpack_P8_PrivateKey ( unsigned char *In , int len ,
            P8_PRIVATEKEY *p8_prikey );
    void
    End_P8_PrivateKey ( P8_PRIVATEKEY *p8_prikey );
    int
    Get_P8_PK_Version ( P8_PRIVATEKEY *p8_prikey , unsigned char *out ,
            int *outlen );
    int
    Get_P8_PK_AlgorithmID ( P8_PRIVATEKEY *p8_prikey , unsigned char *out ,
            int *outlen );
    int
    Get_P8_PK_AlgorithmPARAM ( P8_PRIVATEKEY *p8_prikey , unsigned char *out ,
            int *outlen );
    int
    Get_P8_PK_Key ( P8_PRIVATEKEY *p8_prikey , unsigned char *out ,
            int *outlen );

    int
    P8toP12_PriKey ( unsigned char *p8_data , int p8_len ,
            unsigned char *p12_prikey , int *prikey_len );
    int
    Cert_BagInfo ( unsigned char *data , int len , unsigned char *pUserCert ,
            int *pUserCertlen , unsigned char *pCACert , int *pCACertlen );

    int
    Unpack_P12_PrivateKey ( unsigned char *In , int len ,
            P12_PRIVATEKEY *p12_prikey );
    void
    End_P12_PrivateKey ( P12_PRIVATEKEY *p12_prikey );
    int
    Get_P12_PK_Version ( P12_PRIVATEKEY *p12_prikey , unsigned char *out ,
            int *outlen );
    int
    Get_P12_PK_PriKey ( P12_PRIVATEKEY *p12_prikey , unsigned char *out ,
            int *outlen );
    int
    Get_P12_PK_AlgorithmPARAM ( P12_PRIVATEKEY *p12_prikey ,
            unsigned char *out , int *outlen );
    int
    Get_P12_PK_PubKey ( P12_PRIVATEKEY *p12_prikey , unsigned char *out ,
            int *outlen );
    int
    Get_P12_PK_Der_PubKey ( P12_PRIVATEKEY *p12_prikey , unsigned char *out ,
            int *outlen );

    int
    P12toP8_PriKey ( unsigned char *p12_data , int p12_len ,
            unsigned char *p8_prikey , int *prikey_len );

    int
    IWN_ParseP12ToX509 ( const char *pP12FileName , const char *password ,
            unsigned char *pUserCert , unsigned short *pUserCertlen ,
            unsigned char *pPrivateKey , unsigned short *pKeylen ,
            unsigned char *pCACert , unsigned short *pCACertlen );

    int
    IWN_ParseX509ToP12 ( const unsigned char *pUserCert ,
            const unsigned short UserCertlen ,
            const unsigned char *pPrivateKey , const unsigned short Keylen ,
            const unsigned char *pCACert , const unsigned short CACertlen ,
            const char *pP12FileName , const char *password ,
            char *friendly_name , int cert_iter_num , int mac_iter_num );

    int
    IWN_ParseP12ToX509_Mod ( const unsigned char *cert_buf , int cert_len ,
            const char *password , unsigned char *pUserCert ,
            unsigned short *pUserCertlen , unsigned char *pPrivateKey ,
            unsigned short *pKeylen , unsigned char *pCACert ,
            unsigned short *pCACertlen );

    /*--------------------------------------------------------------------------------------------*/
    int
    Unpack_X509_PrivateKey ( unsigned char *In , int len ,
            X509_PRIVATEKEY *prikey_ctx );
    void
    End_X509_PrivateKey ( X509_PRIVATEKEY *prikey_ctx );
    int
    Get_X509_PK_Version ( X509_PRIVATEKEY *prikey_ctx , unsigned char *out ,
            int *outlen );
    int
    Get_X509_PK_PrivateKey ( X509_PRIVATEKEY *prikey_ctx , unsigned char *out ,
            int *outlen );
    int
    Get_X509_PK_AlgorithmID ( X509_PRIVATEKEY *prikey_ctx , unsigned char *out ,
            int *outlen );
    int
    Get_X509_PK_PublicKey ( X509_PRIVATEKEY *prikey_ctx , unsigned char *out ,
            int *outlen );
    int
    Get_X509_PK_Reserve ( X509_PRIVATEKEY *prikey_ctx , unsigned char *out ,
            int *outlen );

    int
    GetX509Info_ASN1 ( X509_Cert *ctx , X509_Info type , unsigned char *out ,
            int *outlen );

    int
    cert_search ( const char *psourcestr , const char *psubstr , int offest );
    void
    str_filter ( char *p , size_t len );

    int
    EncodeBase64 ( unsigned char *btSrc , int iSrcLen , unsigned char *btRet ,
            int *piRetLen );
    int
    DecodeBase64 ( unsigned char *btSrc , int iSrcLen , unsigned char *btRet ,
            int *piRetLen );
    /*-------------------------------------------------------------------------------------------*/
    int
    Write_Cert_Key_Pem ( const char *file_name , unsigned char *cert ,
            int cert_len , unsigned char *key , int key_len );
    int
    Total_Name_Info_X509 ( char *data_dc , char *data_c , char *data_o ,
            char *data_ou , char *data_cn , unsigned char *out_data ,
            int *out_len );
    int
    Oid_To_Asn1 ( char *oid_data , int buf , char *data );
    int
    Creat_Exten_Obj ( ASN1_Obj_Info *obj_data , const char *oid_data ,
            const char *exp_data );
    int
    Exten_Info_X509 ( ASN1_Obj_Info *obj_data , char *data ,
            unsigned char *ex_data , int *len );
    int
    Total_Exten_Info_X509 ( unsigned char *in_data , int in_len ,
            unsigned char *out_data , int *out_len );
    int
    Time_Info_X509 ( char *begin , char *end , unsigned char *out_data ,
            int *out_len );
    /*----------------------------------------------------------------------------------------------*/
    int
    IWN_Create_X509 ( unsigned int v , unsigned int ser_num ,
            unsigned char *key , int key_len , unsigned char *iss ,
            int iss_len , unsigned char *sub , int sub_len ,
            unsigned char *ext , int ext_len , unsigned char *time ,
            int time_len , int mode , unsigned char *ca_pri_key ,
            unsigned int ca_ser_num , unsigned char *out , int *out_len );
    int
    IWN_Create_PK_X509 ( unsigned char *key , int key_len , int mode ,
            unsigned char *out , int *outlen );
/*----------------------------------------------------------------------------------------------*/

#ifdef  __cplusplus
}
#endif

#endif /*_STRUCTDEF_H*/

