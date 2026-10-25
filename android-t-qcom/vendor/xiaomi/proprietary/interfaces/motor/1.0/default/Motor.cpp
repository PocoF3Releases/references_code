#include "Motor.h"
#include <utils/Log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "motor_impl"

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace motor {
namespace V1_0 {
namespace implementation {

/*sp<IMotorCallback> Motor::mCallback = nullptr;*/

// Methods from ::vendor::xiaomi::hardware::motor::V1_0::IMotor follow.

Motor::Motor()
{
    mModule = new MotorModule();
}

Motor::~Motor() {}


Return<void> Motor::init(){
    mExit = false;

    mModule->initialize();

    return Void();
}

Return<void> Motor::release(){
    mExit = true;

    mModule->release();

    return Void();
}

Return<void> Motor::popupMotor(uint32_t cookie) {
    // TODO implement
    ALOGE("popupMotor------cookie=%d",cookie);

    mModule->pop(cookie);

    return Void();
}

Return<void> Motor::takebackMotor(uint32_t cookie) {
    // TODO implement
    ALOGE("takebackMotor-----cookie=%d",cookie);

    mModule->retract(cookie);

    return Void();
}

Return<void> Motor::setMotorCallback(const sp<::vendor::xiaomi::hardware::motor::V1_0::IMotorCallback>& motorcallback) {
    // TODO implement
    if(motorcallback != nullptr){
       mModule->setCallback(motorcallback);
       ALOGD("setMotorCallback: done");
    }
    return Void();
}

Return<uint32_t> Motor::getMotorStatus(){
    ALOGE("getMotorStatus()");
    return mModule->getMotorStatus();
}

Return<void> Motor::calibration(){
    ALOGE("calibration()");
    mModule->calibration();
    return Void();

}

Return<void> Motor::takebackMotorShortly(){
    ALOGE("takebackMotorShortly()");
    mModule->falldown();
    return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

//IMotor* HIDL_FETCH_IMotor(const char* /* name */) {
    //return new Motor();
//}
//
}  // namespace implementation
}  // namespace V1_0
}  // namespace motor
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor
