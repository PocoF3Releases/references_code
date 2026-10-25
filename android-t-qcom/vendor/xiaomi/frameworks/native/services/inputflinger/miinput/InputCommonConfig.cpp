#define LOG_TAG "InputCommonConfig"

#include "InputCommonConfig.h"
#include <android-base/stringprintf.h>

#define INDENT2 "    "

using android::base::StringPrintf;

// MIUI ADD:
namespace android {

// static inline const char* toString(bool value) {
//     return value ? "true" : "false";
// }

InputCommonConfig::InputCommonConfig(uint32_t touchWakeupMode, int32_t synergyMode)
      : mTouchWakeupMode(touchWakeupMode), mSynergyMode(synergyMode) {}

status_t InputCommonConfig::readFromParcel(const Parcel& parcel,
                                           sp<InputCommonConfig> inputCommonConfig) {
    uint32_t touchWakeUpMode;
    int32_t synergyMode;
    bool injectEventStatus;
    bool recordEventStatus;
    bool inputMethodShown;
    bool customizeInputMethod;
    bool padMode;
    bool mouseNaturalScroll;
    int32_t miuiKeyboardProductId;
    int32_t miuiKeyboardVendorId;
    bool onewayMode = false;
    SAFE_PARCEL(parcel.readUint32, &touchWakeUpMode);
    SAFE_PARCEL(parcel.readInt32, &synergyMode);
    SAFE_PARCEL(parcel.readBool, &injectEventStatus);
    SAFE_PARCEL(parcel.readBool, &recordEventStatus);
    SAFE_PARCEL(parcel.readBool, &inputMethodShown);
    SAFE_PARCEL(parcel.readBool, &customizeInputMethod);
    SAFE_PARCEL(parcel.readBool, &padMode);
    SAFE_PARCEL(parcel.readBool, &mouseNaturalScroll);
    SAFE_PARCEL(parcel.readInt32, &miuiKeyboardProductId);
    SAFE_PARCEL(parcel.readInt32, &miuiKeyboardVendorId);
    SAFE_PARCEL(parcel.readBool, &onewayMode);

    int32_t result = 0;
    if (inputCommonConfig->mTouchWakeupMode != touchWakeUpMode) {
        result |= CONFIG_TOUCH_WAKEUP;
    }
    if (inputCommonConfig->mSynergyMode != synergyMode) {
        result |= CONFIG_SYNERGY_MODE;
    }
    if(inputCommonConfig->mInjectEventStatus != injectEventStatus){
        result |= CONFIG_INJECTEVENT_STATUS;
    }
    if (inputCommonConfig->mInputMethodShown != inputMethodShown) {
        result |= CONFIG_INPUTMETHOD_STATUS;
    }
    if (inputCommonConfig->mPadMode != padMode) {
        result |= CONFIG_PAD_MODE;
    }
    if(inputCommonConfig->mMouseNaturalScroll != mouseNaturalScroll) {
        result |= CONFIG_MOUSE_NATURAL_SCROLL;
    }
    if(inputCommonConfig->mMiuiKeyboardProductId != miuiKeyboardProductId ||
            inputCommonConfig->mMiuiKeyboardVendorId != miuiKeyboardVendorId) {
        result |= CONFIG_MIUI_KEYBOARD_INFO;
    }
    if (inputCommonConfig->mOnewayMode != onewayMode) {
        result |= CONFIG_ONEWAY_MODE;
    }
    inputCommonConfig->mTouchWakeupMode = touchWakeUpMode;
    inputCommonConfig->mSynergyMode = synergyMode;
    inputCommonConfig->mInjectEventStatus = injectEventStatus;
    inputCommonConfig->mRecordEventStatus = recordEventStatus;
    inputCommonConfig->mInputMethodShown = inputMethodShown;
    inputCommonConfig->mCustomizeInputMethod = customizeInputMethod;
    inputCommonConfig->mPadMode = padMode;
    inputCommonConfig->mMouseNaturalScroll = mouseNaturalScroll;
    inputCommonConfig->mMiuiKeyboardProductId = miuiKeyboardProductId;
    inputCommonConfig->mMiuiKeyboardVendorId = miuiKeyboardVendorId;
    inputCommonConfig->mOnewayMode = onewayMode;
    return result;
}


void InputCommonConfig::dump(std::string& dump) {
    dump += "\nInputCommonConfig State\n";
    dump += StringPrintf(INDENT2 "mTouchWakeupMode: %d\n", mTouchWakeupMode);
    dump += StringPrintf(INDENT2 "mSynergyMode: %d\n", mSynergyMode);
    dump += StringPrintf(INDENT2 "mInjectEventStatus: %d\n", mInjectEventStatus);
    dump += StringPrintf(INDENT2 "mRecordEventStatus: %d\n", mRecordEventStatus);
    dump += StringPrintf(INDENT2 "mInputMethodShown: %d\n", mInputMethodShown);
    dump += StringPrintf(INDENT2 "mCustomizeInputMethod: %d\n", mCustomizeInputMethod);
    dump += StringPrintf(INDENT2 "mPadMode: %d\n", mPadMode);
    dump += StringPrintf(INDENT2 "mMouseNaturalScroll: %d\n", mMouseNaturalScroll);
    dump += StringPrintf(INDENT2 "mMiuiKeyboardProductId: %d\n", mMiuiKeyboardProductId);
    dump += StringPrintf(INDENT2 "mMiuiKeyboardVendorId: %d\n", mMiuiKeyboardVendorId);
    dump += StringPrintf(INDENT2 "mOnewayMode: %d\n", mOnewayMode);
}

} // namespace android
