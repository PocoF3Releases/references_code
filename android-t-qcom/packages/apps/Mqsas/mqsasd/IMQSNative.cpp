/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#include <inttypes.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/PermissionCache.h>
#include <utils/String16.h>
#include "IMQSNative.h"

namespace android {


const android::String16
IMQSNative::descriptor("miui.mqsas.IMQSNative");

const android::String16&
IMQSNative::getInterfaceDescriptor() const {
    return IMQSNative::descriptor;
}

static const char* SUPPORT_CP_PARAMS = "-rf,/sys/bootinfo/powerup_reason,/sdcard/MIUI/debug_log/powerinfo/ -rf,/sys/bootinfo/poweroff_reason,/sdcard/MIUI/debug_log/powerinfo/";
static const char* SUPPORT_CP_PART_PARAMS = "/cache/mqsas/hang/";
static const char* SUPPORT_MKDIR_PARAMS = "/cache/mqsas/ /cache/mqsas/hang/";
static const char* SUPPORT_RM_PARAMS = "-rf,/data/mqsas/mi_ic.log";
static const char* SUPPORT_BIN = "MI_IC";

#define LOGE(...)    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

status_t BnMQSNative::onTransact(uint32_t code, const Parcel& data, Parcel* reply,
        uint32_t flags) {

    const IPCThreadState* ipc = IPCThreadState::self();
    const int calling_uid = ipc->getCallingUid();
    if(calling_uid != 1000) {
       LOGE("calling_uid == %d", calling_uid);
       return NO_ERROR;
    }
    switch(code) {

        //virtual void captureLog(String16 type, String16 headline,Vector<String16>actions, Vector<String16>params, bool offline, int32_t id, bool upload,
        //                                                   /*ICaptureCallback callback,*/ String16 where, Vector<String16> includedFiles, bool isRecordInLocal)=0;
        case CAPTURE_LOG: {
             CHECK_INTERFACE(IMQSNative, data, reply);
             const String16 type = data.readString16();
             const String16 headline = data.readString16();
             const int32_t actionsSize = data.readInt32();
             if (actionsSize <= 0) {
                 return BAD_VALUE;
             }
             Vector<String16> actions;
             for (int32_t i = 0; i < actionsSize; i++) {
                 const String16 action = data.readString16();
                 actions.push(action);
             }
             const int32_t paramSize = data.readInt32();
             if (paramSize <= 0) {
                 return BAD_VALUE;
             }
             Vector<String16> params;
             for (int32_t i = 0; i < paramSize; i++) {
                 const String16 param = data.readString16();
                 params.push(param);
             }
             const bool offline = data.readInt32()==1 ? true : false;
             const int32_t id = data.readInt32();
             const bool upload = data.readInt32()==1 ? true : false;
             const String16 where = data.readString16();
             const int32_t includedFileSize = data.readInt32();
             Vector<String16> includedFiles;
             if (includedFileSize != -1) {
                     for (int32_t i = 0; i < includedFileSize; i++) {
                     const String16 includeFile = data.readString16();
                     includedFiles.push(includeFile);
                 }
             }
             const bool isRecordInLocal = data.readInt32()==1 ? true : false;
             captureLog(type, headline,actions, params,  offline,  id, upload, /*ICaptureCallback callback,*/  where, includedFiles, isRecordInLocal);
             reply->writeNoException();
             return NO_ERROR;
        };
        case ATTACH_SOCK_FILTER: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            String16 srcMac = data.readString16();
            String16 dstMac = data.readString16();
            uint32_t srcIp = data.readInt32();
            uint32_t dstIp = data.readInt32();

            uint32_t proto = data.readInt32();
            uint32_t srcPort = data.readInt32();
            uint32_t dstPort = data.readInt32();

            uint32_t identifyDataLen = data.readInt32();
            uint8_t *identifyData = NULL;
            if (identifyDataLen > 0) {
                identifyData = (uint8_t *)data.readInplace(identifyDataLen);
            }
            uint32_t retDataLen = data.readInt32();

            String16 iface = data.readString16();
            int sock = attachSockFilter(
                String8(srcMac).string(), String8(dstMac).string(), srcIp, dstIp, proto, srcPort, dstPort,
                identifyData, identifyDataLen, retDataLen, String8(iface).string());
            reply->writeNoException();
            if (sock < 0) {
                reply->writeInt32(0);
            } else {
                reply->writeInt32(1);
                reply->writeInt32(0); // java ParcelFileDescriptor CommFd
                reply->writeDupFileDescriptor(sock);
                close(sock);
            }
            return NO_ERROR;
        }
        case SET_CORE_FILTER:{
             CHECK_INTERFACE(IMQSNative, data, reply);
             const int32_t pid = data.readInt32();
             const bool isFull = data.readInt32()==1 ? true : false;
             setCoreFilter(pid, isFull);
             reply->writeNoException();
             return NO_ERROR;
        }
        case RUN_COMMAND:{
             CHECK_INTERFACE(IMQSNative, data, reply);
             const String16 action16 = data.readString16();
             const String16 params16 = data.readString16();
             const String8 action8 = String8(action16);
             const String8 params8 = String8(params16);
             const int32_t  timeout = data.readInt32();
             if(0 == strcmp(action8.string(),"kill")) {
                  if(strstr(params8.string(), "-3") == 0) {
                       return NO_ERROR;
                  }
             } else if (0 == strcmp(action8.string(),"cp")) {
                  if(strstr(SUPPORT_CP_PARAMS, params8.string()) == 0 && strstr(params8.string(), SUPPORT_CP_PART_PARAMS) == 0) {
                       return NO_ERROR;
                  }
             } else if (0 == strcmp(action8.string(),"mkdir")) {
                  if(strstr(SUPPORT_MKDIR_PARAMS, params8.string()) == 0) {
                       return NO_ERROR;
                  }
             } else if (0 == strcmp(action8.string(),"rm")) {
                  if(strstr(SUPPORT_RM_PARAMS, params8.string()) == 0) {
                       return NO_ERROR;
                  }
             } else {
                  if(strstr(SUPPORT_BIN, action8.string()) == 0) {
                       return NO_ERROR;
                  }
             }
             int result = runCommand(action16, params16, timeout);
             reply->writeInt32(result);
             reply->writeNoException();
             return NO_ERROR;
        }
        case GET_FREEFRAG_STATS: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            String8 fragInfo;
            getFreeFragStats(fragInfo);
            reply->writeNoException();
            reply->writeString16((String16)fragInfo);
            return NO_ERROR;
        }
        case CREATE_FILE_IN_PERSIST: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            String16 path = data.readString16();
            int result = createFileInPersist(String8(path).string());
            reply->writeInt32(result);
            reply->writeNoException();
            return NO_ERROR;
        }
        case WRITE_TO_PERSIST_FILE: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            String16 path = data.readString16();
            String16 buff = data.readString16();
            writeToPersistFile(String8(path).string(),String8(buff).string());
            reply->writeNoException();
            return NO_ERROR;
        }
        case READ_FILE: {
             CHECK_INTERFACE(IMQSNative, data, reply);
             String16 path = data.readString16();
             String8 content;
             readFile(String8(path).string(),content);
             reply->writeNoException();
             reply->writeString16((String16)content);
             return NO_ERROR;
        }
        case GET_FILES_FROM_PERSIST: {
             CHECK_INTERFACE(IMQSNative, data, reply);
             String8 files;
             getFilesFromPersist(files);
             reply->writeNoException();
             reply->writeString16((String16)files);
             return NO_ERROR;
        }
        case DEFRAG_DATA_PARTITION: {
             CHECK_INTERFACE(IMQSNative, data, reply);
             defragDataPartition();
             reply->writeNoException();
             return NO_ERROR;
        }
        case STOP_DEFRAG: {
             CHECK_INTERFACE(IMQSNative, data, reply);
             stopDefrag();
             reply->writeNoException();
             return NO_ERROR;
        }
        case GET_PSI_SOCK_FD: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            int sock = getPSISocketFd();
            reply->writeNoException();
            if (sock < 0) {
                reply->writeInt32(0);
            } else {
                reply->writeInt32(1);
                reply->writeInt32(0);
                reply->writeDupFileDescriptor(sock);
                close(sock);
            }
            return NO_ERROR;
        }
        case ADD_PSI_TRIGGER: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            int32_t callbackKey = data.readInt32();
            String16 node = data.readString16();
            String16 command = data.readString16();
            addPSITriggerCommand(callbackKey, String8(node).string(), String8(command).string());
            reply->writeNoException();
            return NO_ERROR;
        }
        case FLASH_DEBUGPOLICY: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            int32_t type = data.readInt32();
            flashDebugPolicy(type);
            reply->writeNoException();
            return NO_ERROR;
        }
        case CONNECT_PSI_MONITOR: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            String16 clientKey = data.readString16();
            int sock = connectPSIMonitor(String8(clientKey).string());
            reply->writeNoException();
            if (sock < 0) {
                reply->writeInt32(0);
            } else {
                reply->writeInt32(1);
                reply->writeInt32(0);
                reply->writeDupFileDescriptor(sock);
                close(sock);
            }
            return NO_ERROR;
        }
        case REGISTER_TCP_STATUS_LISTENER: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            status_t _aidl_ret_status = OK;
            int32_t in_uid;
            sp<ITcpStatusListener> in_listener;
            _aidl_ret_status = data.readInt32(&in_uid);
            if (((_aidl_ret_status) != (::android::OK))) {
              return _aidl_ret_status;
            }
            _aidl_ret_status = data.readStrongBinder(&in_listener);
            if (((_aidl_ret_status) != (::android::OK))) {
              return _aidl_ret_status;
            }
            registerTcpStatusListener(in_uid, in_listener);
            reply->writeNoException();
            return NO_ERROR;
        }
        break;
        case UNREGISTER_TCP_STATUS_LISTENER: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            status_t _aidl_ret_status = OK;
            int32_t in_uid;
            sp<ITcpStatusListener> in_listener;
            _aidl_ret_status = data.readInt32(&in_uid);
            if (((_aidl_ret_status) != (::android::OK))) {
              return _aidl_ret_status;
            }
            _aidl_ret_status = data.readStrongBinder(&in_listener);
            if (((_aidl_ret_status) != (::android::OK))) {
              return _aidl_ret_status;
            }
            unregisterTcpStatusListener(in_uid, in_listener);
            reply->writeNoException();
            return NO_ERROR;
        }
        break;
        case GET_WIFI_RSSI: {
            CHECK_INTERFACE(IMQSNative, data, reply);
            String16 iface = data.readString16();
            String16 bssid_mac = data.readString16();
            int rssi = getWifiRssi(String8(iface).string(), String8(bssid_mac).string());
            if(rssi < 0){
                reply->writeNoException();
                reply->writeInt32(rssi);
            }
        return NO_ERROR;
        }
        break;
        case REGISTER_CNSS_STATISTIC_LISTENER:{
            CHECK_INTERFACE(IMQSNative, data, reply);
            status_t _aidl_ret_status = OK;
            sp<ICnssStatisticListener> in_listener;
            _aidl_ret_status = data.readStrongBinder(&in_listener);
            if (((_aidl_ret_status) != (::android::OK))) {
              return _aidl_ret_status;
            }
            registerCnssStatisticListener(in_listener);
            reply->writeNoException();
            return NO_ERROR;
        }
        break;
        case UNREGISTER_CNSS_STATISTIC_LISTENER:{
            CHECK_INTERFACE(IMQSNative, data, reply);
             status_t _aidl_ret_status = OK;
             sp<ICnssStatisticListener> in_listener;
             _aidl_ret_status = data.readStrongBinder(&in_listener);
            if (((_aidl_ret_status) != (::android::OK))) {
              return _aidl_ret_status;
            }
            unregisterCnssStatisticListener(in_listener);
            reply->writeNoException();
            return NO_ERROR;
        }
        break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
};

bool BnMQSNative::checkPermission(const String16& permission) {
    const IPCThreadState* ipc = IPCThreadState::self();
    const int calling_pid = ipc->getCallingPid();
    const int calling_uid = ipc->getCallingUid();
    return PermissionCache::checkPermission(permission, calling_pid, calling_uid);
}


}; // namespace android
