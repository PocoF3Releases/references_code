/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#define LOG_TAG "mqsasd"

#include <binder/IServiceManager.h>
#include <utils/Log.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/klog.h>
#include <time.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <android/log.h>
#include <cutils/properties.h>
#include <private/android_filesystem_config.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <string.h>

#include "dump.h"
#include "CmdTool.h"
#include "MQSNativeDeamon.h"
#include "CnssStatistic/CnssStatistic.h"

#ifdef SUPPORT_CAPTURE_NETWORK_PACKET
#include "NetworkCapture.h"
#endif

#ifdef SUPPORT_FREE_FRAG
#include "FreeFrag.h"
#endif

#ifdef SUPPORT_TCP_MONITOR
#include "TcpSocketMonitor.h"
#endif

#ifdef SUPPORT_WIFI_RSSI
#include "wificond/netlink_utils.h"
#endif

#ifdef SUPPORT_PSI_MONITOR
#include "PSIMonitor.h"
#endif

namespace android {

MQSNativeDeamon* MQSNativeDeamon::sInstance = NULL;


MQSNativeDeamon::MQSNativeDeamon() {
}

MQSNativeDeamon::~MQSNativeDeamon() {

}

void MQSNativeDeamon::captureLog(String16 type,
                                String16 headline,
                                Vector<String16> actions,
                                Vector<String16> params,
                                bool offline,
                                int32_t id,
                                bool upload,
                                String16 where,
                                Vector<String16> includedFiles,
                                bool isRecordInLocal) {
    CaptureLogUtil dump;
    dump.dump_all(type, headline, actions, params, offline, id, upload, where, includedFiles, isRecordInLocal);
}

void MQSNativeDeamon::getFilesFromPersist(String8& files){
     CaptureLogUtil dump;
     dump.get_files_from_persist(files);
}

void MQSNativeDeamon::readFile(const char* path,String8& content){
     CaptureLogUtil dump;
     dump.read_file(path,content);
}

int MQSNativeDeamon::createFileInPersist(const char* path){
    CaptureLogUtil dump;
    return dump.create_file_in_persist(path);
}

void MQSNativeDeamon::writeToPersistFile(const char* path, const char *buff){
    CaptureLogUtil dump;
    dump.write_to_persist_file(path,buff);
}

void MQSNativeDeamon::flashDebugPolicy(int32_t type) {
    CaptureLogUtil dump;
    dump.flash_debugpolicy(type);
}

void MQSNativeDeamon::setCoreFilter(int pid, bool isFull) {
    char path[PATH_MAX];
    snprintf(path,sizeof(path),"/proc/%d/coredump_filter",pid);
    int fd = open(path, O_WRONLY);
    if (fd > 0) {
        if (isFull) {
            write(fd, "39", 2);    /*0x27 MMF_DUMP_ANON_PRIVATE|MMF_DUMP_ANON_SHARED|MMF_DUMP_MAPPED_PRIVATE|MMF_DUMP_HUGETLB_PRIVATE*/
        } else {
            write(fd, "35", 2);    /*0x23 MMF_DUMP_ANON_PRIVATE|MMF_DUMP_ANON_SHARED|MMF_DUMP_HUGETLB_PRIVATE*/
        }
    }
    close(fd);
}

int MQSNativeDeamon::runCommand(String16 action, String16 params, int timeout){
     CmdTool cmdtool;
     int result = cmdtool.run_command_lite(action,params,timeout);
     return result;
}

int MQSNativeDeamon::attachSockFilter(__unused const char* srcMac, __unused const char* dstMac, __unused uint32_t srcIp,
                                      __unused uint32_t dstIp, __unused uint32_t proto, __unused uint32_t srcPort,
                                      __unused uint32_t dstPort, __unused uint8_t* identifyData,
                                      __unused uint32_t identifyDataLen, __unused uint32_t retDataLen, __unused const char* iface) {
#ifdef SUPPORT_CAPTURE_NETWORK_PACKET
    SocketFilter filter(srcMac, dstMac, srcIp, dstIp, proto, srcPort, dstPort, identifyData, identifyDataLen, retDataLen, iface);
    return filter.attach();
#else
    return -1;
#endif
}

void MQSNativeDeamon::getFreeFragStats(String8& fragInfo) {
#ifdef SUPPORT_FREE_FRAG
    FreeFrag::getStats(fragInfo);
#endif
}

void MQSNativeDeamon::defragDataPartition() {
#ifdef SUPPORT_FREE_FRAG
    FreeFrag::defragDataPartition();
#endif
}

void MQSNativeDeamon::stopDefrag() {
#ifdef SUPPORT_FREE_FRAG
    FreeFrag::stopDefrag();
#endif
}

int MQSNativeDeamon::getPSISocketFd() {
#ifdef SUPPORT_PSI_MONITOR
    return PSIMonitor::getPSISocketFd();
#else
    return -1;
#endif
}

void MQSNativeDeamon::addPSITriggerCommand(int callbackKey, const char* node, const char* command) {
#ifdef SUPPORT_PSI_MONITOR
    PSIMonitor::addPSITriggerCommand(callbackKey, node, command);
#endif
}

int MQSNativeDeamon::connectPSIMonitor(const char* clientKey) {
#ifdef SUPPORT_PSI_MONITOR
    return PSIMonitor::connectPSIMonitor(clientKey);
#else
    return -1;
#endif
}

void MQSNativeDeamon::registerTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener) {
#ifdef SUPPORT_TCP_MONITOR
    ALOGD(" tcpinfo registerTcpStatusListener");

    TcpSocketMonitor * mTcpSocketMonitor = TcpSocketMonitor::instance();
    mTcpSocketMonitor->registerTcpStatusListener(uid, listener);
#endif
}

void MQSNativeDeamon::unregisterTcpStatusListener(int32_t uid, sp<ITcpStatusListener>& listener) {
#ifdef SUPPORT_TCP_MONITOR
    ALOGD(" tcpinfo unregisterTcpStatusListener");

    TcpSocketMonitor * mTcpSocketMonitor = TcpSocketMonitor::instance();
    mTcpSocketMonitor->unregisterTcpStatusListener(uid, listener);
#endif
}

int MQSNativeDeamon:: getWifiRssi(const char *interface, const char* bssid_mac) {
#ifdef SUPPORT_WIFI_RSSI
    wificond::NetlinkUtils* mNetlinkUtilsHandler = wificond::NetlinkUtils::instance();
    int mCurrentRssi = mNetlinkUtilsHandler->GetStationInfo(interface, bssid_mac);
    return mCurrentRssi;
#else
    return 0;
#endif
}

void MQSNativeDeamon::registerCnssStatisticListener(sp<ICnssStatisticListener>& listener){
    CnssStatistic::registerCnssStatusListener(listener);
    return;
}

void MQSNativeDeamon::unregisterCnssStatisticListener(sp<ICnssStatisticListener>& listener){
    CnssStatistic::unregisterCnssStatusListener(listener);
    return;
}

}
