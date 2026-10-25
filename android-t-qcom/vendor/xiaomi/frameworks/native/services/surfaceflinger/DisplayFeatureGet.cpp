/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// #define LOG_NDEBUG 0

#include <cutils/properties.h>
#include <log/log.h>

#include <utils/StrongPointer.h>
#include <utils/Mutex.h>
#include "DisplayFeatureGet.h"

#include <vendor/xiaomi/hardware/displayfeature/1.0/IDisplayFeature.h>
#include <vendor/xiaomi/hardware/displayfeature/1.0/types.h>

using vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature;
using android::hidl::base::V1_0::IBase;
using android::hardware::hidl_death_recipient;


namespace android {

const sp<IDisplayFeature> DisplayFeatureGet::getDisplayFeatureService()
{
    Mutex::Autolock autoLock(mServiceLock);
    bool displayfeatureRun = false;
    char propertyValue[PROPERTY_VALUE_MAX] = {0};

    if (mDisplayFeatureService.get() == NULL) {
        if (property_get(DISPLAYFEATURE_HAL_STATUS_PROP, propertyValue, NULL) > 0) {
            if (!(strncmp(propertyValue, "running", PROPERTY_VALUE_MAX))) {
                displayfeatureRun = true;
            }
        }

        if (displayfeatureRun) {
            ALOGD("get displayfeature service!");
            mDisplayFeatureService = IDisplayFeature::getService();
            if (mDisplayFeatureService.get() == NULL) {
                ALOGE("Query DisplayFeature service returned null!");
                return mDisplayFeatureService;
            }

            mDisplayFeatureService->linkToDeath(this, /*cookie*/1);
        }
    }

    return mDisplayFeatureService;
}

void DisplayFeatureGet::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/)
{
    Mutex::Autolock autoLock(mServiceLock);

    ALOGW("receive the notification, displayfeaturehal service die!");
    if (mDisplayFeatureService.get()) {
        mDisplayFeatureService->unlinkToDeath(this);
        mDisplayFeatureService.clear();
        ALOGD("clean displayfeaturehal handle!");
    }
}

bool DisplayFeatureGet::setFeature(int32_t display, int32_t cmd, int32_t param, int32_t cookie) {
    bool ret = true;
    if (getDisplayFeatureService().get() != NULL) {
        if(!mDisplayFeatureService->setFeature(display, cmd, param, cookie).isOk()) {
            ALOGE("Failed to setFeature to DisplayFeature");
            ret = false;
        }
    }
    return ret;
}

void DisplayFeatureGet::registerCallback(int32_t display, const sp<IDisplayFeatureCallback>& callback)
{
    if (getDisplayFeatureService().get() != NULL) {
        mDisplayFeatureService->registerCallback(display, callback);
    }
}

void DisplayFeatureGet::setDisplayFeatureGameMode(int gameColorMode) {
    ALOGD("%s gamemode=%d", __func__, gameColorMode);
    setFeature(0, (int32_t)FeatureId::GAME_STATE, gameColorMode, 0);
}

int32_t DisplayFeatureGet::setDisplayParam(int32_t param)
{
    FILE* fp;
    char writeString[STRING_LEN] = {0};
    ALOGD("enter");

    if (param == -1) {
        ALOGE("invalid param");
        return 0;
    }

    if (snprintf(writeString, sizeof(writeString), "0x%x", param) < 0) {
        ALOGE("error print!");
        return -1;
    }

    fp = fopen(kDISP_PARAM_PATH, "w");
    if (fp == NULL) {
        ALOGE("open file error: %s", kDISP_PARAM_PATH);
        return -1;
    }

    if (fputs(writeString, fp) < 0) {
        ALOGE("write file error!");
        fclose(fp);
        return -1;
    }

    ALOGD("successfully write %s into %s", writeString, kDISP_PARAM_PATH);

    fclose(fp);
    return 0;
}

int32_t DisplayFeatureGet::setFODFlag(bool success)
{
    int32_t ret = -1;

    if (success) {
        ret = setDisplayParam(DISPPARAM_UNLOCK_SUCCESS);
    } else {
        ret = setDisplayParam(DISPPARAM_UNLOCK_FAIL);
    }
    return ret;
}

int32_t DisplayFeatureGet::setFingerprintState(int fp_status)
{
    int32_t ret = -1;
    int32_t fodstatus = DISPPARAM_FP_STATUS | fp_status;

    ret = setDisplayParam(fodstatus);
    return ret;
}

void DisplayFeatureGet::Dump(std::string& result)
{
    std::string cb_result;
    bool displayfeature_Dump = false;

    if (getDisplayFeatureService().get() != NULL) {
        displayfeature_Dump = property_get_bool(DISPLAYFEATURE_DUMP_ENABLE_PROP, false);
        if(displayfeature_Dump) {
            if (mDisplayFeatureService.get() != NULL) {
                mDisplayFeatureService->dumpDebugInfo([&](const auto& tmp_res) {
                    cb_result = tmp_res;
                });
            }
        } else {
            ALOGD("%s no support dump displayfeature", __func__);
        }
    }
    result.append(cb_result);
}

}
