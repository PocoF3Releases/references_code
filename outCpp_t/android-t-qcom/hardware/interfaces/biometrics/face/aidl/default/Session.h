/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <aidl/android/hardware/biometrics/face/BnSession.h>
#include <aidl/android/hardware/biometrics/face/ISessionCallback.h>
#include <aidl/android/hardware/biometrics/common/BnCancellationSignal.h>

#include <vendor/xiaomi/hardware/miface/1.0/IMiFace.h>
#include "MiFaceClientCallback.h"
#include <hardware/hw_auth_token.h>
#include "FaceUtils.h"

namespace aidl::android::hardware::biometrics::face {

namespace common = aidl::android::hardware::biometrics::common;
namespace keymaster = aidl::android::hardware::keymaster;
using ::aidl::android::hardware::keymaster::HardwareAuthToken;
using aidl::android::hardware::biometrics::face::FaceUtils;

using aidl::android::hardware::common::NativeHandle;

using ::android::sp;

using ::vendor::xiaomi::hardware::miface::V1_0::IMiFace;
using ::vendor::xiaomi::hardware::miface::V1_0::IMiFaceSession;
using ::vendor::xiaomi::hardware::miface::implementation::MiFaceClientCallback;

class Session : public BnSession {
  public:
    explicit Session(int32_t sensorId, int32_t userId, std::shared_ptr<ISessionCallback> cb);

    ndk::ScopedAStatus generateChallenge() override;

    ndk::ScopedAStatus revokeChallenge(int64_t challenge) override;

    ndk::ScopedAStatus getEnrollmentConfig(EnrollmentType enrollmentType,
                                           std::vector<EnrollmentStageConfig>* return_val) override;

    ndk::ScopedAStatus enroll(const keymaster::HardwareAuthToken& hat,
                              EnrollmentType enrollmentType, const std::vector<Feature>& features,
                              const std::optional<NativeHandle>& previewSurface,
                              std::shared_ptr<common::ICancellationSignal>* return_val) override;

    ndk::ScopedAStatus authenticate(
            int64_t keystoreOperationId,
            std::shared_ptr<common::ICancellationSignal>* returnVal) override;

    ndk::ScopedAStatus detectInteraction(
            std::shared_ptr<common::ICancellationSignal>* returnVal) override;

    ndk::ScopedAStatus enumerateEnrollments() override;

    ndk::ScopedAStatus removeEnrollments(const std::vector<int32_t>& enrollmentIds) override;

    ndk::ScopedAStatus getFeatures() override;

    ndk::ScopedAStatus setFeature(const keymaster::HardwareAuthToken& hat, Feature feature,
                                  bool enabled) override;

    ndk::ScopedAStatus getAuthenticatorId() override;

    ndk::ScopedAStatus invalidateAuthenticatorId() override;

    ndk::ScopedAStatus resetLockout(const keymaster::HardwareAuthToken& hat) override;

    ndk::ScopedAStatus close() override;

    ndk::ScopedAStatus authenticateWithContext(
            int64_t operationId, const common::OperationContext& context,
            std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus enrollWithContext(
            const keymaster::HardwareAuthToken& hat, EnrollmentType enrollmentType,
            const std::vector<Feature>& features, const std::optional<NativeHandle>& previewSurface,
            const common::OperationContext& context,
            std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus detectInteractionWithContext(
            const common::OperationContext& context,
            std::shared_ptr<common::ICancellationSignal>* out) override;

    ndk::ScopedAStatus onContextChanged(const common::OperationContext& context) override;

    sp<IMiFaceSession> setUpMiFaceSession();
    sp<IMiFaceSession> miface_session;
    sp<IMiFace> miface;
    int32_t sensorId;
    int32_t userId;

  private:
    sp<IMiFace> checkMiFaceService();
    std::shared_ptr<ISessionCallback> cb_;
    std::mutex mifaceSessionMutex;
    sp<MiFaceClientCallback> mifaceClientCallback;
};

class CancellationSignal : public common::BnCancellationSignal {
  private:
    std::shared_ptr<ISessionCallback> cb_;
    sp<IMiFaceSession> miface_session_;

  public:
    explicit CancellationSignal(std::shared_ptr<ISessionCallback> cb, sp<IMiFaceSession> miface_session)
        : cb_(std::move(cb)), miface_session_(std::move(miface_session)) {};

    ndk::ScopedAStatus cancel() override;
};

}  // namespace aidl::android::hardware::biometrics::face
