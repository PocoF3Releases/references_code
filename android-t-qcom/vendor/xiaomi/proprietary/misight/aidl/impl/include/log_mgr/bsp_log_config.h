/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight bsp log config class head file
 *
 */
#ifndef BSP_LOG_CONFIG_H
#define BSP_LOG_CONFIG_H
#include <string>
#include <vector>

#include <utils/RefBase.h>

namespace aidl::vendor::xiaomi::hardware::misight {
using namespace android;

class BSPLogConfig : public android::RefBase {
public:
    BSPLogConfig(const std::string& name, 
                    const std::string& path, 
                    const std::vector<sp<BSPLogConfig>> children,
                    bool compress, 
                    bool delayCompress,
                    bool pushEventOnly,
                    bool watchSSRRamdumpHack,
                    bool inVendor,
                    uint64_t maxSize, 
                    uint32_t maxValidDay, 
                    uint32_t maxLogNumber);

    virtual ~BSPLogConfig(){}

    std::string GetFolderName();
    std::string GetFolderPath();
    bool GetNeedCompress();
    bool GetNeedDelayCompress();
    bool GetNeedPushEventOnly();
    bool GetWatchSSRRamdumpHack();
    bool GetIsInVendor();
    uint64_t GetMaxSize();
    uint32_t GetValidDay();
    uint32_t GetMaxLogNumber();
    std::vector<sp<BSPLogConfig>> GetChildren();

private:
    std::string folderName_; // Folder Name
    std::string folderPath_; // Folder Absolute path, eg: "/sdcard/diag_logs"
    std::vector<sp<BSPLogConfig>> children_;
    bool compress_; // Need Compress after clean
    bool delayCompress_; // Do not compress the latest log
    bool pushEventOnly_; // Do not clean if any condition match, push event only
    bool watchSSRRamdumpHack_; // Hack to watch legacy ssr dump, like 8450/8475 platform
    bool inVendor_; // is Vendor Module
    uint64_t maxSize_; // Max Size of Folder (KB)
    uint32_t maxValidDay_; // For Outdated log check
    uint32_t maxLogNumber_; // How much log can be exist in a type
};

}
#endif
