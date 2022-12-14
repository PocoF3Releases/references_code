#pragma once

#include <mutex>
#include <string>
//#include "../dispatcher/Connection.h"
//#include "../dispatcher/InputDispatcher.h"
#include "IMiInputDispatcherStub.h"

namespace android {

using namespace inputdispatcher;

class MiInputDispatcherStub {
private:
    static void* sLibMiInputDispatcherImpl;
    static IMiInputDispatcherStub* sStubImplInstance;
    static std::mutex sLock;
    static bool inited;
    static IMiInputDispatcherStub* getImplInstance();
    static void destroyImplInstance();
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmiinputflinger.so";

public:
    MiInputDispatcherStub();
    virtual ~MiInputDispatcherStub();
    static void init(inputdispatcher::InputDispatcher* inputDispatcher);
    static bool getInputDispatcherAll();
    static void appendDetailInfoForConnectionLocked(const sp<Connection>& connection,
                                              std::string& targetList);

    static void beforeNotifyKey(const NotifyKeyArgs* args);

    static void beforeNotifyMotion(const NotifyMotionArgs* args);

    static void addAnrState(const std::string& time, const std::string& anrState);
    static void dump(std::string& dump);
    // gesture start
    static bool needSkipWindowLocked(const android::gui::WindowInfo& windowInfo);
    static bool needSkipDispatchToSpyWindow();
    static void beforePointerEventFindTargetsLocked(nsecs_t currentTime, const MotionEntry* entry,
                                                    nsecs_t* nextWakeupTime);
    static void checkHotDownTimeoutLocked(nsecs_t* nextWakeupTime);
    static void afterPointerEventFindTouchedWindowTargetsLocked(MotionEntry* entry);
    // gesture end
    static bool needPokeUserActivityLocked(const EventEntry& eventEntry);
    static void afterPublishEvent(status_t status, const sp<Connection>& connection,
                                  const DispatchEntry* dispatchEntry);
    static void accelerateMetaShortcutsAction(const int32_t deviceId, const int32_t action,
                                              int32_t& keyCode, int32_t& metaState);
    static void modifyDeviceIdBeforeInjectEventInitialize(const int32_t flags,
                                                          int32_t& resolvedDeviceId,
                                                          const int32_t realDeviceId);
    static void beforeInjectEventLocked(int32_t injectorPid, const InputEvent* inputEvent);

    static void beforeSetInputDispatchMode(bool enabled, bool frozen);
    static void beforeSetInputFilterEnabled(bool enabled);
    static void beforeSetFocusedApplicationLocked(
            int32_t displayId, std::shared_ptr<InputApplicationHandle> inputApplicationHandle);
    static void beforeSetFocusedWindowLocked(const android::gui::FocusRequest& request);
    static void beforeOnFocusChangedLocked(const FocusResolver::FocusChanges& changes);
    static void changeInjectedKeyEventPolicyFlags(const KeyEvent& keyEvent, uint32_t& policyFlags);
    static void changeInjectedMotionEventPolicyFlags(const MotionEvent& motionEvent,
                                                     uint32_t& policyFlags);
    static void changeInjectedMotionArgsPolicyFlags(const NotifyMotionArgs* motionArgs,
                                                    uint32_t& policyFlags);
    static bool needSkipEventAfterPopupFromInboundQueueLocked(nsecs_t* nextWakeupTime);
    static void addExtraTargetFlagLocked(const sp<android::gui::WindowInfoHandle>& windowHandle,
                                         int32_t x, int32_t y, int32_t& targetFlag);
    static void addMotionExtraResolvedFlagLocked(const std::unique_ptr<DispatchEntry>& dispatchEntry);
    static void afterFindTouchedWindowAtDownActionLocked(
            const sp<android::gui::WindowInfoHandle>& newTouchedWindowHandle,
            const MotionEntry& entry);
    static void whileAddGlobalMonitoringTargetsLocked(const Monitor& monitor,
                                                      InputTarget& inputTarget);
    static bool isOnewayMode();
    static bool isOnewayEvent(const EventEntry& eventEntry);
    static void addCallbackForOnewayMode(const sp<Connection>& connection, bool output);
    static bool isBerserkMode();
    // MIUI ADD: START Activity Embedding
    static bool isNeedMiuiEmbeddedEventMapping(const InputTarget& inputTarget);
    static std::unique_ptr<DispatchEntry> creatEmbeddedEventMappingEntry(std::unique_ptr<MotionEntry>& combinedMotionEntry, int32_t inputTargetFlags, const InputTarget& inputTarget);
    static bool addEmbeddedEventMappingPointers(InputTarget& inputTarget, BitSet32 pointerIds, const android::gui::WindowInfo* windowInfo);
    static void embeddedEventMapping(DispatchEntry* dispatchEntry, const MotionEntry& motionEntry, const PointerCoords* &usingCoords);
    // END
    static void beforeTransferTouchFocusLocked(const sp<IBinder>& fromToken,
                                               const sp<IBinder>& toToken, bool isDragDrop);
};
} // namespace android
