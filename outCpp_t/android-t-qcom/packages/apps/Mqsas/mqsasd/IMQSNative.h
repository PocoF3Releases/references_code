/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef IMQS_NATIVE_H_
#define IMQS_NATIVE_H_

#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/String8.h>
#include <string.h>
#include <cutils/log.h>
#include "ITcpStatusListener.h"
#include "wificond/netlink_utils.h"
#include "CnssStatistic/ICnssStatisticListener.h"
#include "CnssStatistic/CnssStatistic.h"

namespace android {


/*
* Abstract base class for native implementation of IMQSNative.
*
* Note: This must be kept manually in sync with IMQSNative.aidl
*/
class IMQSNative : public IInterface {
    public:
        enum {
           CAPTURE_LOG = IBinder::FIRST_CALL_TRANSACTION + 0,
           ATTACH_SOCK_FILTER = IBinder::FIRST_CALL_TRANSACTION + 1,
           SET_CORE_FILTER = IBinder::FIRST_CALL_TRANSACTION + 2,
           RUN_COMMAND = IBinder::FIRST_CALL_TRANSACTION + 3,
           GET_FREEFRAG_STATS = IBinder::FIRST_CALL_TRANSACTION + 4,
           CREATE_FILE_IN_PERSIST = IBinder::FIRST_CALL_TRANSACTION + 5,
           WRITE_TO_PERSIST_FILE = IBinder::FIRST_CALL_TRANSACTION + 6,
           READ_FILE = IBinder::FIRST_CALL_TRANSACTION + 7,
           GET_FILES_FROM_PERSIST = IBinder::FIRST_CALL_TRANSACTION + 8,
           DEFRAG_DATA_PARTITION = IBinder::FIRST_CALL_TRANSACTION + 9,
           STOP_DEFRAG = IBinder::FIRST_CALL_TRANSACTION + 10,
           GET_PSI_SOCK_FD = IBinder::FIRST_CALL_TRANSACTION + 11,
           ADD_PSI_TRIGGER = IBinder::FIRST_CALL_TRANSACTION + 12,
           FLASH_DEBUGPOLICY = IBinder::FIRST_CALL_TRANSACTION + 13,
           CONNECT_PSI_MONITOR = IBinder::FIRST_CALL_TRANSACTION + 14,
           REGISTER_TCP_STATUS_LISTENER = IBinder::FIRST_CALL_TRANSACTION + 15,
           UNREGISTER_TCP_STATUS_LISTENER = IBinder::FIRST_CALL_TRANSACTION + 16,
           GET_WIFI_RSSI = IBinder::FIRST_CALL_TRANSACTION + 17,
           REGISTER_CNSS_STATISTIC_LISTENER = IBinder::FIRST_CALL_TRANSACTION + 18,
           UNREGISTER_CNSS_STATISTIC_LISTENER = IBinder::FIRST_CALL_TRANSACTION + 19,
        };

        IMQSNative() { }
        virtual ~IMQSNative() { }
        virtual const android::String16& getInterfaceDescriptor() const;

        // Binder interface methods

        virtual void captureLog(String16 type, String16 headline,Vector<String16>actions, Vector<String16>params, bool offline, int32_t id, bool upload,
                                                   /*ICaptureCallback callback,*/ String16 where, Vector<String16> includedFiles, bool isRecordInLocal)=0;

        virtual int attachSockFilter(const char* srcMac, const char* dstMac, uint32_t srcIp, uint32_t dstIp, uint32_t proto, uint32_t srcPort,
                                                   uint32_t dstPort, uint8_t *identifyData, uint32_t identifyDataLen, uint32_t retDataLen, const char* iface) = 0;

        virtual void setCoreFilter(int pid, bool isFull) = 0;
        
        virtual int runCommand(String16 action, String16 params, int timeout) = 0;

        virtual void getFreeFragStats(String8& fragInfo) = 0;

        // DECLARE_META_INTERFACE - C++ client interface not needed
        static const android::String16 descriptor;

        virtual int createFileInPersist(const char* path) = 0;

        virtual void writeToPersistFile(const char* path, const char *buff) = 0;

        virtual void readFile(const char* path,String8& content) = 0;

        virtual void getFilesFromPersist(String8& files) = 0;

        virtual void defragDataPartition() = 0;

        virtual void stopDefrag() = 0;

        virtual int getPSISocketFd() = 0;

        virtual void addPSITriggerCommand(int callbackKey, const char* node, const char* command) = 0;

        virtual void flashDebugPolicy(int type) = 0;

        virtual int connectPSIMonitor(const char* clientKey) = 0;

        virtual void registerTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener) = 0;

        virtual void unregisterTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener) = 0;

        virtual int getWifiRssi(const char *interface, const char* bssid_mac) = 0;

        virtual void registerCnssStatisticListener(sp<ICnssStatisticListener>& listener) = 0;

        virtual void unregisterCnssStatisticListener(sp<ICnssStatisticListener>& listener) = 0;
};

// ----------------------------------------------------------------------------

class BnMQSNative: public BnInterface<IMQSNative> {
    public:
       virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
               uint32_t flags = 0);
    private:
       bool checkPermission(const String16& permission);
};

} // namespace android

#endif // IMQS_NATIVE_H_

