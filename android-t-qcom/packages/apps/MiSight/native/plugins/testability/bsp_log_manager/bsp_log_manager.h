/*
 * Copyright (C) Xiaomi Inc.
 * Description: bsp log manager head file
 *
 */

#ifndef BSP_LOG_MANAGER_H
#define BSP_LOG_MANAGER_H
#include "plugin.h"
#include "bsp_log_config.h"
#include "json_util.h"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <aidl/vendor/xiaomi/hardware/misight/ILogMgr.h>
#include <aidl/vendor/xiaomi/hardware/misight/ILogMgrCallback.h>
#include <aidl/vendor/xiaomi/hardware/misight/BnLogMgrCallback.h>

namespace android {
namespace MiSight {

using aidl::vendor::xiaomi::hardware::misight::ILogMgr;
using aidl::vendor::xiaomi::hardware::misight::ILogMgrCallback;
class BSPLogManager : public Plugin, public FileEventHandler, public EventListener {
public:
    BSPLogManager() : loaded_(false) {}
    ~BSPLogManager() {}
    bool CanProcessEvent(sp<Event> event) override;
    EVENT_RET OnEvent(sp<Event>& event) override;
    bool PreToLoad();
    void OnLoad() override;
    void OnUnload() override;
    void DelayInit();
    void WatchAllLogFolder();
    void WatchLogFolder(sp<BSPLogConfig>& config);
    std::vector<sp<BSPLogConfig>> GetConfigFromJson(std::string& cfgFilePath);
    sp<BSPLogConfig> GetConfigByFolderName(std::string& folderName);
    std::vector<sp<BSPLogConfig>> ConvertConfigObjToVec(Json::Value& obj);
    bool CheckAndClean(std::string& folderPath);
    void CleanAll();
    bool IsLoad() {
        return loaded_;
    }
    void SendEventToDft(Json::Value& root);
    void SendEventToDft(const std::string& dirFullPath, size_t dirSizeBefore, size_t dirSizeAfter);

    void OnUnorderedEvent(sp<Event> event);
    void MonitorSdcardWatchTask();
    std::string GetListenerName() {
        return GetName();
    }

    int32_t AddFileWatch(std::string watchPath, uint32_t mask);
    int32_t OnFileEvent(int fd, int Type) override;
    int32_t GetPollFd() override;
    int32_t GetPollEventType() override;

private:
    std::shared_ptr<ILogMgr> getVendorAidlService();
    bool loaded_;

    const std::string BSP_LOG_PATH = "/data/miuilog/bsp";
    const std::string BSP_VENDOR_LOG_PATH = "/data/vendor/bsplog";
    const std::string BSP_LOG_CONFIG_NAME = "bsp_log_config.json";
    const std::string BSP_LOG_LISTENER_TAG = "bspLogListener";

    std::vector<sp<BSPLogConfig>> configVec;
    bool isDebugMode = false;
    static constexpr uint32_t AID_ROOT = 0;
    static constexpr uint32_t AID_SYSTEM = 1000;
    bool sdcardReady = false;

    int inotifyFd_;
    int inotifyVendorFd_;
    std::map<std::string, int> fileMap_;
    std::shared_ptr<ILogMgr> vendorLogMgrService;

    void SendEventToDft(std::string& dirFullPath, size_t dirSizeBefore, size_t dirSizeAfter);

};
}
}
#endif
