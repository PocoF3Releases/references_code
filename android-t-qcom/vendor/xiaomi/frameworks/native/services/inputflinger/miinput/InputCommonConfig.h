#ifndef INPUT_COMMON_CONFIG_H
#define INPUT_COMMON_CONFIG_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <stdint.h>
#include <utils/RefBase.h>
#include <errno.h>
#include <binder/Parcel.h>
#include <private/gui/ParcelUtils.h>
#include <cutils/properties.h>
#include <android-base/strings.h>

// MIUI ADD:
namespace android {

enum InputCommonChanged {
    //双击唤醒，老机型的
    CONFIG_TOUCH_WAKEUP = 0x01,
    //多屏协同模式
    CONFIG_SYNERGY_MODE = 0x02,
    //注入事件状态
    CONFIG_INJECTEVENT_STATUS = 0x04,
    // PAD键盘自定义快捷键(可复写,该功能已弃用)
    CONFIG_ENABLE_KS_STATUS = 0x08,
    CONFIG_INPUTMETHOD_STATUS = 0x10,
    CONFIG_PAD_MODE = 0x20,
    //鼠标自然滚动
    CONFIG_MOUSE_NATURAL_SCROLL = 0x40,
    //Miui Keyboard Info
    CONFIG_MIUI_KEYBOARD_INFO = 0x80,
    //事件oneway传递
    CONFIG_ONEWAY_MODE = 0x0100,
};


class InputCommonConfig : public RefBase
{
public:
    static status_t readFromParcel(const Parcel& parcel, sp<InputCommonConfig> inputCommonConfig);
    virtual void dump(std::string& dump);
    InputCommonConfig(uint32_t touchWakeupMode, int32_t synergyMode);

    uint32_t mTouchWakeupMode;

    int32_t mSynergyMode;

    bool mInjectEventStatus = false;
    bool mRecordEventStatus = false;

    bool mInputMethodShown;
    bool mCustomizeInputMethod;
    bool mPadMode;
    bool mMouseNaturalScroll;
    int32_t mMiuiKeyboardProductId;
    int32_t mMiuiKeyboardVendorId;

    bool mOnewayMode = false;

private:

};

} // namespace android

#endif // INPUT_COMMON_CONFIG_H