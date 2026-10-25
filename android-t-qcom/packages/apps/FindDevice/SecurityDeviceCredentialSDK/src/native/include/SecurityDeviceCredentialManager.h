#ifndef SECURITY_DEVICE_CREDENTIAL_MANAGER
#define SECURITY_DEVICE_CREDENTIAL_MANAGER



class SecurityDeviceCredentialManager {
public:
    enum {
        ERROR_CODE_OK = 0,
        ERROR_CODE_REMOTE = -10,
        ERROR_CODE_SECURITY = -11,
        ERROR_CODE_UNKNOWN = -1,

        ERROR_CODE_NOT_SUPPORTED = -100,
        ERROR_CODE_TRUST_ZONE_SERVICE_NOT_AVALIABLE = -101,
        ERROR_CODE_KEY_TYPE_NOT_SUPPORTED = -102,
        ERROR_CODE_EMPTY_DATA = -103,
        ERROR_CODE_SIGN_FAIL = -104,
        ERROR_CODE_RELOAD_FAILURE_NETWORK = -105,
        ERROR_CODE_RELOAD_FAILURE_NO_AVAILABLE_KEY_ON_SERVER = -106,
        ERROR_CODE_RELOAD_FAILURE_INTERNAL = -107,
        ERROR_CODE_FORCE_RELOAD_REFUSED = -108,
    };

public:
    static int isThisDeviceSupported(bool * pRst);
    static int getSecurityDeviceId(const char ** pOut);
    static int signWithDeviceCredential(const unsigned char * signData, int signDataLen, const unsigned char ** pSignature, int * pSignatureLen);
    static int signWithDeviceCredential(const unsigned char * signData, int signDataLen, const unsigned char ** pSignature, int * pSignatureLen, bool reloadIfNecessary);
    static int signWithFinancialCredential(const unsigned char * signData, int signDataLen, const unsigned char ** pSignature, int * pSignatureLen);
    static int signWithFinancialCredential(const unsigned char * signData, int signDataLen, const unsigned char ** pSignature, int * pSignatureLen, bool reloadIfNecessary);
    static int sign(int keyType, const unsigned char * signData, int signDataLen,
                        const unsigned char ** pSignature, int * pSignatureLen, bool reloadIfNecessary);
    static int forceReload();
};

#endif
