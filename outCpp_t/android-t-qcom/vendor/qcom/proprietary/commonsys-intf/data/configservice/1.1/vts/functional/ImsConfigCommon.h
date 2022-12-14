/*****************************************************************************
Copyright (c) 2022 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
*****************************************************************************/

#ifndef IMS_CONFIG_COMMON_HEADERS
#define IMS_CONFIG_COMMON_HEADERS

#include <log/log.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <zlib.h>
#include <map>
#include <sstream>
#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetCallbackBase.h>

#define LOG_TAG "IMSConfig_VTS"
#define GZIP_WINDOWSIZE 15
#define GZIP_CFACTOR 9
#define GZIP_BSIZE 8096
#define GZIP_OUTPUTBUFFSIZE 32768

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

constexpr char slot_id_key[] = "slot_id";
constexpr char isCompressed_key[] = "isCompressed";
constexpr char xmlFileName_key[] = "xmlFile";
constexpr char AppToken_key[] = "app_token";
constexpr char PROPERTIES_FILE[] = "/data/properties.txt";

extern std::map<std::string, std::string> g_configValues;
extern uint32_t userData;
extern std::string userProvidedAppId;

constexpr char kCallbackOnCommandStatus[] = "onCommandStatus";
constexpr char kCallbackOnReconfigNeeded[] = "onReconfigNeeded";
constexpr char kCallbackOnUceStatusUpdate[] = "onUceStatusUpdate";
constexpr char kCallbackOnTokenFetchRequest[] = "onTokenFetchRequest";
constexpr char kCallbackOnGetUpdatedSettings[] = "onGetUpdatedSettings";
constexpr char kCallbackOnGetSettingsResponse[] = "onGetSettingsResponse";
constexpr char kCallbackOnRcsServiceStatusUpdate[] = "onRcsServiceStatusUpdate";
constexpr char kCallbackOnAutoConfigurationReceived[] = "onAutoConfigurationReceived";
constexpr char kCallbackOnAutoConfigErrorSipResponse[] = "onAutoConfigErrorSipResponse";
constexpr char kCallbackOnCommandStatus_1_1[] = "onCommandStatus_1_1";
constexpr char kCallbackOnGetUpdatedSettings_1_1[] = "onGetUpdatedSettings_1_1";
constexpr char kCallbackOnGetSettingsResponse_1_1[] = "onGetSettingsResponse_1_1";

void loadProperties();
#endif
