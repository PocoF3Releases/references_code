/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight dev info head file
 *
 */
#ifndef MISIGHT_UTILS_DEV_INFO_H
#define MISIGHT_UTILS_DEV_INFO_H

#include <string>

#include "plugin.h"

namespace android {
namespace MiSight {
namespace DevInfo {
const std::string DEV_VERSION_PROP = "ro.build.version.incremental";
const std::string DEV_BUILD_FINGERPRINT = "ro.build.fingerprint";
const std::string DEV_PSN_PROP = "ro.ril.oem.psno";
const std::string DEV_SN_PROP = "ro.ril.oem.sno";
const std::string DEV_FACTORY_MODE = "ro.boot.factorybuild";

const std::string DEV_PRODUCT_LOCALE = "ro.product.locale";
const std::string DEV_MISIGHT_TEST_MODE = "persist.sys.misight.testmode";
const std::string DEV_MISIGHT_UE_MODE = "persist.sys.misight.ue_mode";
const std::string DEV_MISIGHT_REGINE = "ro.miui.region";

enum MiuiVersionType {
    PRE,
    DEV,
    STABLE
};

constexpr int UE_SWITCH_OFF = 0;
constexpr int UE_SWITCH_ON = 1;
constexpr int UE_SWITCH_ONCTRL = 2;

MiuiVersionType GetMiuiVersionType(PluginContext* context);
std::string GetMiuiVersion(PluginContext* context);
bool IsMiuiStableVersion(const std::string& version);
bool IsMiuiDevVersion(const std::string& version);
bool IsInnerVersion(const std::string& version);
void SetUploadSwitch(PluginContext* context, int onSwitch);
std::string GetUploadSwitch(PluginContext* context);
void SetRunMode(PluginContext* context, const std::string& runMode, const std::string& filePath);
bool GetRunMode(PluginContext* context, std::string& runMode, std::string& filePath);
void SetActivateTime(uint64_t activeTime);
uint64_t GetActivateTime();

}
} // namespace MiSight
} // namespace android
#endif

