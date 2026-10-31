/*
 * Copyright (c) 2017 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#define LOG_TAG "DisplayFeature-Hal"
#include "DisplayFeature.h"
#include <utils/Log.h>
#include <cutils/properties.h>
/*#include <QServiceUtils.h>*/

using std::string;

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace displayfeature {
namespace V1_0 {
namespace implementation {

sp<IDisplayFeatureCallback> DisplayFeature::callbacks_[MAX_CALLBACK_CNT] = {NULL};
std::mutex DisplayFeature::callbacks_lock_;

DisplayFeature::DisplayFeature(const hw_module_t* module)
{
    ALOGD("HW DisplayFeature contruct");
    hw_device_t* device = nullptr;
    int error = module->methods->open(module, DF_DISPLAYFEATURE, &device);
    if (error != 0) {
        ALOGE("Failed to open DisplayFeature device (%s)", strerror(-error));
    }
    ALOGD("device->version 0x%x", device->version);
    uint32_t majorVersion = (device->version >> 24) & 0xF;
    uint32_t minorVersion = device->version & HARDWARE_API_VERSION_2_MAJ_MIN_MASK;
    minorVersion = (minorVersion >> 16) & 0xF;
    ALOGI("Found DF implementation v%d.%d", majorVersion, minorVersion);

    mDevice = reinterpret_cast<df_device_t*>(device);

    initCapabilities();
    initDispatch();
}

DisplayFeature::~DisplayFeature()
{
    ALOGD("HW DisplayFeature destroy");
    displayfeature_close(mDevice);
}

void DisplayFeature::initCapabilities()
{
}

template<typename T>
void DisplayFeature::initDispatch(df_function_descriptor_t desc, T* outPfn)
{
    ALOGD("initDispatch df_function_descriptor_t outPfn");
    auto pfn = mDevice->getFunction(mDevice, desc);
    if (!pfn) {
        if (desc == DF_FUNCTION_SET_LISTENER) {
             *outPfn = 0;
             return;
        } else {
            LOG_ALWAYS_FATAL("failed to get displayfeature function %d", desc);
        }
    }

    *outPfn = reinterpret_cast<T>(pfn);
}

void DisplayFeature::initDispatch()
{
    ALOGD("initDispatch");
    initDispatch(DF_FUNCTION_SET_FEATURE,
         &mDispatch.setFeature);
    initDispatch(DF_FUNCTION_SET_FUNCTION,
         &mDispatch.setFunction);
    initDispatch(DF_FUNCTION_SEND_MESSAGE,
         &mDispatch.sendMessage);
    initDispatch(DF_FUNCTION_SET_LISTENER,
         &mDispatch.setListener);
    if(property_get_bool(DISPLAYFEATURE_DUMP_ENABLE_PROP, false)) {
        initDispatch(DF_FUNCTION_DUMP,
         &mDispatch.dump);
    }
}

// Methods from ::vendor::xiaomi::hardware::displayfeature::V1_0::IDisplayFeature follow.
Return<Status> DisplayFeature::sendPanelCommand(const hidl_string& cmd)
{
    // TODO implement
    ALOGD("sendPanelCommand %s", cmd.c_str());
    return ::vendor::xiaomi::hardware::displayfeature::V1_0::Status::OK;
}

Return<Status> DisplayFeature::sendRefreshCommand()
{
    // TODO implement
    ALOGD("sendRefreshCommand trigger");
/*
    sp<qService::IQService> iqservice;

    iqservice = android::interface_cast<qService::IQService>(android::defaultServiceManager()->getService(android::String16("display.qservice")));
    if (iqservice.get()) {
        iqservice->dispatch(qService::IQService::SCREEN_REFRESH, NULL, NULL);
    } else {
        ALOGW("Failed to acquire %s", "display.qservice");
        return ::vendor::xiaomi::hardware::displayfeature::V1_0::Status::INVALID_OPERATION;
    }
*/
    return ::vendor::xiaomi::hardware::displayfeature::V1_0::Status::OK;
}

Return<void> DisplayFeature::notifyBrightness(int32_t brightness)
{
    // TODO implement
    ALOGV("notifyBrightness %d", brightness);
    static string cmd = "brightness";
    sendMessage((int32_t)FeatureId::NOTIFY_BRIGHTNESS_STATE, brightness, cmd);

    return Void();
}

Return<void> DisplayFeature::sendMessage(int32_t index, int32_t value, const hidl_string& cmd)
{
    // TODO implement
    ALOGV("sendMessage (%d %d %s)", index, value, cmd.c_str());
    mDispatch.sendMessage(mDevice, index, value, cmd);
    return Void();
}

Return<void> DisplayFeature::dumpDebugInfo(dumpDebugInfo_cb _hidl_cb)
{
    std::string result;
    mDispatch.dump(mDevice, result);
    _hidl_cb(result);

    return Void();
}

Return<Status> DisplayFeature::sendPostProcCommand(int32_t cmd, int32_t value)
{
    // TODO implement
    ALOGD("sendPostProcCommand (%d %d)", cmd, value);
    return ::vendor::xiaomi::hardware::displayfeature::V1_0::Status::OK;
}

Return<Status> DisplayFeature::setFeature(int32_t displayId, int32_t caseId, int32_t modeId, int32_t cookie)
{
    // TODO implement
    ALOGV("setFeature (%d %d %d %d)", displayId, caseId, modeId, cookie);
    mDispatch.setFeature(mDevice, displayId, caseId, modeId, cookie);

    return ::vendor::xiaomi::hardware::displayfeature::V1_0::Status::OK;
}

Return<Status> DisplayFeature::setFunction(int32_t displayId, int32_t caseId, int32_t modeId, int32_t cookie)
{
    // TODO implement
    ALOGD("setFunction (%d %d %d %d)", displayId, caseId, modeId, cookie);
    mDispatch.setFunction(mDevice, displayId, caseId, modeId, cookie);
    return ::vendor::xiaomi::hardware::displayfeature::V1_0::Status::OK;
}

Return<Status> DisplayFeature::registerCallback(int32_t displayId, const sp<IDisplayFeatureCallback>& callback)
{
    if (callback == nullptr) {
        return ::vendor::xiaomi::hardware::displayfeature::V1_0::Status::OK;
    }

    {
        std::lock_guard<std::mutex> _lock(DisplayFeature::callbacks_lock_);
        for (int i = 0; i < MAX_CALLBACK_CNT; i++) {
            if (DisplayFeature::callbacks_[i] == NULL) {
                DisplayFeature::callbacks_[i] = callback;
                ALOGD("callback register to %d", i);
                break;
            }
        }
    }

    auto linkRet = callback->linkToDeath(this, 0u /* cookie */);

    if(mDispatch.setListener) {
        mDispatch.setListener(mDevice, notifyListeners);
    }

    return ::vendor::xiaomi::hardware::displayfeature::V1_0::Status::OK;
}

void DisplayFeature::serviceDied(uint64_t /* cookie */, const wp<IBase>& who)
{
    std::lock_guard<std::mutex> _lock(callbacks_lock_);

    sp<IBase> callback = who.promote();
    if (callback != NULL) {
        (void)callback->unlinkToDeath(this).isOk();
    }
    for (int i = 0; i < MAX_CALLBACK_CNT; i++) {
        if (callbacks_[i] != NULL) {
            if (callbacks_[i].get() && callbacks_[i] == callback) {
                ALOGE("callback i %d died", i);
                callbacks_[i].clear();
                if (callbacks_[i] != NULL)
                    callbacks_[i] = NULL;
                break;
            }
        }
    }
    ALOGD("the service that register callback to displayfeature is died,");
}

void DisplayFeature::notifyListeners(int32_t displayId, int32_t value, float red, float green, float blue)
{
    ALOGV("notifyListeners, id:%d, value:%d", displayId, value);
    std::lock_guard<std::mutex> _lock(callbacks_lock_);
    for (int i = 0; i < MAX_CALLBACK_CNT; i++) {
        if (callbacks_[i] != NULL) {
            if (callbacks_[i].get()) {
                auto ret = callbacks_[i]->displayfeatureInfoChanged(displayId, value, red, green, blue);
                if(!ret.isOk()) {
                    ALOGD("failed to callback_->displayfeatureInfoChanged: %s. likely unavailable.",
                            ret.description().c_str());
                    if (ret.isDeadObject()) {
                        callbacks_[i].clear();
                    }
                }
            }
        }
    }
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

IDisplayFeature* HIDL_FETCH_IDisplayFeature(const char* /* name */) {
    const hw_module_t* module = nullptr;
    int err = hw_get_module(DISPLAYFEATURE_HARDWARE_MODULE_ID, &module);
    if (err) {
        ALOGE("failed to get hw displayfeature module");
        return nullptr;
    }

    return new DisplayFeature(module);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace displayfeature
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor
