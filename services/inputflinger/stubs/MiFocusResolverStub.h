#pragma once
#include <android/gui/FocusRequest.h>
#include <binder/Binder.h>
#include <mutex>
#include <string>
#include "IMiFocusResolverStub.h"

namespace android {
class MiFocusResolverStub {
private:
    static void* sLibMiFocusResolverImpl;
    static IMiFocusResolverStub* sStubImplInstance;
    static std::mutex sLock;
    static bool inited;
    static IMiFocusResolverStub* getImplInstance();
    static void destroyImplInstance();
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmiinputflinger.so";

public:
    static void afterSetInputWindowsFocusChecked(const std::optional<gui::FocusRequest>& request,
                                                 int32_t displayId, int result, int previousResult,
                                                 const sp<IBinder>& currentFocus);
    static void afterSetFocusedWindowFocusChecked(const gui::FocusRequest& request,
                                                  int32_t displayId, int result, int previousResult,
                                                  const sp<IBinder>& currentFocus);
};
} // namespace android
