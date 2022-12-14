#pragma once
#include <android/gui/FocusRequest.h>
#include <binder/Binder.h>
#include "IMiFocusResolverStub.h"

namespace android {
class MiFocusResolverStubImpl : public IMiFocusResolverStub {
private:
    static constexpr int FOCUSABILITY_OK = 0;
    static constexpr int FOCUSABILITY_NO_WINDOW = 1;
    static constexpr int FOCUSABILITY_NOT_FOCUSABLE = 2;
    static constexpr int FOCUSABILITY_NOT_VISIBLE = 3;
    int mLastFailFocusReason = FOCUSABILITY_OK;

public:
    void afterSetInputWindowsFocusChecked(const std::optional<gui::FocusRequest>& request,
                                          int32_t displayId, int result, int previousResult,
                                          const sp<IBinder>& currentFocus) override;
    void afterSetFocusedWindowFocusChecked(const gui::FocusRequest& request, int32_t displayId,
                                           int result, int previousResult,
                                           const sp<IBinder>& currentFocus) override;
};
} // namespace android
