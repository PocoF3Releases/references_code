/*
 * Copyright (C) Xiaomi Inc.
 *
 */
#include <vendor/xiaomi/hardware/cld/1.0/ICld.h>
#include "Cld.h"

using android::hardware::Return;
using android::hardware::Void;
using vendor::xiaomi::hardware::cld::V1_0::ICld;
using vendor::xiaomi::hardware::cld::V1_0::FragmentLevel;
using vendor::xiaomi::hardware::cld::V1_0::CldOperationStatus;
using vendor::xiaomi::hardware::cld::V1_0::ICldCallback;

namespace android {
namespace vold {

class CldCallback : public ICldCallback {
  public:
    Return<void> notifyStatusChange(FragmentLevel level) override {
        android::os::PersistableBundle extras;
        extras.putInt(String16("frag_level"), static_cast<int>(level));
        if (mCldListener) mCldListener->onFinished(0, extras);
        LOG(DEBUG) << "Stop Cld with level " << static_cast<int>(level);
        return Void();
    }

    void setCldListener(const android::sp<android::os::IVoldTaskListener>& listener) {
        mCldListener = listener;
    }

  private:
    android::sp<android::os::IVoldTaskListener> mCldListener;
};

static android::sp<CldCallback> cb;

void miuiSetCldListener(const android::sp<android::os::IVoldTaskListener>& listener) {
    auto service = ICld::getService();
    if (!service || !service->isCldSupported()) {
        LOG(WARNING) << "CLD is not supported on current device!";
        return;
    }

    if (!cb) {
        cb = new CldCallback();
        cb->setCldListener(listener);
        service->registerCallback(cb);
    }
}

/**
 * Trigger CLD if needed.
 * This function returns immediately and the result of CLD
 * will be post to storageManagerService by CldCallback when
 * CLD is done.
 * @return current fragment level
 */
int miuiRunCldOnHal() {
    auto service = ICld::getService();
    if (!service || !service->isCldSupported()) {
        LOG(WARNING) << "CLD is not supported on current device!";
        return CLD_NOTSUPPORT;
    }

    FragmentLevel level = service->getFragmentLevel();
    if (level != FragmentLevel::FRAG_LEVEL_MEDIUM && level != FragmentLevel::FRAG_LEVEL_SEVERE) {
        LOG(DEBUG) << "Frag level: " << static_cast<int>(level) << ", no need for cld";
        return static_cast<int>(level);
    }

    // trigger cld!
    LOG(DEBUG) << "Start Cld on HAL with level " << static_cast<int>(level);
    service->triggerCld(1);

    return static_cast<int>(level);
}

int miuiGetFragLevelHal() {
    auto service = ICld::getService();
    if (!service || !service->isCldSupported()) {
        LOG(WARNING) << "CLD is not supported on current device!";
        return CLD_NOTSUPPORT;
    }
    FragmentLevel level = service->getFragmentLevel();
    if (level < FragmentLevel::FRAG_ANALYSIS || level > FragmentLevel::FRAG_UNKNOWN) {
        level = FragmentLevel::FRAG_UNKNOWN;
    }
    return static_cast<int>(level);
}

} // namespace vold
} // namespace android