/*
 * Copyright (c) 2022 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#pragma once

#include <aidl/vendor/xiaomi/hardware/misight/BnLogMgr.h>
#include <aidl/vendor/xiaomi/hardware/misight/ILogMgrCallback.h>

#include <memory>
#include <map>

#include <android/binder_auto_utils.h>
#include <log_mgr/bsp_log_config.h>
#include <utils/JsonUtil.h>
#include <pthread.h>

namespace aidl::vendor::xiaomi::hardware::misight {
using namespace android;

static int epollFd_;
static pthread_t fileMonitorThread;

class LogMgrLinkedCallback;

class LogMgr : public BnLogMgr {
  public:
    LogMgr();
    ::ndk::ScopedAStatus initNewINotify(int32_t* return_inotifyFd) override;
    ::ndk::ScopedAStatus addFolderToWatch(int32_t in_inotifyFd, const std::string& in_folderPath, int32_t in_mask, int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus triggerCleanProcess(const std::string& in_folderPath, bool* _aidl_return) override;
    ::ndk::ScopedAStatus updateGlobalConfig(const std::string& in_jsonStr, bool* _aidl_return) override;
    ::ndk::ScopedAStatus registerCallBack(const std::shared_ptr<::aidl::vendor::xiaomi::hardware::misight::ILogMgrCallback>& in_cb) override;
    ::ndk::ScopedAStatus unregisterCallBack(const std::shared_ptr<::aidl::vendor::xiaomi::hardware::misight::ILogMgrCallback>& in_cb) override;
    void OnFileEvent(int fd, int events);

  private:
    friend LogMgrLinkedCallback;  // for exposing death_recipient_

    static void* EPollEventTriggerThread(void *user);
    int addToEpoll(int fd);
    bool CheckAndClean(const std::string& folderPath);
    sp<BSPLogConfig> GetConfigByFolderName(const std::string& folderName);
    std::vector<sp<BSPLogConfig>> ConvertConfigObjToVec(Json::Value& obj);
    void SendEventToDft(const std::string dirFullPath, int64_t dirSizeBefore, int64_t dirSizeAfter);
    std::map<std::string, int> fileMap_;
    std::mutex callbacks_lock_;
    bool loaded_;
    std::vector<sp<BSPLogConfig>> configVec;
    std::vector<std::unique_ptr<LogMgrLinkedCallback>> callbacks_;
    ndk::ScopedAIBinder_DeathRecipient death_recipient_;
    
    const std::string BSP_VENDOR_LOG_PATH = "/data/vendor/bsplog";
    static constexpr uint32_t AID_ROOT = 0;
    static constexpr uint32_t AID_SYSTEM = 1000;
    static const int MAX_FDS = 8;
};
}  // namespace aidl::vendor::xiaomi::hardware::misight
