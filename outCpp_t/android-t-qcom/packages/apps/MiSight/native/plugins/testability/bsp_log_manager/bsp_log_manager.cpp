/*
 * Copyright (C) Xiaomi Inc.
 * Description: bsp log manager implement file
 *
 */

#include "bsp_log_manager.h"
#include "bsp_log_manager_utils.h"
#include "bsp_log_config.h"

#include "file_util.h"
#include "time_util.h"
#include "plugin.h"
#include "plugin_factory.h"
#include "json_util.h"
#include "zip_pack.h"
#include "public_util.h"
#include "event_util.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <iostream>
#include <fstream>
#include <regex>

namespace android {
namespace MiSight {
REGISTER_PLUGIN(BSPLogManager);

using aidl::vendor::xiaomi::hardware::misight::ILogMgr;
using aidl::vendor::xiaomi::hardware::misight::ILogMgrCallback;

class LogMgrCallback : public aidl::vendor::xiaomi::hardware::misight::BnLogMgrCallback {
public:
    LogMgrCallback(android::MiSight::BSPLogManager* bspLogMgr) : mBSPLogMgr(bspLogMgr) {};

    ::ndk::ScopedAStatus onCleanProcessDone(const std::string& dfxJsonStr, int64_t ignoredDirSize) override {
        Json::Value root;

        if (!JsonUtil::ConvertStrToJson(dfxJsonStr, root) || ignoredDirSize > 0) {
            MISIGHT_LOGV("Convert dfxJsonStr to json obj failed, treat it as standard dirFullPath");
            mBSPLogMgr->SendEventToDft(dfxJsonStr, -1, ignoredDirSize);
            return ::ndk::ScopedAStatus::ok();
        }

        mBSPLogMgr->SendEventToDft(root);
        return ::ndk::ScopedAStatus::ok();
    }

private:
    android::MiSight::BSPLogManager* mBSPLogMgr;
};

bool BSPLogManager::CanProcessEvent(sp<Event> event)
{
    return false;
}

std::vector<sp<BSPLogConfig>> BSPLogManager::GetConfigFromJson(std::string& cfgFilePath)
{
    Json::Value root;

    MISIGHT_LOGD("BSPLogManager ReadConfig %s\n", cfgFilePath.c_str());

    std::ifstream in(cfgFilePath.c_str(), std::ios::binary);
    if (!in.is_open()) {
        MISIGHT_LOGE("open config failed!!");
        return configVec;
    }
    std::string jsonStr((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    if (getVendorAidlService()) {
        bool _isSuccess;
        vendorLogMgrService->updateGlobalConfig(jsonStr, &_isSuccess);
        MISIGHT_LOGI("Updated Config for Vendor Aidl, result: %s", _isSuccess ? "success" : "failed");
    }

    if (!JsonUtil::ConvertStrToJson(jsonStr, root)) {
        MISIGHT_LOGE("parse config json error!!");
        return configVec;
    }

    return ConvertConfigObjToVec(root);
}

std::vector<sp<BSPLogConfig>> BSPLogManager::ConvertConfigObjToVec(Json::Value& obj) {
    std::vector<sp<BSPLogConfig>> configVec;

    for (int i = 0; i < obj.size(); i++) {
        Json::Value &current = obj[i];
        Json::Value &childrenObj = current["children"];
        std::vector<sp<BSPLogConfig>> childrenVec = ConvertConfigObjToVec(childrenObj);

        if (
            (!current["folderName"].isNull() && !current["folderName"].isString())
            || (!current["folderPath"].isNull() && !current["folderPath"].isString())
            || (!current["compress"].isNull() && !current["compress"].isBool())
            || (!current["delayCompress"].isNull() && !current["delayCompress"].isBool())
            || (!current["pushEventOnly"].isNull() && !current["pushEventOnly"].isBool())
            || (!current["watchSSRRamdumpHack"].isNull() && !current["watchSSRRamdumpHack"].isBool())
            || (!current["inVendor"].isNull() && !current["inVendor"].isBool())
            || (!current["maxSize"].isNull() && !current["maxSize"].isNumeric())
            || (!current["maxValidDay"].isNull() && !current["maxValidDay"].isNumeric())
            || (!current["maxLogNumber"].isNull() && !current["maxLogNumber"].isNumeric())
        ) {
            std::string folderName = current["folderName"].isString() ? current["folderName"].asString() : "INVALID";
            MISIGHT_LOGE("Invalid Config found!!Please check the config json!! folderName: %s will be skip", folderName.c_str());
            continue;
        }

        sp<BSPLogConfig> config = new BSPLogConfig(
            current["folderName"].asString(),
            current["folderPath"].asString(),
            childrenVec,
            current["compress"].asBool(),
            current["delayCompress"].asBool(),
            current["pushEventOnly"].asBool(),
            current["watchSSRRamdumpHack"].asBool(),
            current["inVendor"].asBool(),
            current["maxSize"].asUInt64(),
            current["maxValidDay"].asUInt(),
            current["maxLogNumber"].asUInt()
        );
        configVec.push_back(config);
    }
    return configVec;
}

sp<BSPLogConfig> BSPLogManager::GetConfigByFolderName(std::string& folderName) {
    for (auto config : configVec) {
        if (config->GetFolderName() == folderName
            || (BSP_LOG_PATH + "/" + config->GetFolderName()) == folderName
            || config->GetFolderPath() == folderName)
            return config;

        // loop for find match child
        for (auto child : config->GetChildren()) {
            if (child->GetFolderName() == folderName
                || (BSP_LOG_PATH + "/" + config->GetFolderName() + "/" + child->GetFolderName()) == folderName
                || config->GetFolderPath() == folderName)
            return child;
        }
    }

    // Should not reach
    MISIGHT_LOGE("ERR! config not found for %s", folderName.c_str());
    return nullptr;
}

void BSPLogManager::WatchAllLogFolder() {
    // Create master log folder first
    bool isCreate = FileUtil::CreateDirectoryWithOwner(BSP_LOG_PATH.c_str(), AID_ROOT, AID_SYSTEM);
    if (isCreate) {
        FileUtil::ChangeMode(BSP_LOG_PATH.c_str(), (mode_t) 509U);
        // MISIGHT_LOGI("BSPLogManager: master log dir is not exist, successfully created dir:%s \n", BSP_LOG_PATH.c_str());
    }

    for (auto config : configVec) {
        WatchLogFolder(config);
    }

    auto task = std::bind(&BSPLogManager::MonitorSdcardWatchTask, this);
    GetLooper()->AddTask(task, DAY_SECONDS, true);
}

void BSPLogManager::WatchLogFolder(sp<BSPLogConfig>& config) {
    std::string fullPath = config->GetFolderName().empty() 
                    ? config->GetFolderPath() 
                    : config->GetIsInVendor() ? BSP_VENDOR_LOG_PATH + "/" + config->GetFolderName()
                    : BSP_LOG_PATH + "/" + config->GetFolderName();
    std::vector<sp<BSPLogConfig>> children = config->GetChildren();
    if (strstr(fullPath.c_str(), "/sdcard") && !sdcardReady) {
        MISIGHT_LOGI("sdcard path detected! now checking and waiting for sdcard init!!");
        while (access("/sdcard", 0)) {
            sleep(5);
        }
        MISIGHT_LOGI("sdcard is ready! let's continue!");
        sdcardReady = true;
    }

    bool isCreate = FileUtil::CreateDirectoryWithOwner(fullPath.c_str(), AID_ROOT, AID_SYSTEM);
    if (isCreate) {
        FileUtil::ChangeMode(fullPath.c_str(), (mode_t) 511U);
    }

    // watch children first
    for (auto child : children) {
        MISIGHT_LOGD("Start watch children");
        std::string childFullPath = fullPath + "/" + child->GetFolderName();
        isCreate = FileUtil::CreateDirectoryWithOwner(childFullPath.c_str(), AID_ROOT, AID_SYSTEM);
        if (isCreate) {
            FileUtil::ChangeMode(childFullPath.c_str(), (mode_t) 511U);
        }

        if (child->GetIsInVendor() && getVendorAidlService()) {
            if (!config->GetFolderName().empty()) {
                childFullPath = BSP_VENDOR_LOG_PATH + "/" + config->GetFolderName() + "/" + child->GetFolderName();
            }

            int wd;
            vendorLogMgrService->addFolderToWatch(inotifyVendorFd_, childFullPath, IN_CREATE | IN_ONLYDIR, &wd);
            MISIGHT_LOGI("Added Vendor Folder To Watch %s", wd > 0 ? "Succeed!" : "Failed!!");
            continue;
        }

        int err = AddFileWatch(childFullPath, IN_CREATE | IN_ONLYDIR);
        if (err == ENOTDIR) {
            if (!isDebugMode) {
                MISIGHT_LOGE("BSPLogManager: path %s is not a directory! we can only monitor directory, delete it.", childFullPath.c_str());
                unlink(fullPath.c_str());
            } else {
                MISIGHT_LOGE("WARNING: File %s in master log folder will be delete and recreate as directory in user build!!!!!", child->GetFolderName().c_str());
            }
            continue;
        }
        MISIGHT_LOGI("BSPLogManager: child watch dir added:%s \n", childFullPath.c_str());
    }

    if (config->GetIsInVendor()) {
        if (getVendorAidlService()) {
            int wd;
            vendorLogMgrService->addFolderToWatch(inotifyVendorFd_, fullPath, IN_CREATE | IN_ONLYDIR, &wd);
            MISIGHT_LOGI("Added Vendor Folder To Watch %s", wd > 0 ? "Succeed!" : "Failed!!");
        } else {
            MISIGHT_LOGE("Vendor Log Mgr Service Is Null!!");
        }
        return;
    }
    int err = AddFileWatch(fullPath, IN_CREATE | IN_ONLYDIR);
    if (err == ENOTDIR) {
        if (!isDebugMode) {
            MISIGHT_LOGE("BSPLogManager: path %s is not a directory! we can only monitor directory, delete it.", fullPath.c_str());
            unlink(fullPath.c_str());
        } else {
            MISIGHT_LOGE("WARNING: File %s in master log folder will be delete and recreate as directory in user build!!!!!", config->GetFolderName().c_str());
        }
        return;
    }

    if (!err)
        MISIGHT_LOGI("BSPLogManager: watch dir added:%s \n", fullPath.c_str());
}

void BSPLogManager::MonitorSdcardWatchTask() {
    for (auto config : configVec) {
        // Check is sdcard config
        if (config->GetFolderName().empty() && config->GetFolderPath().find("/sdcard/") != std::string::npos) {
            bool isWatched = false;
            // Check already in Watch List;
            for (const auto &it : fileMap_) {
                if (config->GetFolderPath() == it.first) {
                    isWatched = true;
                    break;
                }
            }

            if (!isWatched) {
                WatchLogFolder(config);
            }
        }
    }
}

void BSPLogManager::CleanAll() {
    for (auto config : configVec) {
        std::string parentFolderName = config->GetFolderName();
        std::string fullPath = config->GetFolderName().empty() 
                        ? config->GetFolderPath() 
                        : config->GetIsInVendor() ? BSP_VENDOR_LOG_PATH + "/" + config->GetFolderName()
                        : BSP_LOG_PATH + "/" + config->GetFolderName();
        if (config->GetIsInVendor()) {
            if (getVendorAidlService()) {
                bool _isSuccess;
                vendorLogMgrService->triggerCleanProcess(fullPath, &_isSuccess);
                MISIGHT_LOGI("Trigger Clean to Aidl %s!", _isSuccess ? "success!" : "Failed! Please Check Vendor Aidl log!");
            } else {
                MISIGHT_LOGE("MiSight Vendor Aidl is null!!! skip clean");
            }
        } else {
            CheckAndClean(fullPath);
        }

        for (auto config : config->GetChildren()) {
            std::string childFullPath = fullPath + "/" + config->GetFolderName();
            if (config->GetIsInVendor()) {
                if (!parentFolderName.empty()) {
                    childFullPath = BSP_VENDOR_LOG_PATH + "/" + parentFolderName + "/" + config->GetFolderName();
                }
                if (getVendorAidlService()) {
                    bool _isSuccess;
                    vendorLogMgrService->triggerCleanProcess(childFullPath, &_isSuccess);
                    MISIGHT_LOGI("Trigger Clean to Aidl %s!", _isSuccess ? "success!" : "Failed! Please Check Vendor Aidl log!");
                } else {
                    MISIGHT_LOGE("MiSight Vendor Aidl is null!!! skip clean");
                }
            } else {
                CheckAndClean(childFullPath);
            }
        }
    }
}

EVENT_RET BSPLogManager::OnEvent(sp<Event>& event) {
	// Stub
	return ON_SUCCESS;
}

bool BSPLogManager::CheckAndClean(std::string& folderPath) {
    size_t totalSize = 0;
    size_t totalSizeBefore = 0;
    bool needPushEvent = false;
    sp<ZipPack> zipPack = new ZipPack();

    sp<BSPLogConfig> config = GetConfigByFolderName(folderPath);
    if (config == nullptr) {
        MISIGHT_LOGE("get config failed for %s!!", folderPath.c_str());
        return false;
    }

    if (config->GetChildren().size() > 0) {
        MISIGHT_LOGD("found children items, ignore master folder clean");
        return false;
    }

    MISIGHT_LOGI("Start CheckAndClean %s", folderPath.c_str());

    DIR *dir = opendir(folderPath.c_str());
    if (dir == NULL) {
        MISIGHT_LOGE("BSPLogManager: CheckAndClean: can not open dir %s ", folderPath.c_str());
        return false;
    }

    std::vector<std::string> fileNameList;
    struct dirent *file;
    // Get Dir Info
    while ((file = readdir(dir)) != NULL) {
        std::string fullPath = folderPath + "/" + file->d_name;
        // ignore . and ..
        if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, ".."))
            continue;

        // ignore non-ssrdump file if watchSSRRamdumpHack enabled
        if (config->GetWatchSSRRamdumpHack()) {
            std::regex e("ramdump_.*_\\d{4}-\\d{1,2}-\\d{1,2}_\\d{1,2}-\\d{1,2}-\\d{1,2}.*\\..*", std::regex::icase);

            // For old ramdump file format: ramdump_sensor_2022-08-05_18-19-58.tgz
            if (!regex_match(std::string(file->d_name), e)) {
                MISIGHT_LOGW("OldRamdumpFileFormatHack: fileName %s not matched regex, skip!", file->d_name);
                continue;
            }
        }

        fileNameList.push_back(file->d_name);
        if (file->d_type == DT_DIR) {
            totalSize += FileUtil::GetDirectorySize(fullPath);
        } else if (file->d_type == DT_REG) {
            if (strstr(file->d_name, ".zip") != NULL) {
                MISIGHT_LOGD("BSPLogManager: Detected zipped log %s ", file->d_name);
            }
            totalSize += FileUtil::GetFileSize(fullPath);
        } else {
            MISIGHT_LOGI("BSPLogManager: Only directory with date named is allowed, but unknown file %s was detected!!", file->d_name);
        }
    }

    if (closedir(dir)) {
        MISIGHT_LOGE("Close Dir failed!");
    }

    totalSizeBefore = totalSize;

    if (config->GetNeedPushEventOnly()) {
        if (totalSize > config->GetMaxSize() * 1024) {
            SendEventToDft(folderPath, totalSizeBefore, totalSize);
        }
        return true;
    }

    if (fileNameList.size() == 0) {
        MISIGHT_LOGI("%s is empty, no need clean!", folderPath.c_str());
        return true;
    }

    std::sort(fileNameList.begin(), fileNameList.end());

    // Step.1 Clean out when dir condition matched
    while ((config->GetMaxLogNumber() != 0 && (fileNameList.size() > config->GetMaxLogNumber()))
            || (config->GetMaxSize() != 0 && (totalSize > (config->GetMaxSize() * 1024)))
            || ((config->GetValidDay() != 0 && fileNameList.size() != 0)
                && CompareTimeStrExpired(fileNameList[0], 
                                        TimeUtil::GetCurrentTime() - GetOffsetDay(config->GetValidDay()),
                                        config->GetWatchSSRRamdumpHack() ))
    ) {
        std::string fullPath = folderPath + "/" + fileNameList[0];
        size_t dirSize = 0;
        // We just care the size exceed.
        needPushEvent = config->GetMaxSize() != 0 && (totalSize > (config->GetMaxSize() * 1024));

        if (fileNameList.size() == 0) {
            MISIGHT_LOGW("%s is clean, should not enter clean loop again!", folderPath.c_str());
            return true;
        }

        MISIGHT_LOGI("Real Clean: fileName %s, fileNameList size %zu, totalSize %zu, "
                        "NowTime %zu, OffsetDay %zu, time offset %zu",
                        fileNameList[0].c_str(), fileNameList.size(), totalSize, 
                        TimeUtil::GetCurrentTime(), 
                        GetOffsetDay(config->GetValidDay()), 
                        TimeUtil::GetCurrentTime() - GetOffsetDay(config->GetValidDay()));
        if (!FileUtil::DeleteDirectoryOrFile(fullPath.c_str(), &dirSize)) {
            MISIGHT_LOGI("DeleteDirectoryOrFile succeed, removed size %zu bytes (%zu MB)", dirSize, (dirSize / 1024 / 1024));
            fileNameList.erase(fileNameList.begin());
            totalSize -= dirSize;
        } else {
            MISIGHT_LOGE("remove %s failed! retrying in 5 sec...", fullPath.c_str());
            chmod(fullPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
            sleep(5);
        }
    }

    // Step.2 compress if needed
    if (config->GetNeedCompress() && fileNameList.size() > 0) {
        size_t fileListSize = (config->GetNeedDelayCompress() || loaded_) ? fileNameList.size() - 1 : fileNameList.size(); // Only do full compress when load
        for (int i = 0; i < fileListSize; i++) {
            if (strstr(fileNameList[i].c_str(), ".zip")) continue;

            size_t dirSize = 0;
            std::string fullPath = folderPath + "/" + fileNameList[i];
            std::vector<std::string> logPaths;
            if (getZipFileList(fullPath, logPaths)) {
                MISIGHT_LOGE("get file list for compress failed!!");
                return false;
            }
            if (!zipPack->CreateZip(fullPath + ".zip")) {
                MISIGHT_LOGE("Create new zip failed!!");
                return false;
            }
            zipPack->AddFiles(logPaths);
            zipPack->Close(); // force flush
            FileUtil::DeleteDirectoryOrFile(fullPath.c_str(), &dirSize);
            FileUtil::ChangeMode(fullPath + ".zip", (mode_t) 509U);
        }

        MISIGHT_LOGD("Get Total Size again after compress done.");
        totalSize = 0;
        dir = opendir(folderPath.c_str());
        while ((file = readdir(dir)) != NULL && needPushEvent) {
            std::string fullPath = folderPath + "/" + file->d_name;
            if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..")) 
                continue;

            if (file->d_type == DT_DIR) {
                totalSize += FileUtil::GetDirectorySize(fullPath);
            } else if (file->d_type == DT_REG && strstr(file->d_name, ".zip") != NULL) {
                MISIGHT_LOGD("BSPLogManager: Detected zipped log %s ", file->d_name);
                totalSize += FileUtil::GetFileSize(fullPath);
            }
        }
        if (closedir(dir)) {
            MISIGHT_LOGE("Close Dir failed!");
        }
    }
    if (needPushEvent) {
        SendEventToDft(folderPath, totalSizeBefore, totalSize);
    }

    return true;
}

void BSPLogManager::SendEventToDft(std::string& dirFullPath, size_t dirSizeBefore, size_t dirSizeAfter) {
    const std::string dirFullPathConst = dirFullPath;
    SendEventToDft(dirFullPathConst, dirSizeBefore, dirSizeAfter);
}

void BSPLogManager::SendEventToDft(const std::string& dirFullPath, size_t dirSizeBefore, size_t dirSizeAfter) {
    Json::Value root;

    MISIGHT_LOGI("Sending Event To Dft, dirFullPath: %s, dirSizeBefore %zu (%f M), dirSizeAfter %zu (%f M)\n", 
                dirFullPath.c_str(), dirSizeBefore, (float)dirSizeBefore / 1024 / 1024, dirSizeAfter, (float)dirSizeAfter / 1024 / 1024);

    root["DirFullPath"] = dirFullPath;
    root["DirSizeBefore"] = dirSizeBefore;
    root["DirSizeAfter"] = dirSizeAfter;

    SendEventToDft(root);
}

void BSPLogManager::SendEventToDft(Json::Value& root) {
    sp<Event> event = new Event("LogManager");
    if (event == nullptr) {
        MISIGHT_LOGE("Create new event failed!");
        return;
    }

    event->eventId_ = 908002003;
    event->happenTime_ = std::time(nullptr);
    event->messageType_ = Event::MessageType::STATISTICS_EVENT;
    event->SetValue(EventUtil::DEV_SAVE_EVENT, JsonUtil::ConvertJsonToStr(root));

    GetPluginContext()->InsertEventToPipeline("dftFaultEvent", event);

}

bool BSPLogManager::PreToLoad() {
    auto context = GetPluginContext();

    bool disableLogMgr = context->GetSysProperity("persist.bsp.logmgr.disable", "0") == "1" 
                    || context->GetSysProperity("persist.bsp.logmgr.disable", "false") == "true";

    if (disableLogMgr) {
        MISIGHT_LOGI("Will not load caused by disableLogMgr is true");
    }

    return !disableLogMgr;
}

void BSPLogManager::OnLoad()
{
    SetName("BSPLogManager");
    SetVersion("BSPLogManager.0");
    MISIGHT_LOGI("BSPLogManager OnLoad \n");

    auto context = GetPluginContext();

    isDebugMode = context->GetSysProperity("ro.debuggable", "0") == "1" 
                || context->GetSysProperity("ro.debuggable", "false") == "true";

    GetLooper()->AddFileEventHandler(BSP_LOG_LISTENER_TAG, this);

    auto task = std::bind(&BSPLogManager::DelayInit, this);
    GetLooper()->AddTask(task, 5, false);

    MISIGHT_LOGI("BSPLogManager Load Done! isDebug:%d\n", isDebugMode);
    loaded_ = true;

    // Cloud Control Updater
    EventRange range(EVENT_SERVICE_CONFIG_UPDATE_ID);
    AddListenerInfo(Event::MessageType::PLUGIN_MAINTENANCE, range);
    context->RegisterUnorderedEventListener(this);

}

std::shared_ptr<ILogMgr> BSPLogManager::getVendorAidlService() {
    if (vendorLogMgrService) {
        return vendorLogMgrService;
    }
    std::string instanceName = std::string(ILogMgr::descriptor) + "/default";
    int tryInitAidlCount = 5;
    while (vendorLogMgrService == nullptr && tryInitAidlCount > 0) {
        vendorLogMgrService = ILogMgr::fromBinder(ndk::SpAIBinder(AServiceManager_checkService(instanceName.c_str())));
        if (vendorLogMgrService) {
            MISIGHT_LOGE("Got Vendor LogMgr Service! retry count remain %d", tryInitAidlCount);
            break;
        }
        tryInitAidlCount--;
    }

    return vendorLogMgrService;
}

void BSPLogManager::DelayInit() {
    // Init Vendor Aidl in Delay Init, make sure aidl loaded before we try to get it.
    getVendorAidlService();
    const std::shared_ptr<ILogMgrCallback> vendorLogMgrCallback = ndk::SharedRefBase::make<LogMgrCallback>(this);

    // Init Config after Vendor Aidl Loaded, so Vendor Config will be updated here.
    std::string cfgPath = GetPluginContext()->GetConfigDir(BSP_LOG_CONFIG_NAME) + BSP_LOG_CONFIG_NAME;
    configVec = GetConfigFromJson(cfgPath);

    // Clean All first when plugin load.
    // it may failed caused by userdata not decrypted, but whatever, let's try it first =_=.
    MISIGHT_LOGI("Trying do clean when plugin load...\n");
    CleanAll();

    // init for Aidl
    if (getVendorAidlService()) {
        vendorLogMgrService->initNewINotify(&inotifyVendorFd_);
        if (inotifyVendorFd_ > 0) {
            MISIGHT_LOGI("init Vendor New INotify Succeed, fd %d", inotifyVendorFd_);
        }
        vendorLogMgrService->registerCallBack(vendorLogMgrCallback);
    } else {
        MISIGHT_LOGE("MiSight Vendor LogMgr Service is nullptr!!");
    }

    WatchAllLogFolder();
}

void BSPLogManager::OnUnload()
{
    MISIGHT_LOGI("BSPLogManager OnUnload \n");
}

void BSPLogManager::OnUnorderedEvent(sp<Event> event)
{
    if (event->eventId_ == EVENT_SERVICE_CONFIG_UPDATE_ID) {
        MISIGHT_LOGD("Received Config Update, update config here");
        std::string cfgPath = GetPluginContext()->GetConfigDir(BSP_LOG_CONFIG_NAME) + BSP_LOG_CONFIG_NAME;
        configVec = GetConfigFromJson(cfgPath);
        MISIGHT_LOGD("update config done!");
    }
}

// FileEventListener
int32_t BSPLogManager::OnFileEvent(int fd, int type)
{
    const int bufSize = 2048;
    char buffer[bufSize] = {0};
    char *offset = nullptr;
    struct inotify_event *event = nullptr;
    if (inotifyFd_ < 0) {
        MISIGHT_LOGE("BSPLogFileListener Invalid inotify fd:%d", inotifyFd_);
        return 1;
    }

    int len = read(fd, buffer, bufSize);
    if (len < sizeof(struct inotify_event)) {
        MISIGHT_LOGE("BSPLogFileListener failed to read event");
        return 1;
    }

    offset = buffer;
    event = (struct inotify_event *)buffer;
    while ((reinterpret_cast<char *>(event) - buffer) < len) {
        for (const auto &it : fileMap_) {
            if (it.second != event->wd) {
                continue;
            }

            if (event->len <= 0) {
                MISIGHT_LOGD("Remove watch because event len is zero, user may removed watched folder, wd: %d", event->wd);
                inotify_rm_watch(fd, event->wd);
                std::map<std::string, int>::iterator iter = fileMap_.find(it.first);
                fileMap_.erase(iter);
                break;
            }

            if (event->name[event->len - 1] != '\0') {
                event->name[event->len - 1] = '\0';
            }
            std::string filePath = it.first + "/" + std::string(event->name);
            std::string dirPath = it.first;
            MISIGHT_LOGD("handle file event in %s \n", filePath.c_str());
            if (loaded_ && std::string(event->name).find(".zip") == std::string::npos)
                CheckAndClean(dirPath);
        }
        int tmpLen = sizeof(struct inotify_event) + event->len;
        event = (struct inotify_event *)(offset + tmpLen);
        offset += tmpLen;
    }
    return 1;
}

int32_t BSPLogManager::GetPollFd()
{
    if (inotifyFd_ > 0) {
        return inotifyFd_;
    }

    inotifyFd_ = inotify_init();
    if (inotifyFd_ == -1) {
        MISIGHT_LOGE("BSPLogFileListener failed to init inotify: %s.\n", strerror(errno));
        return -1;
    }

    MISIGHT_LOGI("BSPLogFileListener GetPollFd %d \n", inotifyFd_);
    return inotifyFd_;
}

int32_t BSPLogManager::AddFileWatch(std::string watchPath, uint32_t mask)
{
    if (inotifyFd_ < 0) {
        MISIGHT_LOGE("inotify is not inited, Can not Add File to Watch now.\n");
        return -1;
    }
    int wd = inotify_add_watch(inotifyFd_, watchPath.c_str(), mask);
    if (wd < 0) {
        MISIGHT_LOGE("BSPLogFileListener failed to add watch entry : %s.\n", strerror(errno));
        return errno;
    }

    MISIGHT_LOGI("BSPLogFileListener Added File To Watch: %s, inotifyFd_ %d \n", watchPath.c_str(), inotifyFd_);
    fileMap_[watchPath] = wd;
    return 0;
}

int32_t BSPLogManager::GetPollEventType()
{
    return EPOLLIN;
}


}
}
