#ifndef INPUT_KNOCK_CONFIG_H
#define INPUT_KNOCK_CONFIG_H

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

enum InputKnockChanged {
    CONFIG_KNOCK_STATE = 1 << 0,
    CONFIG_KNOCK_DEVICE_PROPERTY = 1 << 1,
    CONFIG_KNOCK_VALID_RECT = 1 << 2,
    CONFIG_KNOCK_INVALID_RECT = 1 << 3,
    CONFIG_KNOCK_ALGORITHM_PATH = 1 << 4,
    CONFIG_KNOCK_SCORE_THRESHOLD = 1 << 5,
    CONFIG_KNOCK_USE_FRAME = 1 << 6,
    CONFIG_KNOCK_QUICK_MOVE_SPEED = 1 << 7,
    CONFIG_KNOCK_SENSOR_THRESHOLD = 1 << 8
};

class InputKnockConfig : public RefBase {
public:
    static status_t readFromParcel(const Parcel& parcel, sp<InputKnockConfig> outKnockConfig);
    virtual void dump(std::string& dump);
    InputKnockConfig();
    InputKnockConfig(int32_t knockState, int32_t knockDeviceProperty, int32_t knockValidRectLeft, int32_t knockValidRectTop,
                     int32_t knockValidRectRight, int32_t knockValidRectBottom, int32_t knockInValidRectLeft, int32_t knockInValidRectTop,
                     int32_t knockInValidRectRight, int32_t knockInValidRectBottom, char* knockAlgorithmPath, float knockScoreThreshold,
                     int32_t knockUseFrame, float knockQuickMoveSpeed, std::vector<int> sensorThreshold);

    int32_t mKnockFeatureState;
    int32_t mKnockDeviceProperty;
    int32_t mKnockValidRectLeft;
    int32_t mKnockValidRectTop;
    int32_t mKnockValidRectRight;
    int32_t mKnockValidRectBottom;
    int32_t mKnockInValidRectLeft;
    int32_t mKnockInValidRectTop;
    int32_t mKnockInValidRectRight;
    int32_t mKnockInValidRectBottom;
    String8 mKnockAlgorithmPath;
    float mKnockScoreThreshold;
    int32_t mKnockUseFrame;
    float mKnockQuickMoveSpeed;
    std::vector<int>  mKnockSensorThreshold = {60, 60, 60, 60};
};

} // namespace android

#endif //INPUT_KNOCK_CONFIG_H
