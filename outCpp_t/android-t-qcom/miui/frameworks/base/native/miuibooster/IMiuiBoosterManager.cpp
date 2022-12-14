/*
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 */

#include "IMiuiBoosterManager.h"

namespace android {

class BpMiuiBoosterManager : public BpInterface<IMiuiBoosterManager> {
public:
    BpMiuiBoosterManager(const sp<IBinder>& binder)
        : BpInterface<IMiuiBoosterManager>(binder)
    {
    }

    virtual int removeAuthorizedUid (int uid){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        remote()->transact(REMOVE_AUTHORIZED_UID, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int checkPermission (String16& pkg_name, int uid){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeString16(pkg_name);
        data.writeInt32(uid);
        remote()->transact(CHECK_PERMISSION, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int checkPermission_key (String16& pkg_name, int uid, String16& auth_key){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeString16(pkg_name);
        data.writeInt32(uid);
        data.writeString16(auth_key);
        remote()->transact(CHECK_PERMISSION_KEY, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int requestCpuHighFreq (int uid, int level, int timeoutms){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(level);
        data.writeInt32(timeoutms);
        remote()->transact(REQUEST_CPU_HIGH, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int cancelCpuHighFreq(int uid){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         remote()->transact(CANCEL_CPU_HIGH, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestGpuHighFreq (int uid, int level, int timeoutms){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(level);
        data.writeInt32(timeoutms);
        remote()->transact(REQUEST_GPU_HIGH, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int cancelGpuHighFreq(int uid){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         remote()->transact(CANCEL_GPU_HIGH, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestIOHighFreq (int uid, int level, int timeoutms){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(level);
        data.writeInt32(timeoutms);
        remote()->transact(REQUEST_IO_HIGH, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int cancelIOHighFreq(int uid){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         remote()->transact(CANCEL_IO_HIGH, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestDdrHighFreq (int uid, int level, int timeoutms){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(level);
        data.writeInt32(timeoutms);
        remote()->transact(REQUEST_DDR_HIGH, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int requestBindCore (int uid, int tid){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(tid);
        remote()->transact(REQUEST_BIND_CORE, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int cancelDdrHighFreq(int uid){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         remote()->transact(CANCEL_DDR_HIGH, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestVpuHighFreq (int uid, int level, int timeoutms){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(level);
        data.writeInt32(timeoutms);
        remote()->transact(REQUEST_VPU_HIGH, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int cancelVpuHighFreq(int uid){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         remote()->transact(CANCEL_VPU_HIGH, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestMemory (int uid, int level, int timeoutms){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(level);
        data.writeInt32(timeoutms);
        remote()->transact(REQUEST_MEMORY, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int cancelMemory(int uid){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         remote()->transact(CANCEL_MEMORY, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestNetwork (int uid, int level, int timeoutms){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(level);
        data.writeInt32(timeoutms);
        remote()->transact(REQUEST_NETWORK, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int cancelNetwork(int uid){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         remote()->transact(CANCEL_NETWORK, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestThreadPriority (int uid, int req_tid, int timeoutms){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeInt32(req_tid);
        data.writeInt32(timeoutms);
        remote()->transact(REQUEST_THREAD_PRIORITY, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }

    virtual int cancelThreadPriority(int uid, int req_tid){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         data.writeInt32(req_tid);
         remote()->transact(CANCEL_THREAD_PRIORITY, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestIOPrefetch(int uid, String16& filePath){
         Parcel data, reply;
         data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
         data.writeInt32(uid);
         data.writeString16(filePath);
         remote()->transact(REQUEST_IO_PREFETCH, data, &reply);
         reply.readExceptionCode();
         return reply.readInt32();
    }

    virtual int requestSetScene(int uid, String16& pkgName, int sceneId){
        Parcel data, reply;
        data.writeInterfaceToken(IMiuiBoosterManager::getInterfaceDescriptor());
        data.writeInt32(uid);
        data.writeString16(pkgName);
        data.writeInt32(sceneId);
        remote()->transact(REQUEST_SET_SCENE, data, &reply);
        reply.readExceptionCode();
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(MiuiBoosterManager, "com.miui.performance.IMiuiBoosterManager");

status_t BnMiuiBoosterManager::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{

    switch (code) {
    case REMOVE_AUTHORIZED_UID: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int ret = removeAuthorizedUid(uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CHECK_PERMISSION: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        String16 pkg_name = data.readString16();
        int uid = data.readInt32();
        int ret = checkPermission(pkg_name, uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CHECK_PERMISSION_KEY: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        String16 pkg_name = data.readString16();
        int uid = data.readInt32();
        String16 auth_key = data.readString16();
        int ret = checkPermission_key(pkg_name, uid, auth_key);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_CPU_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int level = data.readInt32();
        int timeoutms = data.readInt32();
        int ret = requestCpuHighFreq(uid, level, timeoutms);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CANCEL_CPU_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int ret = cancelCpuHighFreq(uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_GPU_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int level = data.readInt32();
        int timeoutms = data.readInt32();
        int ret = requestGpuHighFreq(uid, level, timeoutms);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CANCEL_GPU_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int ret = cancelGpuHighFreq(uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_IO_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int level = data.readInt32();
        int timeoutms = data.readInt32();
        int ret = requestIOHighFreq(uid, level, timeoutms);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CANCEL_IO_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int ret = cancelIOHighFreq(uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_DDR_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int level = data.readInt32();
        int timeoutms = data.readInt32();
        int ret = requestDdrHighFreq(uid, level, timeoutms);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CANCEL_DDR_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int ret = cancelDdrHighFreq(uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_BIND_CORE: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int tid = data.readInt32();
        int ret = requestBindCore(uid, tid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_VPU_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int level = data.readInt32();
        int timeoutms = data.readInt32();
        int ret = requestVpuHighFreq(uid, level, timeoutms);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CANCEL_VPU_HIGH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int ret = cancelVpuHighFreq(uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_MEMORY: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int level = data.readInt32();
        int timeoutms = data.readInt32();
        int ret = requestMemory(uid, level, timeoutms);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CANCEL_MEMORY: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int ret = cancelMemory(uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_NETWORK: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int level = data.readInt32();
        int timeoutms = data.readInt32();
        int ret = requestNetwork(uid, level, timeoutms);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CANCEL_NETWORK: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int ret = cancelNetwork(uid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_THREAD_PRIORITY: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int req_tid = data.readInt32();
        int timeoutms = data.readInt32();
        int ret = requestThreadPriority(uid, req_tid, timeoutms);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case CANCEL_THREAD_PRIORITY: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        int req_tid = data.readInt32();
        int ret = cancelThreadPriority(uid, req_tid);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_IO_PREFETCH: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        String16 filePath = data.readString16();
        int ret = requestIOPrefetch(uid, filePath);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    case REQUEST_SET_SCENE: {
        CHECK_INTERFACE(IMiuiBoosterManager, data, reply);
        int uid = data.readInt32();
        String16 pkgName = data.readString16();
        int sceneId = data.readInt32();
        int ret = requestSetScene(uid, pkgName, sceneId);
        reply->writeNoException();
        reply->writeInt32(ret);
        return NO_ERROR;
    } break;

    default:
      return BBinder::onTransact(code, data, reply, flags);
    }
}
}; //namespace android
