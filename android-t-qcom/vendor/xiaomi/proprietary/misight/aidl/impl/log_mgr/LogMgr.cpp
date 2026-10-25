/*
 * Copyright (c) 2022 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include <log_mgr/LogMgr.h>

#include <android-base/logging.h>

#include <fstream>
#include <iostream>
#include <regex>

#include <stdlib.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <dirent.h>
#include <android/binder_status.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/inotify.h>

#include <log_mgr/bsp_log_config.h>
#include <log_mgr/LogMgrUtil.h>
#include "LogMgrLinkedCallback.h"

#include <utils/FileUtil.h>
#include <utils/zip/zip_pack.h>

namespace aidl::vendor::xiaomi::hardware::misight {
using namespace android;
using namespace LogMgrUtil;

namespace {
// Wrap LinkedCallback::OnCallbackDied() into a void(void*).
void OnCallbackDiedWrapped(void* cookie) {
    LogMgrLinkedCallback* linked = reinterpret_cast<LogMgrLinkedCallback*>(cookie);
    linked->OnCallbackDied();
}
}  // namespace

namespace {
bool IsDeadObjectLogged(const ndk::ScopedAStatus& ret) {
    if (ret.isOk()) return false;
    if (ret.getStatus() == ::STATUS_DEAD_OBJECT) return true;
    LOG(ERROR) << "Cannot call onCleanProcessDone on callback: " << ret.getDescription();
    return false;
}
}  // namespace

LogMgr::LogMgr() 
    : death_recipient_(AIBinder_DeathRecipient_new(&OnCallbackDiedWrapped)) {
    LOG(INFO) << "LogMgr: Ready!";
    epollFd_ = -1;
}

// Log Mgr Aidl Interface
::ndk::ScopedAStatus LogMgr::initNewINotify(int32_t* return_inotifyFd) {
    *return_inotifyFd = inotify_init();
    LOG(INFO) << "LogMgr: initNewINotify: inotifyFd " << *return_inotifyFd;
    if (*return_inotifyFd == -1) {
        return ndk::ScopedAStatus(AStatus_fromStatus(STATUS_INVALID_OPERATION));
    }

    int ret = addToEpoll(*return_inotifyFd);
    if (ret) {
        LOG(INFO) << "LogMgr: initNewINotify: addFd Done! ret: " << ret;
    }

    if (!loaded_) {
        if (pthread_create(&fileMonitorThread, NULL, &EPollEventTriggerThread, (void*)this))
            LOG(ERROR) << "failed to create fileMonitorThread";
    }

    loaded_ = true;
    LOG(INFO) << "LogMgr: initNewINotify: Done! marked service as loaded! ";
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus LogMgr::addFolderToWatch(int32_t in_inotifyFd, const std::string& in_folderPath, int32_t in_mask, int32_t* return_wd) {
    LOG(DEBUG) << "LogMgr: addFolderToWatch: inotifyFd " << in_inotifyFd;
    if (in_inotifyFd < 0) {
        LOG(ERROR) << "inotifyFd is invalid!!\n";
        return ndk::ScopedAStatus(AStatus_fromStatus(STATUS_NO_INIT));
    }

    bool isCreate = FileUtil::CreateDirectoryWithOwner(in_folderPath.c_str(), AID_ROOT, AID_SYSTEM);
    if (isCreate) {
        FileUtil::ChangeMode(in_folderPath.c_str(), (mode_t) 511U);
    }

    int wd = inotify_add_watch(in_inotifyFd, in_folderPath.c_str(), in_mask);
    if (wd < 0) {
        LOG(ERROR) << "LogMgr: addFolderToWatch: failed to add watch entry, error: " << strerror(errno);
        return ndk::ScopedAStatus(AStatus_fromStatus(STATUS_UNKNOWN_ERROR));
    }

    LOG(INFO) << "LogMgr: addFolderToWatch: Added File To Watch: " << in_folderPath << ", inotifyFd_ " << in_inotifyFd;
    fileMap_[in_folderPath] = wd;
    *return_wd = wd;

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus LogMgr::triggerCleanProcess(const std::string& in_folderPath, bool* _aidl_return) {
    if (configVec.size() == 0) {
        LOG(ERROR) << "triggerCleanProcess: Global config is null, Please do updateGlobalConfig first!";
        return ndk::ScopedAStatus(AStatus_fromStatus(STATUS_NO_INIT));
    }

    *_aidl_return = CheckAndClean(in_folderPath);
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus LogMgr::updateGlobalConfig(const std::string& in_jsonStr, bool* _aidl_return) {
    Json::Value root;
    if (!JsonUtil::ConvertStrToJson(in_jsonStr, root)) {
        LOG(ERROR) << "parse config json error!!";
        *_aidl_return = false;
        return ndk::ScopedAStatus(AStatus_fromStatus(STATUS_UNKNOWN_ERROR));
    }

    configVec = ConvertConfigObjToVec(root);

    *_aidl_return = true;
    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus LogMgr::registerCallBack(const std::shared_ptr<::aidl::vendor::xiaomi::hardware::misight::ILogMgrCallback>& in_cb) {
    if (in_cb == nullptr) {
        // For now, this shouldn't happen because argument is not nullable.
        return ndk::ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }

    {
        std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
        callbacks_.emplace_back(LogMgrLinkedCallback::Make(ref<LogMgr>(), in_cb));
        // unlock
    }

    return ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus LogMgr::unregisterCallBack(const std::shared_ptr<::aidl::vendor::xiaomi::hardware::misight::ILogMgrCallback>& in_cb) {
    if (in_cb == nullptr) {
        // For now, this shouldn't happen because argument is not nullable.
        return ndk::ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
    }

    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);

    auto matches = [in_cb](const auto& linked) {
        return linked->callback()->asBinder() == in_cb->asBinder();  // compares binder object
    };
    auto it = std::remove_if(callbacks_.begin(), callbacks_.end(), matches);
    bool removed = (it != callbacks_.end());
    callbacks_.erase(it, callbacks_.end());  // calls unlinkToDeath on deleted callbacks.
    return removed ? ndk::ScopedAStatus::ok()
                   : ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

int LogMgr::addToEpoll(int fd) {
    int res;
    struct epoll_event eventItem;

    if (epollFd_ <= 0) {
        epollFd_ = epoll_create(MAX_FDS);
    }

    memset(&eventItem, 0 , sizeof(eventItem));
    eventItem.events = EPOLLIN;
    eventItem.data.fd = fd;
    res = epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &eventItem);

    return res;
}

void* LogMgr::EPollEventTriggerThread(void* logMgr) {
    struct epoll_event events[50];
    LogMgr* logmgr = (LogMgr*) logMgr;

    while (1) {
        int epollRes = epoll_wait(epollFd_, events, 50, -1);
        if (epollRes < 0) {
            sleep(5);
            continue;
        }

        for (int i = 0; i < size_t(epollRes); i++) {
            const struct epoll_event& eventItem = events[i];
            if (eventItem.events & EPOLLIN) {
                logmgr->OnFileEvent(eventItem.data.fd, eventItem.events);
            }
        }
    }
}

void LogMgr::OnFileEvent(int fd, int events) {
    const int bufSize = 2048;
    char buffer[bufSize] = {0};
    char *offset = nullptr;
    struct inotify_event *event = nullptr;
    if (fd < 0) {
        LOG(ERROR) << "LogMgr: OnFileEvent: inotifyFd is invalid!! fd: " << fd;
        return;
    }

    int len = read(fd, buffer, bufSize);
    if (len < sizeof(struct inotify_event)) {
        LOG(ERROR) << "LogMgr: OnFileEvent: failed to read event";
        return;
    }

    offset = buffer;
    event = (struct inotify_event *)buffer;
    while ((reinterpret_cast<char *>(event) - buffer) < len) {
        for (const auto &it : fileMap_) {
            if (it.second != event->wd) {
                continue;
            }

            if (event->len <= 0) {
                LOG(INFO) << "Remove watch because event len is zero, user may removed watched folder, wd: " << event->wd;
                inotify_rm_watch(fd, event->wd);
                std::map<std::string, int>::iterator iter = fileMap_.find(it.first);
                fileMap_.erase(iter);
                break;
            }

            if (event->name[event->len - 1] != '\0') {
                event->name[event->len - 1] = '\0';
            }

            std::string filePath = it.first + "/" + std::string(event->name);
            const std::string dirPath = it.first;
            LOG(INFO) << "handle file event in " << filePath;
            if (std::string(event->name).find(".zip") == std::string::npos)
                CheckAndClean(dirPath);
        }
        int tmpLen = sizeof(struct inotify_event) + event->len;
        event = (struct inotify_event *)(offset + tmpLen);
        offset += tmpLen;
    }
}

// Log Mgr Implement & Main Functions
std::vector<sp<BSPLogConfig>> LogMgr::ConvertConfigObjToVec(Json::Value& obj) {
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
            LOG(ERROR) << "Invalid Config found!!Please check the config json!! folderName: " << folderName << " will be skip";
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

sp<BSPLogConfig> LogMgr::GetConfigByFolderName(const std::string& folderName) {
    for (auto config : configVec) {
        if (config->GetFolderName() == folderName
            || (BSP_VENDOR_LOG_PATH + "/" + config->GetFolderName()) == folderName
            || config->GetFolderPath() == folderName)
            return config;

        // loop for find match child
        for (auto child : config->GetChildren()) {
            if (child->GetFolderName() == folderName
                || (BSP_VENDOR_LOG_PATH + "/" + config->GetFolderName() + "/" + child->GetFolderName()) == folderName
                || config->GetFolderPath() == folderName)
            return child;
        }
    }

    // Should not reach
    LOG(ERROR) << "ERR! config not found for " << folderName.c_str();
    return nullptr;
}


bool LogMgr::CheckAndClean(const std::string& folderPath) {
    size_t totalSize = 0;
    size_t totalSizeBefore = 0;
    bool needPushEvent = false;
    sp<ZipPack> zipPack = new ZipPack();

    sp<BSPLogConfig> config = GetConfigByFolderName(folderPath);
    if (config == nullptr) {
        LOG(ERROR) << "CheckAndClean: get config failed for " << folderPath.c_str();
        return false;
    }

    if (config->GetChildren().size() > 0) {
        LOG(DEBUG) << "CheckAndClean: found children items, ignore master folder clean";
        return false;
    }

    LOG(INFO) << "CheckAndClean: Start CheckAndClean " << folderPath.c_str();

    DIR *dir = opendir(folderPath.c_str());
    if (dir == NULL) {
        LOG(ERROR) << "CheckAndClean: can not open dir " << folderPath.c_str();
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
                LOG(WARNING) << "OldRamdumpFileFormatHack: fileName " << file->d_name << " not matched regex, skip!";
                continue;
            }
        }

        fileNameList.push_back(file->d_name);
        if (file->d_type == DT_DIR) {
            totalSize += FileUtil::GetDirectorySize(fullPath);
        } else if (file->d_type == DT_REG) {
            if (strstr(file->d_name, ".zip") != NULL) {
                LOG(DEBUG) << "CheckAndClean: Detected zipped log " << file->d_name;
            }
            totalSize += FileUtil::GetFileSize(fullPath);
        } else {
            LOG(INFO) << "CheckAndClean: Only directory with date named is allowed, but unknown file " << file->d_name << " was detected!!";
        }
    }

    if (closedir(dir)) {
        LOG(ERROR) << "Close Dir failed!";
    }

    totalSizeBefore = totalSize;

    if (config->GetNeedPushEventOnly()) {
        if (totalSize > config->GetMaxSize() * 1024) {
            SendEventToDft(folderPath, totalSizeBefore, totalSize);
        }
        return true;
    }

    if (fileNameList.size() == 0) {
        LOG(INFO) << folderPath << " is empty, no need clean!";
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
            LOG(WARNING) << folderPath << " is clean, should not enter clean loop again!";
            return true;
        }

        LOG(INFO) << "Real Clean Start: fileName " << fileNameList[0] << ", fileNameList size " 
                    << fileNameList.size() << ", totalSize , " << totalSize << " NowTime " << TimeUtil::GetCurrentTime() 
                    << " OffsetDay " << GetOffsetDay(config->GetValidDay()) 
                    << " time offset " << TimeUtil::GetCurrentTime() - GetOffsetDay(config->GetValidDay());
        if (!FileUtil::DeleteDirectoryOrFile(fullPath.c_str(), &dirSize)) {
            LOG(INFO) << "DeleteDirectoryOrFile succeed, removed size " << dirSize << " bytes (" << (dirSize / 1024 / 1024) << "MB)";
            fileNameList.erase(fileNameList.begin());
            totalSize -= dirSize;
        } else {
            LOG(ERROR) << "remove " << fullPath << "failed! skipped this file for now...";
            fileNameList.erase(fileNameList.begin());
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
                LOG(ERROR) << "get file list for compress failed!!";
                return false;
            }
            if (!zipPack->CreateZip(fullPath + ".zip")) {
                LOG(ERROR) << "Create new zip failed!!";
                return false;
            }
            zipPack->AddFiles(logPaths);
            zipPack->Close(); // force flush
            FileUtil::DeleteDirectoryOrFile(fullPath.c_str(), &dirSize);
            FileUtil::ChangeMode(fullPath + ".zip", (mode_t) 509U);
        }

        LOG(DEBUG) << "Get Total Size again after compress done.";
        totalSize = 0;
        dir = opendir(folderPath.c_str());
        while ((file = readdir(dir)) != NULL && needPushEvent) {
            std::string fullPath = folderPath + "/" + file->d_name;
            if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..")) 
                continue;

            if (file->d_type == DT_DIR) {
                totalSize += FileUtil::GetDirectorySize(fullPath);
            } else if (file->d_type == DT_REG && strstr(file->d_name, ".zip") != NULL) {
                LOG(DEBUG) << "BSPLogManager: Detected zipped log "  << file->d_name;
                totalSize += FileUtil::GetFileSize(fullPath);
            }
        }
        if (closedir(dir)) {
            LOG(ERROR) << "Close Dir failed!";
        }
    }
    if (needPushEvent) {
        SendEventToDft(folderPath, totalSizeBefore, totalSize);
    }

    return true;
}

void LogMgr::SendEventToDft(const std::string dirFullPath, int64_t dirSizeBefore, int64_t dirSizeAfter) {
    LOG(DEBUG) << "SendEventToDft " << dirFullPath << " dirSizeBefore " << dirSizeBefore << " dirSizeAfter " << dirSizeAfter;

    // Notify all callbacks
    std::unique_lock<decltype(callbacks_lock_)> lock(callbacks_lock_);

    // is_dead notifies a callback and return true if it is dead.
    auto is_dead = [&](const auto& linked) {
        const std::string dfxJsonStr = generateDfxJsonStr(dirFullPath, dirSizeBefore, dirSizeAfter);
        auto res = linked->callback()->onCleanProcessDone(dfxJsonStr, -1);
        return IsDeadObjectLogged(res);
    };
    auto it = std::remove_if(callbacks_.begin(), callbacks_.end(), is_dead);
    callbacks_.erase(it, callbacks_.end());  // calls unlinkToDeath on deleted callbacks.
    lock.unlock();
}

}  // namespace aidl::vendor::xiaomi::hardware::misight

