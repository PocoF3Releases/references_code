/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#ifndef ANDROID_MIUIBOOSTERMANAGER_H
#define ANDROID_MIUIBOOSTERMANAGER_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class IMiuiBoosterManager : public IInterface
{
public:
    DECLARE_META_INTERFACE(MiuiBoosterManager);
    virtual int removeAuthorizedUid(int uid) = 0;
    virtual int checkPermission(String16& pkg_name, int uid) = 0;
    virtual int checkPermission_key(String16& pkg_name, int uid, String16& auth_key) = 0;
    virtual int requestCpuHighFreq(int uid, int level, int timeoutms) = 0;
    virtual int cancelCpuHighFreq(int uid) = 0;
    virtual int requestGpuHighFreq(int uid, int level, int timeoutms) = 0;
    virtual int cancelGpuHighFreq(int uid) = 0;
    virtual int requestIOHighFreq(int uid, int level, int timeoutms) = 0;
    virtual int cancelIOHighFreq(int uid) = 0;
    virtual int requestVpuHighFreq(int uid, int level, int timeoutms) = 0;
    virtual int cancelVpuHighFreq(int uid) = 0;
    virtual int requestMemory(int uid, int level, int timeoutms) = 0;
    virtual int cancelMemory(int uid) = 0;
    virtual int requestNetwork(int uid, int level, int timeoutms) = 0;
    virtual int cancelNetwork(int uid) = 0;
    virtual int requestThreadPriority(int uid, int req_tid, int timeoutms) = 0;
    virtual int cancelThreadPriority(int uid, int req_tid) = 0;
    virtual int requestIOPrefetch(int uid, String16& req_tid) = 0;
    virtual int requestDdrHighFreq(int uid, int level, int timeoutms) = 0;
    virtual int cancelDdrHighFreq(int uid) = 0;
    virtual int requestBindCore(int uid, int tid) = 0;
    virtual int requestSetScene(int uid, String16& pkgName, int sceneId) = 0;

    enum {
        REMOVE_AUTHORIZED_UID = IBinder::FIRST_CALL_TRANSACTION,
        CHECK_PERMISSION = IBinder::FIRST_CALL_TRANSACTION + 1,
        CHECK_PERMISSION_KEY = IBinder::FIRST_CALL_TRANSACTION + 2,
        REQUEST_CPU_HIGH = IBinder::FIRST_CALL_TRANSACTION + 3,
        CANCEL_CPU_HIGH = IBinder::FIRST_CALL_TRANSACTION + 4,
        REQUEST_GPU_HIGH = IBinder::FIRST_CALL_TRANSACTION + 5,
        CANCEL_GPU_HIGH = IBinder::FIRST_CALL_TRANSACTION + 6,
        REQUEST_IO_HIGH = IBinder::FIRST_CALL_TRANSACTION + 7,
        CANCEL_IO_HIGH = IBinder::FIRST_CALL_TRANSACTION + 8,
        REQUEST_VPU_HIGH = IBinder::FIRST_CALL_TRANSACTION + 9,
        CANCEL_VPU_HIGH = IBinder::FIRST_CALL_TRANSACTION + 10,
        REQUEST_MEMORY = IBinder::FIRST_CALL_TRANSACTION + 11,
        CANCEL_MEMORY = IBinder::FIRST_CALL_TRANSACTION + 12,
        REQUEST_NETWORK = IBinder::FIRST_CALL_TRANSACTION + 13,
        CANCEL_NETWORK = IBinder::FIRST_CALL_TRANSACTION + 14,
        REQUEST_THREAD_PRIORITY = IBinder::FIRST_CALL_TRANSACTION + 15,
        CANCEL_THREAD_PRIORITY = IBinder::FIRST_CALL_TRANSACTION + 16,
        REQUEST_IO_PREFETCH = IBinder::FIRST_CALL_TRANSACTION + 17,
        REQUEST_DDR_HIGH = IBinder::FIRST_CALL_TRANSACTION + 18,
        CANCEL_DDR_HIGH = IBinder::FIRST_CALL_TRANSACTION + 19,
        REQUEST_BIND_CORE = IBinder::FIRST_CALL_TRANSACTION + 20,
        REQUEST_SET_SCENE = IBinder::FIRST_CALL_TRANSACTION + 25,
    };
};

class BnMiuiBoosterManager : public BnInterface<IMiuiBoosterManager>
{
public:
    virtual status_t onTransact(uint32_t code,
                            const Parcel& data,
                            Parcel* reply,
                            uint32_t flags = 0);
};
}; //namespace android
#endif //ANDROID_MiuiBoosterManager_H
