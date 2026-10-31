#define DEBUG_UTILS 1

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "miface-aidl"

#include "FaceUtils.h"
#include <utils/Log.h>
#include <hardware/hw_auth_token.h>

namespace aidl::android::hardware::biometrics::face {
static FaceUtils* mfu;

FaceUtils::FaceUtils() {
}

FaceUtils::~FaceUtils() {
}

FaceUtils* FaceUtils::getInstance() {
    if(mfu == nullptr) {
        mfu = new FaceUtils();
    }
    return mfu;
}

constexpr size_t kHmacSize = 32;
HardwareAuthToken FaceUtils::hidlVec2AuthToken(const hidl_vec<uint8_t>& buffer) {
    HardwareAuthToken token;
    static_assert(1 /* version size */ + sizeof(token.challenge) + sizeof(token.userId) +
                                  sizeof(token.authenticatorId) + sizeof(token.authenticatorType) +
                                  sizeof(token.timestamp) + kHmacSize ==
                          sizeof(hw_auth_token_t),
                  "HardwareAuthToken content size does not match hw_auth_token_t size");

    if (buffer.size() != sizeof(hw_auth_token_t)) return {};

    auto pos = buffer.begin();
    ++pos;  // skip first byte
    pos = copy_bytes_from_iterator(&token.challenge, pos);
    pos = copy_bytes_from_iterator(&token.userId, pos);
    pos = copy_bytes_from_iterator(&token.authenticatorId, pos);
    pos = copy_bytes_from_iterator(&token.authenticatorType, pos);
    token.authenticatorType = static_cast<keymaster::HardwareAuthenticatorType>(
            ntohl(static_cast<int32_t>(token.authenticatorType)));
    pos = copy_bytes_from_iterator(&token.timestamp.milliSeconds, pos);
    token.mac.resize(kHmacSize);
    std::copy(pos, pos + kHmacSize, token.mac.data());

    return token;
}

std::vector<uint8_t> FaceUtils::authToken2AidlVec(const HardwareAuthToken& token) {
    static_assert(1 /* version size */ + sizeof(token.challenge) + sizeof(token.userId) +
                          sizeof(token.authenticatorId) + sizeof(token.authenticatorType) +
                          sizeof(token.timestamp) + 32 /* HMAC size */
                      == sizeof(hw_auth_token_t),
                  "HardwareAuthToken content size does not match hw_auth_token_t size");
    if (DEBUG_UTILS) ALOGD("authToken2AidlVec challenge.size %d, userId.size %d, authenticatorId.size %d, authenticatorType.size %d timestamp,size %d",
        sizeof(token.challenge), sizeof(token.userId), sizeof(token.authenticatorId), sizeof(token.authenticatorType), sizeof(token.timestamp));

    std::vector<uint8_t> result;

    if (token.mac.size() < 32) return result;

    result.resize(sizeof(hw_auth_token_t));
    auto pos = result.begin();
    *pos++ = 0;  // Version byte
    pos = copy_bytes_to_iterator(token.challenge, pos);
    pos = copy_bytes_to_iterator(token.userId, pos);
    pos = copy_bytes_to_iterator(token.authenticatorId, pos);
    pos = copy_bytes_to_iterator(token.authenticatorType, pos);
    pos = copy_bytes_to_iterator(token.timestamp, pos);
    pos = std::copy(token.mac.data(), token.mac.data() + token.mac.size(), pos);

    return result;
}

AcquiredInfo FaceUtils::convert2FaceAcquiredInfo(int32_t acquiredInfo) {
    switch(acquiredInfo) {
        case 0:
            return AcquiredInfo::GOOD;
        case 1:
            return AcquiredInfo::INSUFFICIENT;
        case 2:
            return AcquiredInfo::TOO_BRIGHT;
        case 3:
            return AcquiredInfo::TOO_DARK;
        case 4:
            return AcquiredInfo::TOO_CLOSE;
        case 5:
            return AcquiredInfo::TOO_FAR;
        case 6:
            return AcquiredInfo::FACE_TOO_HIGH;
        case 7:
            return AcquiredInfo::FACE_TOO_LOW;
        case 8:
            return AcquiredInfo::FACE_TOO_RIGHT;
        case 9:
            return AcquiredInfo::FACE_TOO_LEFT;
        case 10:
            return AcquiredInfo::POOR_GAZE;
        case 11:
            return AcquiredInfo::NOT_DETECTED;
        case 12:
            return AcquiredInfo::TOO_MUCH_MOTION;
        case 13:
            return AcquiredInfo::RECALIBRATE;
        case 14:
            return AcquiredInfo::TOO_DIFFERENT;
        case 15:
            return AcquiredInfo::TOO_SIMILAR;
        case 16:
            return AcquiredInfo::PAN_TOO_EXTREME;
        case 17:
            return AcquiredInfo::TILT_TOO_EXTREME;
        case 18:
            return AcquiredInfo::ROLL_TOO_EXTREME;
        case 19:
            return AcquiredInfo::FACE_OBSCURED;
        case 20:
            return AcquiredInfo::START;
        case 21:
            return AcquiredInfo::SENSOR_DIRTY;
        case 22:
            return AcquiredInfo::VENDOR;
        default:
            return AcquiredInfo::VENDOR;
    }
}

Error FaceUtils::convert2FaceError(int32_t error) {
    switch(error) {
        case 0:
            return Error::UNKNOWN;
        case 1:
            return Error::HW_UNAVAILABLE;
        case 2:
            return Error::UNABLE_TO_PROCESS;
        case 3:
            return Error::TIMEOUT;
        case 4:
            return Error::NO_SPACE;
        case 5:
            return Error::CANCELED;
        case 6:
            return Error::UNABLE_TO_REMOVE;
        case 7:
            return Error::VENDOR;
        case 8:
            return Error::REENROLL_REQUIRED;
        default:
            return Error::UNKNOWN;
    }
}

Feature FaceUtils::convert2FaceFeature(int32_t feature) {
    switch(feature) {
        case 0:
            return Feature::REQUIRE_ATTENTION;
        case 1:
            return Feature::REQUIRE_DIVERSE_POSES;
        case 2:
            return Feature::DEBUG;
        default:
            return Feature::DEBUG;
    }
}

uint32_t FaceUtils::convertFeature2int(Feature feature) {
    switch (feature)
    {
    case Feature::REQUIRE_ATTENTION:
        return 0;
    case Feature::REQUIRE_DIVERSE_POSES:
        return 1;
    case Feature::DEBUG:
        return 2;
    default:
        return 2;
    }
}

}