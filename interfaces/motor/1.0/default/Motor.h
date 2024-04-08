#ifndef VENDOR_XIAOMI_HARDWARE_MOTOR_V1_0_MOTOR_H
#define VENDOR_XIAOMI_HARDWARE_MOTOR_V1_0_MOTOR_H

#include <vendor/xiaomi/hardware/motor/1.0/IMotor.h>
#include <vendor/xiaomi/hardware/motor/1.0/IMotorCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/threads.h>
#include <MotorModule.h>


namespace vendor {
namespace xiaomi {
namespace hardware {
namespace motor {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;
using ::android::vendor::xiaomi::hardware::motor::V1_0::helper::MotorModule;



class Motor : public IMotor{
    // Methods from ::vendor::xiaomi::hardware::motor::V1_0::IMotor follow.
    public:
    Motor();
    ~Motor();

    bool mExit ;
    Return<void> init();
    Return<void> release();
    Return<void> popupMotor(uint32_t cookie);
    Return<void> takebackMotor(uint32_t cookie) override;
    Return<void> setMotorCallback(const sp<::vendor::xiaomi::hardware::motor::V1_0::IMotorCallback>& motorcallback) override;
    Return<uint32_t> getMotorStatus() override;
    Return<void> calibration() override;
    Return<void> takebackMotorShortly() override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

private:
    sp<MotorModule> mModule;

};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IMotor* HIDL_FETCH_IMotor(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace motor
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor

#endif  // VENDOR_XIAOMI_HARDWARE_MOTOR_V1_0_MOTOR_H
