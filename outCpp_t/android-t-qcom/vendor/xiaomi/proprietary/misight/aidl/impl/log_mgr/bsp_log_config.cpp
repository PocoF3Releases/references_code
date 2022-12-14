/*
 * Copyright (C) Xiaomi Inc.
 * Description: bsp log config obj implements file
 *
 */

#include <log_mgr/bsp_log_config.h>

namespace aidl::vendor::xiaomi::hardware::misight {
using namespace android;

BSPLogConfig::BSPLogConfig(const std::string& name, 
                            const std::string& path, 
                            const std::vector<sp<BSPLogConfig>> children,
                            bool compress, 
                            bool delayCompress, 
                            bool pushEventOnly, 
                            bool watchSSRRamdumpHack,
                            bool inVendor,
                            uint64_t maxSize, 
                            uint32_t maxValidDay, 
                            uint32_t maxLogNumber)
                        : folderName_(name), 
                            folderPath_(path), 
                            children_(children), 
                            compress_(compress), 
                            delayCompress_(delayCompress), 
                            pushEventOnly_(pushEventOnly),
                            watchSSRRamdumpHack_(watchSSRRamdumpHack),
                            inVendor_(inVendor),
                            maxSize_(maxSize), 
                            maxValidDay_(maxValidDay), 
                            maxLogNumber_(maxLogNumber)
{

}

std::string BSPLogConfig::GetFolderName()
{
    return folderName_;
}

std::string BSPLogConfig::GetFolderPath()
{
    return folderPath_;
}

std::vector<sp<BSPLogConfig>> BSPLogConfig::GetChildren()
{
    return children_;
}

bool BSPLogConfig::GetNeedCompress()
{
    return compress_;
}

bool BSPLogConfig::GetNeedDelayCompress()
{
    return delayCompress_;
}

bool BSPLogConfig::GetNeedPushEventOnly()
{
    return pushEventOnly_;
}

bool BSPLogConfig::GetWatchSSRRamdumpHack()
{
    return watchSSRRamdumpHack_;
}

bool BSPLogConfig::GetIsInVendor()
{
    return inVendor_;
}

uint64_t BSPLogConfig::GetMaxSize()
{
    return maxSize_;
}

uint32_t BSPLogConfig::GetValidDay()
{
    return maxValidDay_;
}

uint32_t BSPLogConfig::GetMaxLogNumber()
{
    return maxLogNumber_;
}

}
