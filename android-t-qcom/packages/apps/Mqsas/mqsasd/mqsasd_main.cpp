/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#define LOG_TAG "mqsasd"

#include <cutils/log.h>
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/PermissionCache.h>
#include <utils/String16.h>
#include <binder/Parcel.h>
#include <utils/RefBase.h>
#include "MQSNativeDeamon.h"
#include "BootMonitorThread.h"
#include "PSIMonitor.h"
#include "CaptureServerThread.h"
#include "TombstoneMonitorThread.h"
#include "ProcReporterDelegate.h"
#include "CnssStatistic.h"
#include "RescueSpaceThread.h"

using namespace android;

int main() {
    ALOGI("Starting " LOG_TAG);
#ifndef DISABLE_BOOT_MONITOR
    android::sp<BootMonitorThread> bootMonitorThread = new BootMonitorThread;
    bootMonitorThread->run("BootMonitorThread");
#endif
#ifndef DISABLE_CAPTURE_SERVER
    android::sp<CaptureServerThread> captureServerThread = new CaptureServerThread;
    captureServerThread->run("CaptureServerThread");
#endif
#ifndef DISABLE_TOMBSTONE_MONITOR
    android::sp<TombstoneMonitorThread> tombstoneMonitorThread = new TombstoneMonitorThread;
    tombstoneMonitorThread->run("TombstoneMonitorThread");
#endif
#ifndef DISABLE_RESCUESPACE_MONITOR
    android::sp<RescueSpaceThread> rescueSpaceThread = new RescueSpaceThread;
    rescueSpaceThread->run("RescueSpaceThread");
#endif
#ifndef DISABLE_PROCREPORTER
    android::sp<ProcReporterDelegate> prDelegate = ProcReporterDelegate::getInstance();
#endif
    android::sp<android::IServiceManager> serviceManager = android::defaultServiceManager();
    android::sp<android::MQSNativeDeamon> deamon =
            android::MQSNativeDeamon::getInstance();
    android::status_t ret = serviceManager->addService(
            android::MQSNativeDeamon::descriptor, deamon);
    if (ret != android::OK) {
        ALOGE("Couldn't register " LOG_TAG " binder service!");
        return -1;
    }

    // PSI monitor
    PSIMonitor::initPSIMonitor();
    CnssStatistic::initMonitor();

    /*
     * start thread pool with 4 threads for run command
     */
    ProcessState::self()->setThreadPoolMaxThreadCount(4);
    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();
    android::IPCThreadState::self()->joinThreadPool();

    return 0;
}

