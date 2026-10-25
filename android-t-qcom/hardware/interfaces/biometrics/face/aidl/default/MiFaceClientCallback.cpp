// FIXME: your file license if you have one

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "miface-aidl"

#define DEBUG_CALLBACK 1

#include "MiFaceClientCallback.h"
#include <hardware/hw_auth_token.h>

namespace vendor::xiaomi::hardware::miface::implementation {

Return<void> MiFaceClientCallback::setBiometricsFaceClientCallback(std::shared_ptr<ISessionCallback> clientCallback) {
    mClientCallback = std::move(clientCallback);
    return Void();
}

// Methods from ::vendor::xiaomi::hardware::miface::V1_0::IMiFaceClientCallback follow.
Return<void> MiFaceClientCallback::onChallengeGenerated(int64_t challenge) {
    if(mClientCallback != nullptr) {
        mClientCallback->onChallengeGenerated(challenge);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onChallengeRevoked(int64_t challenge) {
    if(mClientCallback != nullptr) {
        mClientCallback->onChallengeRevoked(challenge);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onError(int32_t error, int32_t vendorCode) {
    if(mClientCallback != nullptr) {
        Error error_= FaceUtils::getInstance()->convert2FaceError(error);
        mClientCallback->onError(error_, vendorCode);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onEnrollmentProgress(int32_t enrollmentId, int32_t remaining) {
    if(mClientCallback != nullptr) {
        mClientCallback->onEnrollmentProgress(enrollmentId, remaining);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onAuthenticationSucceeded(int32_t enrollmentId, const hidl_vec<uint8_t>& hat) {
    if(mClientCallback != nullptr) {
        mClientCallback->onAuthenticationSucceeded(enrollmentId, FaceUtils::getInstance()->hidlVec2AuthToken(hat));
    }
    return Void();
}

Return<void> MiFaceClientCallback::onAuthenticationFailed() {
    if(mClientCallback != nullptr) {
        mClientCallback->onAuthenticationFailed();
    }
    return Void();
}

Return<void> MiFaceClientCallback::onLockoutTimed(int64_t durationMillis) {
    if(mClientCallback != nullptr) {
        mClientCallback->onLockoutTimed(durationMillis);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onLockoutPermanent() {
    if(mClientCallback != nullptr) {
        mClientCallback->onLockoutPermanent();
    }
    return Void();
}

Return<void> MiFaceClientCallback::onLockoutCleared() {
    if(mClientCallback != nullptr) {
        mClientCallback->onLockoutCleared();
    }
    return Void();
}

Return<void> MiFaceClientCallback::onInteractionDetected() {
    if(mClientCallback != nullptr) {
        mClientCallback->onInteractionDetected();
    }
    return Void();
}

Return<void> MiFaceClientCallback::onEnrollmentsEnumerated(const hidl_vec<int32_t>& enrollmentIds) {
    const std::vector<int32_t> enrollmentIds_vec = enrollmentIds;
    if(mClientCallback != nullptr) {
        mClientCallback->onEnrollmentsEnumerated(enrollmentIds_vec);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onFeaturesRetrieved(const hidl_vec<int32_t>& features) {
    std::string featureString = "";
    std::vector<Feature> features_vec(features.size());
    for (int i = 0; i < features.size(); i++) {
        features_vec[i] = FaceUtils::getInstance()->convert2FaceFeature(features[i]);
        featureString += std::to_string(features[i]) + " ";
    }
    ALOGD("onFeaturesRetrieved value %s", featureString.c_str());
    if(mClientCallback != nullptr) {
        mClientCallback->onFeaturesRetrieved(features_vec);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onFeatureSet(int32_t feature) {
    if(mClientCallback != nullptr) {
        mClientCallback->onFeatureSet(FaceUtils::getInstance()->convert2FaceFeature(feature));
    }
    return Void();
}

Return<void> MiFaceClientCallback::onEnrollmentsRemoved(const hidl_vec<int32_t>& enrollmentIds) {
    if(mClientCallback != nullptr) {
        mClientCallback->onEnrollmentsRemoved(enrollmentIds);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onAuthenticatorIdRetrieved(int64_t authenticatorId) {
    if(mClientCallback != nullptr) {
        mClientCallback->onAuthenticatorIdRetrieved(authenticatorId);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onAuthenticatorIdInvalidated(int64_t newAuthenticatorId) {
    if(mClientCallback != nullptr) {
        mClientCallback->onAuthenticatorIdInvalidated(newAuthenticatorId);
    }
    return Void();
}

Return<void> MiFaceClientCallback::onSessionClosed() {
    if(mClientCallback != nullptr) {
        mClientCallback->onSessionClosed();
    }
    return Void();
}

Return<void> MiFaceClientCallback::onAcquired(int32_t acquiredInfo, int32_t vendorCode, bool enroll) {
    BaseFrame baseFrame;
    baseFrame.acquiredInfo = FaceUtils::getInstance()->convert2FaceAcquiredInfo(acquiredInfo);
    baseFrame.vendorCode = vendorCode;
    if (enroll) {
        EnrollmentFrame enrollmentFrame;
        enrollmentFrame.data = baseFrame;
        ALOGD("Enroll--onAcquired acquiredInfo %d vendorCode %d enroll %d", acquiredInfo, vendorCode, enroll);
    
        mClientCallback->onEnrollmentFrame(enrollmentFrame);
    } else {
        AuthenticationFrame authenticationFrame;
        authenticationFrame.data = baseFrame;
        ALOGD("Auth--onAcquired acquiredInfo %d vendorCode %d enroll %d", acquiredInfo, vendorCode, enroll);
        mClientCallback->onAuthenticationFrame(authenticationFrame);
    }
    return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

//IMiFaceClientCallback* HIDL_FETCH_IMiFaceClientCallback(const char* /* name */) {
    //return new MiFaceClientCallback();
//}
//
}  // namespace vendor::xiaomi::hardware::miface::implementation