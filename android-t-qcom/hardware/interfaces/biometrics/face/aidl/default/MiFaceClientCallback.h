
// FIXME: your file license if you have one

#pragma once

#include <aidl/android/hardware/biometrics/face/ISessionCallback.h>
#include <vendor/xiaomi/hardware/miface/1.0/IMiFaceSessionCallback.h>
#include <aidl/android/hardware/biometrics/face/AcquiredInfo.h>
#include <aidl/android/hardware/biometrics/face/Error.h>
#include <utils/Log.h>
#include "FaceUtils.h"

namespace vendor::xiaomi::hardware::miface::implementation {

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using aidl::android::hardware::biometrics::face::AuthenticationFrame;
using aidl::android::hardware::biometrics::face::EnrollmentFrame;
using aidl::android::hardware::biometrics::face::EnrollmentStage;
using aidl::android::hardware::biometrics::face::BaseFrame;
using aidl::android::hardware::biometrics::face::Cell;
using aidl::android::hardware::biometrics::face::ISessionCallback;
using aidl::android::hardware::biometrics::face::AcquiredInfo;
using aidl::android::hardware::biometrics::face::Error;
using aidl::android::hardware::biometrics::face::Feature;
using aidl::android::hardware::biometrics::face::FaceUtils;
namespace keymaster = aidl::android::hardware::keymaster;

struct MiFaceClientCallback : public V1_0::IMiFaceSessionCallback {
public:

    Return<void> setBiometricsFaceClientCallback(std::shared_ptr<ISessionCallback> clientCallback);

    // Methods from ::vendor::xiaomi::hardware::miface::V1_0::IMiFaceClientCallback follow.
    Return<void> onChallengeGenerated(int64_t challenge) override;
    Return<void> onChallengeRevoked(int64_t challenge) override;
    Return<void> onError(int32_t error, int32_t vendorCode) override;
    Return<void> onEnrollmentProgress(int32_t enrollmentId, int32_t remaining) override;
    Return<void> onAuthenticationSucceeded(int32_t enrollmentId, const hidl_vec<uint8_t>& hat) override;
    Return<void> onAuthenticationFailed() override;
    Return<void> onLockoutTimed(int64_t durationMillis) override;
    Return<void> onLockoutPermanent() override;
    Return<void> onLockoutCleared() override;
    Return<void> onInteractionDetected() override;
    Return<void> onEnrollmentsEnumerated(const hidl_vec<int32_t>& enrollmentIds) override;
    Return<void> onFeaturesRetrieved(const hidl_vec<int32_t>& features) override;
    Return<void> onFeatureSet(int32_t feature) override;
    Return<void> onEnrollmentsRemoved(const hidl_vec<int32_t>& enrollmentIds) override;
    Return<void> onAuthenticatorIdRetrieved(int64_t authenticatorId) override;
    Return<void> onAuthenticatorIdInvalidated(int64_t newAuthenticatorId) override;
    Return<void> onSessionClosed() override;
    Return<void> onAcquired(int32_t acquiredInfo, int32_t vendorCode, bool enroll) override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

private:
    std::shared_ptr<ISessionCallback> mClientCallback;
};

// FIXME: most likely delete, this is only for passthrough implementations
// extern "C" IMiFaceClientCallback* HIDL_FETCH_IMiFaceClientCallback(const char* name);

}  // namespace vendor::xiaomi::hardware::miface::implementation