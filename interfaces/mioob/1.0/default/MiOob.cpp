#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MIOOB"

#include <log/log.h>
#include <stdio.h>
#include <unistd.h>
#include <cutils/properties.h>
#include "MiOob.h"

#define N_ELEMENTS(e) (sizeof(e) / sizeof((e)[0]))
static const char *WIRELESS_SUPPORT_ADSP_LIST[] = {
    // 8350
    "venus",
    "star",
    "mars",
};


namespace vendor {
namespace xiaomi {
namespace hardware {
namespace mioob {
namespace V1_0 {
namespace implementation {

const hidl_string MIOOB_BT_STATE_FILE_NODE = "/sys/class/power_supply/wireless/bt_state";
const hidl_string MIOOB_RX_CR_FILE_NODE = "/sys/class/power_supply/wireless/rx_cr";
const hidl_string MIOOB_BT_STATE_FILE_NODE_ADSP = "/sys/class/qcom-battery/bt_state";
const hidl_string MIOOB_RX_CR_FILE_NODE_ADSP = "/sys/class/qcom-battery/rx_cr";

const hidl_string MIOOB_BT_STATE_CONNECTED_STRING = "1";
const hidl_string MIOOB_BT_STATE_DISCONNECTED_STRING = "2";
const hidl_string MIOOB_BT_STATE_CONNECTING_STRING = "3";

MiOob::MiOob() {
    char product_device[PROPERTY_VALUE_MAX] = {0};
    uint32_t pos = 0;

    property_get("ro.product.device", product_device, "");
    for (pos = 0; pos < N_ELEMENTS(WIRELESS_SUPPORT_ADSP_LIST); pos++) {
        if (strcmp(product_device, WIRELESS_SUPPORT_ADSP_LIST[pos]) == 0) {
            mIsADSP = true;
        }
    }
}

// internal functions
int writeToFile(const hidl_string &path, const hidl_string &data){
	int fd = -1;
	int ret = -1;

	fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0){
        ALOGE("Failed to open fileNode:%s", path.c_str());
        return -1;
    }
	ret = write(fd, data.c_str(), data.size());

    if (ret < 0){
        close(fd);
        ALOGE("Failed to write fileNode:%s, data:%s", path.c_str(), data.c_str());
        return -1;
    }

    close(fd);
    ALOGI("Write file success");
    return 0;
}
// Methods from ::vendor::xiaomi::hardware::mioob::V1_0::IMiOob follow.

Return<int32_t> MiOob::setBtState(::vendor::xiaomi::hardware::mioob::V1_0::Bt_State state) {
    // TODO implement
    int ret = -1;
    hidl_string file_node;

    file_node = mIsADSP ? MIOOB_BT_STATE_FILE_NODE_ADSP : MIOOB_BT_STATE_FILE_NODE;
    switch (state){
        case Bt_State::OOB_BT_STATE_CONNECTED:
        ret = writeToFile(file_node, MIOOB_BT_STATE_CONNECTED_STRING);
        break;

        case Bt_State::OOB_BT_STATE_DISCONNECTED:
        ret = writeToFile(file_node, MIOOB_BT_STATE_DISCONNECTED_STRING);
        break;

        case Bt_State::OOB_BT_STATE_CONNECTING:
        ret = writeToFile(file_node, MIOOB_BT_STATE_CONNECTING_STRING);
        break;

        default:
        ALOGE("Unsupported Bt state!");
        break;
    }
    return ret;
}

Return<int32_t> MiOob::setRxCr(const hidl_string& data) {
    // TODO implement
    if (mIsADSP)
        return writeToFile(MIOOB_RX_CR_FILE_NODE_ADSP, data);
    else
        return writeToFile(MIOOB_RX_CR_FILE_NODE, data);
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//IMiOob* HIDL_FETCH_IMiOob(const char* /* name */) {
    //return new MiOob();
//}
//
}  // namespace implementation
}  // namespace V1_0
}  // namespace mioob
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor
