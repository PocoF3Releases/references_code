/*
 * Copyright (c) 2020 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include "MiCameraPerfServiceIntf.h"
#include "MiCameraPerfService.h"
#include <hidl/HidlTransportSupport.h>
#include <utils/Log.h>

#include <vendor/xiaomi/hardware/cameraperf/1.0/IMiCameraPerfService.h>

using android::hardware::configureRpcThreadpool;
using vendor::xiaomi::hardware::cameraperf::V1_0::IMiCameraPerfService;
using vendor::xiaomi::hardware::cameraperf::implementation::MiCameraPerfService;

using android::sp;
using android::status_t;
using android::OK;

const uint32_t NumberOfThreadsNeeded = 2;

////////////////////////////////////////////////////////////////////////////////////////
/// RegisterIMiCameraPerfService
////////////////////////////////////////////////////////////////////////////////////////
void RegisterIMiCameraPerfService()
{
    static android::sp<IMiCameraPerfService> s_pIMiCameraPerfService = NULL;

    if (NULL == s_pIMiCameraPerfService)
    {
        ALOGI("micameraperformance register started");
        s_pIMiCameraPerfService = new MiCameraPerfService();

        status_t status = s_pIMiCameraPerfService->registerAsService();

        if (android::OK != status)
        {
            ALOGE("micameraperformance register failed, status: %u", status);
        }
        else
        {
            ALOGI("micameraperformance register successfully");
            configureRpcThreadpool(NumberOfThreadsNeeded, true);
        }
    }
}