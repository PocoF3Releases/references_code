/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <memory>

#include <aidl/vendor/xiaomi/hardware/misight/ILogMgrCallback.h>
#include <android-base/macros.h>
#include <android/binder_auto_utils.h>

#include <log_mgr/LogMgr.h>

namespace aidl::vendor::xiaomi::hardware::misight {

// Type of the cookie pointer in linkToDeath.
// A (LogMgr, ILogMgrCallback) tuple.
class LogMgrLinkedCallback {
  public:
    // Automatically linkToDeath upon construction with the returned object as the cookie.
    // service->death_reciepient() should be from CreateDeathRecipient().
    // Not using a strong reference to |service| to avoid circular reference. The lifetime
    // of |service| must be longer than this LinkedCallback object.
    static std::unique_ptr<LogMgrLinkedCallback> Make(std::shared_ptr<LogMgr> service,
                                                std::shared_ptr<ILogMgrCallback> callback);

    // Automatically unlinkToDeath upon destruction. So, it is always safe to reinterpret_cast
    // the cookie back to the LinkedCallback object.
    ~LogMgrLinkedCallback();

    // The wrapped ILogMgrCallback object.
    const std::shared_ptr<ILogMgrCallback>& callback() const { return callback_; }

    // On callback died, unregister it from the service.
    void OnCallbackDied();

  private:
    LogMgrLinkedCallback();
    DISALLOW_COPY_AND_ASSIGN(LogMgrLinkedCallback);

    std::shared_ptr<LogMgr> service();

    std::weak_ptr<LogMgr> service_;
    std::shared_ptr<ILogMgrCallback> callback_;
};

}  // namespace aidl::vendor::xiaomi::hardware::misight
