#define LOG_TAG "InputKnockConfig"

#include "InputKnockConfig.h"
#include <android-base/stringprintf.h>
#include <utils/String8.h>

#define INDENT2 "    "

using android::base::StringPrintf;

// MIUI ADD:
namespace android {

InputKnockConfig::InputKnockConfig()
       : mKnockFeatureState(0),
         mKnockDeviceProperty(0),
         mKnockValidRectLeft(0),
         mKnockValidRectTop(0),
         mKnockValidRectRight(0),
         mKnockValidRectBottom(0),
         mKnockInValidRectLeft(0),
         mKnockInValidRectTop(0),
         mKnockInValidRectRight(0),
         mKnockInValidRectBottom(0),
         mKnockAlgorithmPath((char*)""),
         mKnockScoreThreshold(0.0),
         mKnockUseFrame(0),
         mKnockQuickMoveSpeed(0.0){}

InputKnockConfig::InputKnockConfig(int32_t knockFeatureState, int32_t knockDeviceProperty, int32_t knockValidRectLeft,
                                   int32_t knockValidRectTop, int32_t knockValidRectRight, int32_t knockValidRectBottom,
                                   int32_t knockInValidRectLeft, int32_t knockInValidRectTop, int32_t knockInValidRectRight,
                                   int32_t knockInValidRectBottom, char* knockAlgorithmPath, float knockScoreThreshold,
                                   int32_t knockUseFrame, float knockQuickMoveSpeed, std::vector<int> sensorThreshold)
       : mKnockFeatureState(knockFeatureState),
         mKnockDeviceProperty(knockDeviceProperty),
         mKnockValidRectLeft(knockValidRectLeft),
         mKnockValidRectTop(knockValidRectTop),
         mKnockValidRectRight(knockValidRectRight),
         mKnockValidRectBottom(knockValidRectBottom),
         mKnockInValidRectLeft(knockInValidRectLeft),
         mKnockInValidRectTop(knockInValidRectTop),
         mKnockInValidRectRight(knockInValidRectRight),
         mKnockInValidRectBottom(knockInValidRectBottom),
         mKnockAlgorithmPath(knockAlgorithmPath),
         mKnockScoreThreshold(knockScoreThreshold),
         mKnockUseFrame(knockUseFrame),
         mKnockQuickMoveSpeed(knockQuickMoveSpeed),
         mKnockSensorThreshold(sensorThreshold){}

status_t InputKnockConfig::readFromParcel(const Parcel& parcel, sp<InputKnockConfig> knockConfig) {
    int32_t knockFeatureState;
    int32_t knockDeviceProperty;
    int32_t knockValidRectLeft;
    int32_t knockValidRectTop;
    int32_t knockValidRectRight;
    int32_t knockValidRectBottom;
    int32_t knockInValidRectLeft;
    int32_t knockInValidRectTop;
    int32_t knockInValidRectRight;
    int32_t knockInValidRectBottom;
    String8 knockAlgorithmPath;
    float knockScoreThreshold;
    int32_t knockUseFrame;
    float knockQuickMoveSpeed;
    std::vector<int> knockSensorThreshold;

    SAFE_PARCEL(parcel.readInt32, &knockFeatureState);
    SAFE_PARCEL(parcel.readInt32, &knockDeviceProperty);
    SAFE_PARCEL(parcel.readInt32, &knockValidRectLeft);
    SAFE_PARCEL(parcel.readInt32, &knockValidRectTop);
    SAFE_PARCEL(parcel.readInt32, &knockValidRectRight);
    SAFE_PARCEL(parcel.readInt32, &knockValidRectBottom);
    SAFE_PARCEL(parcel.readInt32, &knockInValidRectLeft);
    SAFE_PARCEL(parcel.readInt32, &knockInValidRectTop);
    SAFE_PARCEL(parcel.readInt32, &knockInValidRectRight);
    SAFE_PARCEL(parcel.readInt32, &knockInValidRectBottom);
    SAFE_PARCEL(parcel.readString8, &knockAlgorithmPath);
    SAFE_PARCEL(parcel.readFloat, &knockScoreThreshold);
    SAFE_PARCEL(parcel.readInt32, &knockUseFrame);
    SAFE_PARCEL(parcel.readFloat, &knockQuickMoveSpeed);
    SAFE_PARCEL(parcel.readInt32Vector, &knockSensorThreshold);


    int32_t result = 0;
    if (knockConfig->mKnockFeatureState != knockFeatureState) {
        result |= CONFIG_KNOCK_STATE;
    }
    if (knockConfig->mKnockDeviceProperty != knockDeviceProperty) {
        result |= CONFIG_KNOCK_DEVICE_PROPERTY;
    }
    if (knockConfig->mKnockValidRectLeft != knockValidRectLeft || knockConfig->mKnockValidRectTop != knockValidRectTop
        || knockConfig->mKnockValidRectRight != knockValidRectRight || knockConfig->mKnockValidRectBottom != knockValidRectBottom) {
        result |= CONFIG_KNOCK_VALID_RECT;
    }
    if (knockConfig->mKnockInValidRectLeft != knockInValidRectLeft || knockConfig->mKnockInValidRectTop != knockInValidRectTop
        || knockConfig->mKnockInValidRectRight != knockInValidRectRight || knockConfig->mKnockInValidRectBottom != knockInValidRectBottom) {
        result |= CONFIG_KNOCK_INVALID_RECT;
    }
    if (knockConfig->mKnockAlgorithmPath != knockAlgorithmPath) {
        result |= CONFIG_KNOCK_ALGORITHM_PATH;
    }


    if (knockConfig->mKnockScoreThreshold != knockScoreThreshold) {
        result |= CONFIG_KNOCK_SCORE_THRESHOLD;
    }
    if (knockConfig->mKnockUseFrame != knockUseFrame) {
        result |= CONFIG_KNOCK_USE_FRAME;
    }
    if (knockConfig->mKnockQuickMoveSpeed != knockQuickMoveSpeed) {
        result |= CONFIG_KNOCK_QUICK_MOVE_SPEED;
    }
    if (knockConfig->mKnockSensorThreshold != knockSensorThreshold) {
        result |= CONFIG_KNOCK_SENSOR_THRESHOLD;
    }

    knockConfig->mKnockFeatureState = knockFeatureState;
    knockConfig->mKnockDeviceProperty = knockDeviceProperty;
    knockConfig->mKnockValidRectLeft = knockValidRectLeft;
    knockConfig->mKnockValidRectTop = knockValidRectTop;
    knockConfig->mKnockValidRectRight = knockValidRectRight;
    knockConfig->mKnockValidRectBottom = knockValidRectBottom;
    knockConfig->mKnockInValidRectLeft = knockInValidRectLeft;
    knockConfig->mKnockInValidRectTop = knockInValidRectTop;
    knockConfig->mKnockInValidRectRight = knockInValidRectRight;
    knockConfig->mKnockInValidRectBottom = knockInValidRectBottom;
    knockConfig->mKnockAlgorithmPath = knockAlgorithmPath;
    knockConfig->mKnockScoreThreshold = knockScoreThreshold;
    knockConfig->mKnockUseFrame = knockUseFrame;
    knockConfig->mKnockQuickMoveSpeed = knockQuickMoveSpeed;
    knockConfig->mKnockSensorThreshold = knockSensorThreshold;

    return result;
}

void InputKnockConfig::dump(std::string& dump) {
    dump += "\nInputKnockConfig State\n";
    dump += StringPrintf(INDENT2 "mKnockFeatureState: %d\n", mKnockFeatureState);
    dump += StringPrintf(INDENT2 "mKnockDeviceProperty: %d\n", mKnockDeviceProperty);
    dump += StringPrintf(INDENT2 "mKnockValidRegion: [%d, %d, %d, %d]\n", mKnockValidRectLeft, mKnockValidRectTop, mKnockValidRectRight, mKnockValidRectBottom);
    dump += StringPrintf(INDENT2 "mKnockInValidRegion: [%d, %d, %d, %d]\n", mKnockInValidRectLeft, mKnockInValidRectTop, mKnockInValidRectRight, mKnockInValidRectBottom);
    dump += StringPrintf(INDENT2 "mKnockAlgorithmPath: %s\n", mKnockAlgorithmPath.c_str());
    dump += StringPrintf(INDENT2 "mKnockScoreThreshold: %.2f\n", mKnockScoreThreshold);
    dump += StringPrintf(INDENT2 "mKnockUseFrame: %d\n", mKnockUseFrame);
    dump += StringPrintf(INDENT2 "mKnockQuickMoveSpeed: %.2f\n", mKnockQuickMoveSpeed);
    dump += StringPrintf(INDENT2 "mKnockSensorThreshold: [%d, %d, %d, %d]\n", mKnockSensorThreshold[0], mKnockSensorThreshold[1], mKnockSensorThreshold[2], mKnockSensorThreshold[3]);
}

}