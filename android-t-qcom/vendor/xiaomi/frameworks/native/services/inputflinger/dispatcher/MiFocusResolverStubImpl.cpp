#include "MiFocusResolverStubImpl.h"
#include <android/gui/FocusRequest.h>
#include <binder/Binder.h>
#include <ftl/enum.h>
#include <log/log.h>
#include <log/log_event_list.h>
#include <string>

using android::gui::FocusRequest;

// MIUI ADD
constexpr int LOGTAG_INPUT_FOCUS = 62001;

inline const char* focusabilityToString(int focusability) {
    switch (focusability) {
        case 0:
            return "OK";
        case 1:
            return "NO_WINDOW";
        case 2:
            return "NOT_FOCUSABLE";
        case 3:
            return "NOT_VISIBLE";
        default:
            return "UNKNOWN";
    }
}

namespace android {

void MiFocusResolverStubImpl::afterSetInputWindowsFocusChecked(
        const std::optional<FocusRequest>& request, int32_t displayId, int result,
        int previousResult, const sp<IBinder>& currentFocus) {
    if (result == FOCUSABILITY_OK) {
        std::string message = std::string("At display :") + std::to_string(displayId);
        message = message + std::string(",Focus update :") + request->windowName;
        std::string reason = std::string("reason=setInputWindows");
        android_log_event_list(LOGTAG_INPUT_FOCUS) << message << reason << LOG_ID_EVENTS;
        return;
    }
    if (mLastFailFocusReason != result && currentFocus == nullptr) {
        mLastFailFocusReason = result;
        std::string message = std::string("At display :") + std::to_string(displayId);
        message = message + std::string(",Ignore setInputWindows Focus :") + request->windowName;
        std::string reason = std::string("reason = ") + focusabilityToString(result);
        android_log_event_list(LOGTAG_INPUT_FOCUS) << message << reason << LOG_ID_EVENTS;
    }
}

void MiFocusResolverStubImpl::afterSetFocusedWindowFocusChecked(const FocusRequest& request,
                                                                int32_t displayId, int result,
                                                                int previousResult,
                                                                const sp<IBinder>& currentFocus) {
    if (request.focusedToken) {
        if (result == FOCUSABILITY_OK) {
            return;
        }
        std::string message = std::string("At display :") + std::to_string(displayId);
        message = message + std::string(",Ignore setFocusedWindow :") + request.windowName;
        std::string reason = std::string("Reason = ") + focusabilityToString(result);
        android_log_event_list(LOGTAG_INPUT_FOCUS) << message << reason << LOG_ID_EVENTS;
        return;
    }
    if (result == FOCUSABILITY_OK) {
        return;
    }
    std::string message = std::string("At display :") + std::to_string(displayId);
    message = message + std::string(",Ignore setFocusedWindow :") + request.windowName;
    std::string reason = std::string("Reason = ") + focusabilityToString(result);
    android_log_event_list(LOGTAG_INPUT_FOCUS) << message << reason << LOG_ID_EVENTS;
}

extern "C" IMiFocusResolverStub* createFocusResolverStubImpl() {
    return new MiFocusResolverStubImpl;
}

extern "C" void destroyFocusResolverStubImpl(IMiFocusResolverStub* instance) {
    delete instance;
}
} // namespace android
