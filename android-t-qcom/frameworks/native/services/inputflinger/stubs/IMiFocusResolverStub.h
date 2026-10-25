#pragma once
#include <android/gui/FocusRequest.h>
#include <binder/Binder.h>
#include <memory>
namespace android {

class IMiFocusResolverStub {
public:
    IMiFocusResolverStub(){};

    virtual ~IMiFocusResolverStub(){};

    virtual void afterSetInputWindowsFocusChecked(const std::optional<gui::FocusRequest>& request,
                                                  int32_t displayId, int result, int previousResult,
                                                  const sp<IBinder>& currentFocus);
    virtual void afterSetFocusedWindowFocusChecked(const gui::FocusRequest& request,
                                                   int32_t displayId, int result,
                                                   int previousResult,
                                                   const sp<IBinder>& currentFocus);
};
} // namespace android