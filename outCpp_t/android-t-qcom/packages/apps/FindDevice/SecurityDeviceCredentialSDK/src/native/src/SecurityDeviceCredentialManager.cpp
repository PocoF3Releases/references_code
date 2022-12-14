#include <SecurityDeviceCredentialManager.h>

#include "../common/log.h"

#include <utils/Mutex.h>
#include <utils/String8.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#define WAIT_SERVICE_READY_INTERVAL_MILLISEC 500
#define RETRY_ON_HARDWARE_SERVICE_NOT_AVAILABLE_INTERVAL_MILLISEC 500

#define SECURITY_EXCEPTION_CODE (-1)

enum _KEY_TYPE {
    KEY_TYPE_FINANCIAL = 0,
    KEY_TYPE_ORDINARY = 1,
};

static const android::String16 SECURITY_DEVICE_CREDEINTAIL_MANAGER_INTERFACE_DESCRIPTOR
    ("com.xiaomi.security.devicecredential.ISecurityDeviceCredentialManager");

static __thread android::String8 * tl_pStringBuf = NULL;
static const char * getThreadLocalStringFromString16(const android::String16 & str16) {
    delete tl_pStringBuf;
    tl_pStringBuf = NULL;

    tl_pStringBuf = new android::String8(str16);
    return *tl_pStringBuf;
}

static __thread unsigned char * tl_pBuf = NULL;
static unsigned char * getThreadLocalBuffer(size_t size) {
    delete [] tl_pBuf;
    tl_pBuf = NULL;

    tl_pBuf = new unsigned char [size];
    return tl_pBuf;
}

enum {
    TR_CODE_isThisDeviceSupported = android::IBinder::FIRST_CALL_TRANSACTION,
    TR_CODE_getSecurityDeviceId = android::IBinder::FIRST_CALL_TRANSACTION + 1,
    TR_CODE_sign = android::IBinder::FIRST_CALL_TRANSACTION + 2,
    TR_CODE_forceReload = android::IBinder::FIRST_CALL_TRANSACTION + 3,
};

static const android::sp<android::IBinder> & getSecurityDeviceCredentialManagerService() {
    static android::sp<android::IBinder> sService(NULL);
    static android::Mutex sLock;
    android::Mutex::Autolock autolock(sLock);

    bool isBinderAlive = false;
    if (sService != NULL) {
        LOGI("getSecurityDeviceCredentialManagerService: sService != NULL. ");
        isBinderAlive = sService->pingBinder() == android::NO_ERROR;
    } else {
        LOGI("getSecurityDeviceCredentialManagerService: sService == NULL. ");
    }

    if (!isBinderAlive) {
       LOGW("getSecurityDeviceCredentialManagerService: binder not alive. ");
       android::sp<android::IServiceManager> sm = android::defaultServiceManager();
       do {
          sService = sm->getService(android::String16("miui.sedc"));
          if (sService != NULL) { break; }
          LOGE("Failed to get SecurityDeviceCredentialManagerService binder. ");
          usleep(WAIT_SERVICE_READY_INTERVAL_MILLISEC * 1000);
       } while (true);
    } else {
        LOGI("getSecurityDeviceCredentialManagerService: binder alive. ");
    }

    return sService;
}

