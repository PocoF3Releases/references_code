/*
 * Copyright (c) 2020 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#pragma once

#include <mutex>

#include <vendor/xiaomi/hardware/cameraperf/1.0/IMiCameraPerfService.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor::xiaomi::hardware::cameraperf::implementation {

using ::android::hardware::hidl_death_recipient;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::wp;

struct MiCameraPerfClientDeathHandler;

struct MiCameraPerfService : public V1_0::IMiCameraPerfService {
    // Methods from ::vendor::xiaomi::hardware::cameraperf::V1_0::IMiCameraPerfService follow.
public:
    MiCameraPerfService();

    Return<void> releaseAllSensorCache(int32_t mode) override;
    Return<int32_t> enableReleaseSensorCache(const sp<::android::hidl::base::V1_0::IBase>& callback) override;

private:
    struct service_data {
        ::android::sp<IBase> event_cb;
        ::android::sp<MiCameraPerfClientDeathHandler> mDeathRecipient;
    };

    service_data mData;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

struct MiCameraPerfClientDeathHandler : hidl_death_recipient {
public:
    MiCameraPerfClientDeathHandler(MiCameraPerfService *service) { mMiCameraPerfService = service; };
    ~MiCameraPerfClientDeathHandler()
    {
        mMiCameraPerfService.clear();
        mMiCameraPerfService = NULL;
    };

    virtual void serviceDied(uint64_t /*cookie*/, const wp<::android::hidl::base::V1_0::IBase>& /*who*/);
private:
    std::mutex                  mServiceLock;
    sp<MiCameraPerfService>    mMiCameraPerfService;
};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IMiCameraPerfService* HIDL_FETCH_IMiCameraPerfService(const char* name);

}  // namespace vendor::xiaomi::hardware::cameraperf::implementation
