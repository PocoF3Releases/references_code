/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef MQS_NATIVE_DEAMON_H_
#define MQS_NATIVE_DEAMON_H_

#include "IMQSNative.h"
#include "dump.h"

namespace android {

class MQSNativeDeamon : public BnMQSNative {
    public:
        static MQSNativeDeamon* getInstance() {
            if (sInstance == NULL) {
                sInstance = new MQSNativeDeamon();
            }
            return sInstance;
        }

        virtual void captureLog(String16 type, String16 headline,Vector<String16>actions, Vector<String16>params, bool offline, int32_t id, bool upload,
                                           /*ICaptureCallback callback,*/ String16 where, Vector<String16> includedFiles, bool isRecordInLocal);
        virtual int attachSockFilter(const char* srcMac, const char* dstMac, uint32_t srcIp, uint32_t dstIp, uint32_t proto, uint32_t srcPort,
                                           uint32_t dstPort, uint8_t *identifyData, uint32_t identifyDataLen, uint32_t retDataLen, const char* iface);

        virtual void setCoreFilter(int pid, bool isFull);

        virtual int runCommand(String16 action, String16 params, int timeout=60);

        virtual void getFreeFragStats(String8&);

        virtual int createFileInPersist(const char* path);

        virtual void writeToPersistFile(const char* path, const char *buff);

        virtual void readFile(const char* path,String8&);

        virtual void getFilesFromPersist(String8&);

        virtual void defragDataPartition();

        virtual void stopDefrag();

        virtual int getPSISocketFd();

        virtual void addPSITriggerCommand(int callbackKey, const char* node, const char* command);

        virtual void flashDebugPolicy(int type);

        virtual int connectPSIMonitor(const char* clientKey);

        virtual void registerTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener);

        virtual void unregisterTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener);

        virtual int getWifiRssi(const char* interface, const char* bssid_mac);

        virtual void registerCnssStatisticListener(sp<ICnssStatisticListener>& listener);

        virtual void unregisterCnssStatisticListener(sp<ICnssStatisticListener>& listener);

    private:
        MQSNativeDeamon();
        virtual ~MQSNativeDeamon();

        static MQSNativeDeamon* sInstance;
};

} // namespace android

#endif // MQS_NATIVE_DEAMON_H_
