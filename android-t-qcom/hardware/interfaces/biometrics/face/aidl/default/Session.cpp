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

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "miface-aidl"

#define DEBUG_SESSION 1

#include <aidlcommonsupport/NativeHandle.h>
#include <sys/syscall.h>
#include <utils/Log.h>

#include "Session.h"

namespace aidl::android::hardware::biometrics::face {
using aidl::android::hardware::common::NativeHandle;

Session::Session(int32_t sensorId, int32_t userId, std::shared_ptr<ISessionCallback> cb)
    : sensorId(sensorId), userId(userId), cb_(std::move(cb)) {
    setUpMiFaceSession();
}

sp<IMiFaceSession> Session::setUpMiFaceSession() {
    std::lock_guard<std::mutex> slock(mifaceSessionMutex);
    if (miface_session == nullptr) {
        int tid = (int) syscall(SYS_gettid);
        if (DEBUG_SESSION) ALOGD("setUpMiFaceSession tid %d", tid);
        sp<MiFaceClientCallback> mifaceClientCallback;
        mifaceClientCallback = new MiFaceClientCallback();
        mifaceClientCallback->setBiometricsFaceClientCallback(cb_);
        if (checkMiFaceService()) {
            miface_session = miface->createSession(sensorId, userId, mifaceClientCallback);
            if (miface_session == nullptr) {
                ALOGE("setUpMiFaceSession -> createSession failed");
            }
        }
    }
    return miface_session;
}

sp<IMiFace> Session::checkMiFaceService() {
    if(miface == nullptr) {
        miface = IMiFace::getService();
    }
    if(miface == nullptr) {
        ALOGE("Get miface daemon fail");
    }
    return miface;
}

ndk::ScopedAStatus Session::generateChallenge() {
    if (setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->generateChallenge();
    } else {
        cb_->onChallengeGenerated(0);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::revokeChallenge(int64_t challenge) {
    if (setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->revokeChallenge(challenge);
    } else {
        cb_->onChallengeRevoked(challenge);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getEnrollmentConfig(EnrollmentType /*enrollmentType*/,
                                                std::vector<EnrollmentStageConfig>* return_val) {
    ALOGD("getEnrollmentConfig");
    *return_val = {};
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enroll(
        const keymaster::HardwareAuthToken& hat, EnrollmentType enrollmentType,
        const std::vector<Feature>& features,
        const std::optional<NativeHandle>& previewSurface,
        std::shared_ptr<biometrics::common::ICancellationSignal>* return_val) {

    if (setUpMiFaceSession() != nullptr) {
        if (!previewSurface.has_value()) {
            cb_->onError(Error::UNABLE_TO_PROCESS, 0);
            ALOGE("%s - previewSurface is null", __func__);
        } else {
            std::vector<uint32_t> features_t;
            if (DEBUG_SESSION) {
                for (Feature feature: features) {
                    uint32_t feature_t = FaceUtils::getInstance()->convertFeature2int(feature);
                    ALOGD("enroll feature %d", feature_t);
                    features_t.push_back(feature_t);
                }
            }

            native_handle_t* surfaceRef =
                const_cast<native_handle_t*>(::android::makeFromAidl(previewSurface.value()));
            std::lock_guard<std::mutex> slock(mifaceSessionMutex);
            miface_session->enroll(FaceUtils::getInstance()->authToken2AidlVec(hat), features_t, surfaceRef);
        }
        *return_val = SharedRefBase::make<CancellationSignal>(cb_, miface_session);
    } else {
        cb_->onError(Error::UNABLE_TO_PROCESS, 0);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticate(int64_t keystoreOperationId,
                                         std::shared_ptr<common::ICancellationSignal>* return_val) {

    if(setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->authenticate(keystoreOperationId);
    }
    *return_val = SharedRefBase::make<CancellationSignal>(cb_, miface_session);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::detectInteraction(
        std::shared_ptr<common::ICancellationSignal>* /*return_val*/) {

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::enumerateEnrollments() {

    if(setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->enumerateEnrollments();
    } else {
        cb_->onEnrollmentsEnumerated({});
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::removeEnrollments(const std::vector<int32_t>& enrollmentIds) {

    if(setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->removeEnrollments(enrollmentIds);
    } else {
        cb_->onEnrollmentsRemoved({});
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getFeatures() {
    if(setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->getFeatures();
    } else {
        cb_->onError(Error::UNABLE_TO_PROCESS, 0);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::setFeature(const keymaster::HardwareAuthToken& hat,
                                       Feature feature, bool enabled) {
    if(setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->setFeature(FaceUtils::getInstance()->authToken2AidlVec(hat),
            FaceUtils::getInstance()->convertFeature2int(feature), enabled);
    } else {
        cb_->onError(Error::UNABLE_TO_PROCESS, 0);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::getAuthenticatorId() {
    if (setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->getAuthenticatorId();
    } else {
        cb_->onAuthenticatorIdRetrieved(0);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::invalidateAuthenticatorId() {
    if(setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->invalidateAuthenticatorId();
    } else {
        cb_->onAuthenticatorIdInvalidated(0);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::resetLockout(const keymaster::HardwareAuthToken& hat) {
    if (DEBUG_SESSION) ALOGD("resetLockout");
    if(setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->resetLockout(FaceUtils::getInstance()->authToken2AidlVec(hat));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::close() {
    if (DEBUG_SESSION) ALOGD("close");
    if(setUpMiFaceSession() != nullptr) {
        std::lock_guard<std::mutex> slock(mifaceSessionMutex);
        miface_session->close();
    } else {
        cb_->onSessionClosed();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Session::authenticateWithContext(
        int64_t operationId, const common::OperationContext& /*context*/,
        std::shared_ptr<common::ICancellationSignal>* out) {
    return authenticate(operationId, out);
}

ndk::ScopedAStatus Session::enrollWithContext(const keymaster::HardwareAuthToken& hat,
                                              EnrollmentType enrollmentType,
                                              const std::vector<Feature>& features,
                                              const std::optional<NativeHandle>& previewSurface,
                                              const common::OperationContext& /*context*/,
                                              std::shared_ptr<common::ICancellationSignal>* out) {
    return enroll(hat, enrollmentType, features, previewSurface, out);
}

ndk::ScopedAStatus Session::detectInteractionWithContext(
        const common::OperationContext& /*context*/,
        std::shared_ptr<common::ICancellationSignal>* out) {
    return detectInteraction(out);
}

ndk::ScopedAStatus Session::onContextChanged(const common::OperationContext& /*context*/) {
    return ndk::ScopedAStatus::ok();
}

// namespace aidl::android::hardware::biometrics::face
ndk::ScopedAStatus CancellationSignal::cancel() {
    if (miface_session_ != nullptr) {
        if (DEBUG_SESSION) ALOGD("cancel");
        int32_t result = miface_session_->cancel();
        if(result != 0) {
            if (DEBUG_SESSION) ALOGD("cancel(): cb_->onError(Error::UNABLE_TO_PROCESS)");
            cb_->onError(Error::UNABLE_TO_PROCESS, 0);
        }
    } else {
        cb_->onError(Error::UNABLE_TO_PROCESS, 0);
    }
    return ndk::ScopedAStatus::ok();
}

}
// namespace aidl::android::hardware::biometrics::face
