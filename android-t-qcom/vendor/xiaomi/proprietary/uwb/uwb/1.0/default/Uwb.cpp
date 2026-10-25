// FIXME: your file license if you have one

#include <log/log.h>
#include <hardware/hardware.h>
#include "hal_nxpuwb.h"
#include "Uwb.h"
#include "phNxpUciHal_Adaptation.h"
#include "phUwbStatus.h"

#define CHK_STATUS(x) ((x) == UWBSTATUS_SUCCESS) \
    ? (V1_0::UwbStatus::OK) : (V1_0::UwbStatus::FAILED)

namespace vendor::xiaomi::uwb::implementation {

using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
sp<IUwbClientCallback> Uwb::mCallback = nullptr;
Uwb::Uwb() {}

// Methods from ::vendor::xiaomi::uwb::V1_0::IUwb follow.
Return<::vendor::xiaomi::uwb::V1_0::UwbStatus> Uwb::open(const sp<::vendor::xiaomi::uwb::V1_0::IUwbClientCallback>& clientCallback) {
    // TODO implement
    mCallback = clientCallback;
    ALOGD("HIDL-HAL open ......");
    if (mCallback == nullptr) {
        ALOGE("HIDL-HAL open mCallback is null......");
        return V1_0::UwbStatus::FAILED;
    }
    mCallback->linkToDeath(this, 0 /*cookie*/);
    int ret = phNxpUciHal_open(eventCallback, dataCallback);
    ALOGD("HIDL-HAL Exit......");
    return ret == 0 ? V1_0::UwbStatus::OK : V1_0::UwbStatus::FAILED;
}

Return<uint32_t> Uwb::write(const hidl_vec<uint8_t>& data) {
    // TODO implement
    ALOGD("HIDL-HAL write enter......");
    hidl_vec<uint8_t> copy = data;
    return phNxpUciHal_write(data.size(), &copy[0]);
}

Return<::vendor::xiaomi::uwb::V1_0::UwbStatus> Uwb::close() {
    // TODO implement
    ALOGD("HIDL-HAL close enter......");
    if (mCallback == nullptr) {
        return V1_0::UwbStatus::FAILED;
    }
    int status = phNxpUciHal_close();



    if (mCallback != nullptr) {
        mCallback->unlinkToDeath(this);
        mCallback = nullptr;
    }
    return CHK_STATUS(status);
}

Return<void> Uwb::ioctl(uint64_t ioctlType, const hidl_vec<uint8_t>& inputData, ioctl_cb _hidl_cb) {
    // TODO implement
    int status = 0;
    uwb_uci_IoctlInOutData_t inpOutData;
    ALOGD("HIDL-HAL ioctl enter......");
    uwb_uci_IoctlInOutData_t *pInOutData=(uwb_uci_IoctlInOutData_t*)&inputData[0];

    /*data from proxy->stub is copied to local data which can be updated by

     * underlying HAL implementation since its an inout argument*/
    memcpy(&inpOutData,pInOutData,sizeof(uwb_uci_IoctlInOutData_t));

    status = phNxpUciHal_ioctl(ioctlType, &inpOutData);

    /*copy data and additional fields indicating status of ioctl operation
     * and context of the caller. Then invoke the corresponding proxy callback*/
    inpOutData.out.ioctlType = ioctlType;
    inpOutData.out.context   = pInOutData->inp.context;
    inpOutData.out.result    = status;
    V1_0::UwbData  outputData;
    outputData.setToExternal((uint8_t*)&inpOutData.out, sizeof(uwb_uci_ExtnOutputData_t));
    _hidl_cb(outputData);
    return Void();
}

Return<void> Uwb::getConfig(getConfig_cb _hidl_cb) {
    // TODO implement
    V1_0::UwbConfig uwbVendorConfig;
    ALOGD("HIDL-HAL getConfig enter......");
    phNxpUciHal_getVendorConfig(uwbVendorConfig);
    _hidl_cb(uwbVendorConfig);
    return Void();
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//IUwb* HIDL_FETCH_IUwb(const char* /* name */) {
    //return new Uwb();
//}
//
}  // namespace vendor::xiaomi::hardware::uwb::implementation
