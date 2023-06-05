#pragma once

#include "../dispatcher/Connection.h"
#include "../dispatcher/InputDispatcher.h"

namespace android {

using namespace inputdispatcher;

class IMiInputDispatcherStub {
public:
    IMiInputDispatcherStub(){};

    virtual ~IMiInputDispatcherStub(){};

    virtual void init(inputdispatcher::InputDispatcher* inputDispatcher);

    virtual bool getInputDispatcherAll();

    virtual void appendDetailInfoForConnectionLocked(const sp<Connection>& connection,
                                               std::string& targetList);

    virtual void beforeNotifyKey(const NotifyKeyArgs* args);

    virtual void beforeNotifyMotion(const NotifyMotionArgs* args);

    virtual void addAnrState(const std::string& time, const std::string& anrState);
    virtual void dump(std::string& dump);
    // gesture start
    virtual bool needSkipWindowLocked(const android::gui::WindowInfo& windowInfo);
    virtual bool needSkipDispatchToSpyWindow();
    virtual void beforePointerEventFindTargetsLocked(nsecs_t currentTime, const MotionEntry* entry,
                                                     nsecs_t* nextWakeupTime);
    virtual void checkHotDownTimeoutLocked(nsecs_t* nextWakeupTime);
    virtual void afterPointerEventFindTouchedWindowTargetsLocked(MotionEntry* entry);
    // gesture end
    virtual bool needPokeUserActivityLocked(const EventEntry& eventEntry);
    virtual void afterPublishEvent(status_t status, const sp<Connection>& connection,
                                   const DispatchEntry* dispatchEntry);
    virtual void accelerateMetaShortcutsAction(const int32_t deviceId, const int32_t action,
                                               int32_t& keyCode, int32_t& metaState);
    virtual void modifyDeviceIdBeforeInjectEventInitialize(const int32_t flags,
                                                           int32_t& resolvedDeviceId,
                                                           const int32_t realDeviceId);
    virtual void beforeInjectEventLocked(int32_t injectorPid, const InputEvent* inputEvent);

    virtual void beforeSetInputDispatchMode(bool enabled, bool frozen);
    virtual void beforeSetInputFilterEnabled(bool enabled);
    virtual void beforeSetFocusedApplicationLocked(
            int32_t displayId, std::shared_ptr<InputApplicationHandle> inputApplicationHandle);
    virtual void beforeSetFocusedWindowLocked(const android::gui::FocusRequest& request);
    virtual void beforeOnFocusChangedLocked(const FocusResolver::FocusChanges& changes);
    virtual void changeInjectedKeyEventPolicyFlags(const KeyEvent& keyEvent, uint32_t& policyFlags);
    virtual void changeInjectedMotionEventPolicyFlags(const MotionEvent& motionEvent,
                                                      uint32_t& policyFlags);
    virtual void changeInjectedMotionArgsPolicyFlags(const NotifyMotionArgs* motionArgs,
                                                     uint32_t& policyFlags);
    virtual bool needSkipEventAfterPopupFromInboundQueueLocked(nsecs_t* nextWakeupTime);
    virtual void addExtraTargetFlagLocked(const sp<android::gui::WindowInfoHandle>& windowHandle,
                                          int32_t x, int32_t y, int32_t& targetFlag);
    virtual void addMotionExtraResolvedFlagLocked(const std::unique_ptr<DispatchEntry>& dispatchEntry);
    virtual void afterFindTouchedWindowAtDownActionLocked(
            const sp<android::gui::WindowInfoHandle>& newTouchedWindowHandle,
            const MotionEntry& entry);
    virtual void whileAddGlobalMonitoringTargetsLocked(const Monitor& monitor,
                                                       InputTarget& inputTarget);
    virtual bool isOnewayMode();
    virtual bool isOnewayEvent(const EventEntry& entry);
    virtual void addCallbackForOnewayMode(const sp<Connection>& connection, bool output);
    virtual bool isBerserkMode();
    // MIUI ADD: START Activity Embedding
    virtual bool isNeedMiuiEmbeddedEventMapping(const InputTarget& inputTarget);
    virtual std::unique_ptr<DispatchEntry> creatEmbeddedEventMappingEntry(std::unique_ptr<MotionEntry>& combinedMotionEntry, int32_t inputTargetFlags, const InputTarget& inputTarget);
    virtual bool addEmbeddedEventMappingPointers(InputTarget& inputTarget, BitSet32 pointerIds, const android::gui::WindowInfo* windowInfo);
    virtual void embeddedEventMapping(DispatchEntry* dispatchEntry, const MotionEntry& motionEntry, const PointerCoords* &usingCoords);
    // END
    virtual void beforeTransferTouchFocusLocked(const sp<IBinder>& fromToken,
                                                const sp<IBinder>& toToken, bool isDragDrop);
};
} // namespace android