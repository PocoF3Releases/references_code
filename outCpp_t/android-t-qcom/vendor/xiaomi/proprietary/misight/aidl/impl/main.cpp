/*
 * Copyright (c) 2022 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include "MiSight.h"
#include <log_mgr/LogMgr.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::vendor::xiaomi::hardware::misight::MiSight;
using aidl::vendor::xiaomi::hardware::misight::LogMgr;

int main() {
    LOG(INFO) << "Xiaomi MiSight Vendor Service is starting...";
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    std::shared_ptr<MiSight> misight = ndk::SharedRefBase::make<MiSight>();
    std::shared_ptr<LogMgr> logmgr = ndk::SharedRefBase::make<LogMgr>();

    std::string instance = std::string(MiSight::descriptor) + "/default";
    binder_status_t status = AServiceManager_addService(misight->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);
    LOG(DEBUG) << "Registered MiSight...";

    instance = std::string(LogMgr::descriptor) + "/default";
    status = AServiceManager_addService(logmgr->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK);
    LOG(DEBUG) << "Registered LogMgr...";

    LOG(INFO) << "Xiaomi MiSight Vendor Service started!";

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
