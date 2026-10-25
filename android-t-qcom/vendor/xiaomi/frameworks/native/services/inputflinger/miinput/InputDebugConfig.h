#ifndef INPUT_DEBUG_CONFIG_H
#define INPUT_DEBUG_CONFIG_H

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

/**
 * Motion event flags.
 * @ frameworks/native/include/android/input.h  eg:AMOTION_EVENT_FLAG_WINDOW_IS_OBSCURED
 * 各个大版本需要检查
 */
enum {
    AMOTION_EVENT_FLAG_GESTURE_PENDING            = 0x00010000,
    AMOTION_EVENT_FLAG_GESTURE_BOTTOM             = 0x00020000,
    AMOTION_EVENT_FLAG_GESTURE_LEFT               = 0x00040000,
    AMOTION_EVENT_FLAG_GESTURE_RIGHT              = 0x00080000,
    AMOTION_EVENT_FLAG_GESTURE_NO                 = 0x00100000,
    AMOTION_EVENT_FLAG_DEBUGINPUT_MAJAR           = 0x00200000,
    AMOTION_EVENT_FLAG_DEBUGINPUT_DETAIL          = 0x00400000,
    /*Keep the inject event device id */
    AMOTION_EVENT_FLAG_KEEP_DEVID                 = 0x00800000,
    /*The window obscured by handwriting window*/
    AMOTION_EVENT_FLAG_SKIP_HANDWRITING_WINDOW    = 0x01000000,
    AMOTION_EVENT_FLAG_ONEWAY                     = 0x02000000,
    AMOTION_EVENT_FLAG_DEBUGINPUT_TRANSPORT_ALL   = 0x04000000,
    /*The event sent to handwriting window*/
    AMOTION_EVENT_FLAG_SEND_TO_HANDWRITING_WINDOW = 0x08000000,
};

enum DebugInputEnum {
    DEBUG_INPUT_EVENT_MAJAR     = 0x01,
    DEBUG_INPUT_EVENT_DETAIL    = 0x02,
    DEBUG_INPUT_DISPATCHER_ALL  = 0x08,
    DEBUG_INPUT_READER_ALL      = 0x10,
    DEBUG_INPUT_TRANSPORT_ALL   = 0x20,
};


enum InputDebugChanged {
    CONFIG_INPUT_TRANSPORT = 0x01,
};


class InputDebugConfig : public RefBase
{
public:
    static status_t readFromParcel(const Parcel& parcel, sp<InputDebugConfig> debugInput);
    virtual void dump(std::string& dump);
    InputDebugConfig(bool inputDispatcherMajor, bool inputDispatcherDetail, bool inputDispatcherAll, bool inputReaderAll, bool inputTransportAll);

    bool mInputDispatcherMajor;
    bool mInputDispatcherDetail;
    bool mInputDispatcherAll;
    bool mInputReaderAll;
    bool mInputTransportAll;

private:

};

} // namespace android

#endif // INPUT_DEBUG_CONFIG_H