int SecurityDeviceCredentialManager::isThisDeviceSupported(bool * pRst) {

#ifndef __SEDC_SERVICE_AVAILABLE
    return ERROR_CODE_NOT_SUPPORTED;
#endif
    android::sp<android::IBinder> service = getSecurityDeviceCredentialManagerService();

    android::Parcel data, reply;

    data.writeInterfaceToken(SECURITY_DEVICE_CREDEINTAIL_MANAGER_INTERFACE_DESCRIPTOR);

    android::status_t trRst = service->transact(TR_CODE_isThisDeviceSupported, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("SecurityDeviceCredentialManager::isThisDeviceSupported: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        return ERROR_CODE_REMOTE;
    }

    int32_t expCode = reply.readExceptionCode();
    if (expCode) {
        LOGE("SecurityDeviceCredentialManager::isThisDeviceSupported: remote exception caught: %d",
            (int)expCode);
        if (expCode == SECURITY_EXCEPTION_CODE) {
            return ERROR_CODE_SECURITY;
        }
        return ERROR_CODE_REMOTE;
    }

    *pRst = reply.readInt32() != 0;

    return ERROR_CODE_OK;
}

int SecurityDeviceCredentialManager::getSecurityDeviceId(const char ** pOut) {

    *pOut = NULL;

#ifndef __SEDC_SERVICE_AVAILABLE
    return ERROR_CODE_NOT_SUPPORTED;
#endif
    android::sp<android::IBinder> service = getSecurityDeviceCredentialManagerService();

    // TODO: Too much work to factor out this logic, especially in native code?
    while (true) {
        android::Parcel data, reply;
        data.writeInterfaceToken(SECURITY_DEVICE_CREDEINTAIL_MANAGER_INTERFACE_DESCRIPTOR);

        android::status_t trRst = service->transact(TR_CODE_getSecurityDeviceId, data, &reply);
        if (trRst != android::NO_ERROR) {
            LOGE("SecurityDeviceCredentialManager::getSecurityDeviceId: Transaction failed with code: %lu. ",
                (unsigned long)trRst);
            return ERROR_CODE_REMOTE;
        }

        int32_t expCode = reply.readExceptionCode();
        if (expCode) {
            LOGE("SecurityDeviceCredentialManager::getSecurityDeviceId: remote exception caught: %d",
                (int)expCode);
            if (expCode == SECURITY_EXCEPTION_CODE) {
                return ERROR_CODE_SECURITY;
            }
            return ERROR_CODE_REMOTE;
        }

        int resultCode = reply.readInt32();
        if (resultCode != ERROR_CODE_TRUST_ZONE_SERVICE_NOT_AVALIABLE) {
            if (resultCode == 0) {
                *pOut = getThreadLocalStringFromString16(reply.readString16());
            }
            return resultCode;
        }

        LOGE("SecurityDeviceCredentialManager::getSecurityDeviceId: Hardware service not ready, retry...");
        usleep(RETRY_ON_HARDWARE_SERVICE_NOT_AVAILABLE_INTERVAL_MILLISEC * 1000);
    }
}


int SecurityDeviceCredentialManager::sign(int keyType, const unsigned char * signData, int signDataLen,
    const unsigned char ** pSignature, int * pSignatureLen, bool reloadIfNecessary) {

    *pSignature = NULL;
    *pSignatureLen = 0;


#ifndef __SEDC_SERVICE_AVAILABLE
    return ERROR_CODE_NOT_SUPPORTED;
#endif
    android::sp<android::IBinder> service = getSecurityDeviceCredentialManagerService();

    // TODO: Too much work to factor out this logic, especially in native code?
    while (true) {
        android::Parcel data, reply;
        data.writeInterfaceToken(SECURITY_DEVICE_CREDEINTAIL_MANAGER_INTERFACE_DESCRIPTOR);

        if (signData == NULL) { signDataLen = -1; }

        data.writeInt32(keyType);
        data.writeInt32(signDataLen);
        if (signDataLen >= 0) {
            data.write(signData, signDataLen);
        }
        data.writeInt32(reloadIfNecessary ? 1 : 0);

        android::status_t trRst = service->transact(TR_CODE_sign, data, &reply);
        if (trRst != android::NO_ERROR) {
            LOGE("SecurityDeviceCredentialManager::sign: Transaction failed with code: %lu. ",
                (unsigned long)trRst);
            return ERROR_CODE_REMOTE;
        }

        int32_t expCode = reply.readExceptionCode();
        if (expCode) {
            LOGE("SecurityDeviceCredentialManager::sign: remote exception caught: %d",
                (int)expCode);
            if (expCode == SECURITY_EXCEPTION_CODE) {
                return ERROR_CODE_SECURITY;
            }
            return ERROR_CODE_REMOTE;
        }

        int resultCode = reply.readInt32();
        if (resultCode != ERROR_CODE_TRUST_ZONE_SERVICE_NOT_AVALIABLE) {
            if (resultCode == 0) {
                int len = reply.readInt32();
                if (len > 0) {
                    unsigned char * buf = getThreadLocalBuffer(len);
                    reply.read(buf, len);
                    *pSignature = buf;
                    *pSignatureLen = len;
                }
            }
            return resultCode;
        }

        LOGE("SecurityDeviceCredentialManager::sign: Hardware service not ready, retry...");
        usleep(RETRY_ON_HARDWARE_SERVICE_NOT_AVAILABLE_INTERVAL_MILLISEC * 1000);
    }
}

int SecurityDeviceCredentialManager::forceReload() {
    #ifndef __SEDC_SERVICE_AVAILABLE
        return ERROR_CODE_NOT_SUPPORTED;
    #endif
    android::sp<android::IBinder> service = getSecurityDeviceCredentialManagerService();

    // TODO: Too much work to factor out this logic, especially in native code?
    while (true) {
        android::Parcel data, reply;
        data.writeInterfaceToken(SECURITY_DEVICE_CREDEINTAIL_MANAGER_INTERFACE_DESCRIPTOR);

        android::status_t trRst = service->transact(TR_CODE_forceReload, data, &reply);
        if (trRst != android::NO_ERROR) {
            LOGE("SecurityDeviceCredentialManager::forceReload: Transaction failed with code: %lu. ",
                (unsigned long)trRst);
            return ERROR_CODE_REMOTE;
        }

        int32_t expCode = reply.readExceptionCode();
        if (expCode) {
            LOGE("SecurityDeviceCredentialManager::forceReload: remote exception caught: %d",
                (int)expCode);
            if (expCode == SECURITY_EXCEPTION_CODE) {
                return ERROR_CODE_SECURITY;
            }
            return ERROR_CODE_REMOTE;
        }

        int resultCode = reply.readInt32();
        if (resultCode != ERROR_CODE_TRUST_ZONE_SERVICE_NOT_AVALIABLE) {
            return resultCode;
        }

        LOGE("SecurityDeviceCredentialManager::forceReload: Hardware service not ready, retry...");
        usleep(RETRY_ON_HARDWARE_SERVICE_NOT_AVAILABLE_INTERVAL_MILLISEC * 1000);
    }
}

int SecurityDeviceCredentialManager::signWithDeviceCredential(const unsigned char * signData, int signDataLen,
    const unsigned char ** pSignature, int * pSignatureLen) {
    return sign(KEY_TYPE_ORDINARY, signData, signDataLen, pSignature, pSignatureLen, false);
}

int SecurityDeviceCredentialManager::signWithDeviceCredential(const unsigned char * signData, int signDataLen,
    const unsigned char ** pSignature, int * pSignatureLen, bool reloadIfNecessary) {
    return sign(KEY_TYPE_ORDINARY, signData, signDataLen, pSignature, pSignatureLen, reloadIfNecessary);
}

int SecurityDeviceCredentialManager::signWithFinancialCredential(const unsigned char * signData, int signDataLen,
    const unsigned char ** pSignature, int * pSignatureLen) {
    return sign(KEY_TYPE_FINANCIAL, signData, signDataLen, pSignature, pSignatureLen, false);
}

int SecurityDeviceCredentialManager::signWithFinancialCredential(const unsigned char * signData, int signDataLen,
    const unsigned char ** pSignature, int * pSignatureLen, bool reloadIfNecessary) {
    return sign(KEY_TYPE_FINANCIAL, signData, signDataLen, pSignature, pSignatureLen, reloadIfNecessary);
}