/*
 * Copyright (c) 2020 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include "MiCameraPerfService.h"

#include <dlfcn.h>
#include <utils/Log.h>

namespace vendor::xiaomi::hardware::cameraperf::implementation {
// Methods from ::vendor::xiaomi::hardware::cameraperf::V1_0::IMiCameraPerfService follow.
#define QCOM_HAL_LIBRARY "/vendor/lib64/hw/camera.qcom.so"
typedef void (*ReleaseAllSensorCache)(int);
ReleaseAllSensorCache pReleaseAllSensorCache = NULL;

MiCameraPerfService::MiCameraPerfService() {
    void* pLibHandle = dlopen(QCOM_HAL_LIBRARY, RTLD_NOW);

    if (NULL != pLibHandle)
    {
        pReleaseAllSensorCache = (ReleaseAllSensorCache)dlsym(pLibHandle, "release_all_sensor_cache");
        if (NULL == pReleaseAllSensorCache)
        {
            ALOGE("%s function pointers are NULL, dlsym failed, %p", __FUNCTION__, pReleaseAllSensorCache);
        }
    }
    else
    {
        char const *err_str = dlerror();
        ALOGE("load: module=%s\n%s", QCOM_HAL_LIBRARY, err_str ? err_str : "unknown");
        return;
    }
}

Return<void> MiCameraPerfService::releaseAllSensorCache(int32_t mode) {
    // TODO implement
    pReleaseAllSensorCache(mode);
    if ((NULL != mData.event_cb) && (NULL != mData.mDeathRecipient))
    {
        mData.event_cb->unlinkToDeath(mData.mDeathRecipient);
    }
    ALOGE("%s: unlinkToDeath", __func__);
    return Void();
}

Return<int32_t> MiCameraPerfService::enableReleaseSensorCache(const sp<::android::hidl::base::V1_0::IBase>& callback) {
    // TODO implement
    int32_t ret = -1;
    mData.mDeathRecipient = new MiCameraPerfClientDeathHandler(this);
    if (NULL == mData.mDeathRecipient)
    {
        ALOGE("%s: mDeathRecipient is NULL", __func__);
        return ret;
    }
    mData.event_cb = callback;

    if (NULL == mData.event_cb)
    {
        ALOGE("%s: mData.event_cb is NULL", __func__);
    }

    mData.event_cb->linkToDeath(mData.mDeathRecipient, 0);
    releaseAllSensorCache(1);
    ret = 1;
    ALOGE("%s: linkToDeath", __func__);

    return ret;
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

//IMiCameraPerfService* HIDL_FETCH_IMiCameraPerfService(const char* /* name */) {
    //return new MiCameraPerfService();
//}
//

/**
 * MiCameraPerfClientDeathHandler
 */
void MiCameraPerfClientDeathHandler::serviceDied(uint64_t /*cookie*/, const wp<::android::hidl::base::V1_0::IBase>& /*who*/)
{
    std::lock_guard<std::mutex> lock(mServiceLock);
    mMiCameraPerfService->releaseAllSensorCache(0);
    ALOGE("Phoenix test binderDied");
}

}  // namespace vendor::xiaomi::hardware::cameraperf::implementation
