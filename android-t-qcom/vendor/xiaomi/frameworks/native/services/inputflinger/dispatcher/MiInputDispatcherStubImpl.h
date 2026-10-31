#pragma once
//because we can't require mLock (in dispatcher private member), we disable thread-safety-analysis.
#pragma clang diagnostic ignored "-Wthread-safety-analysis"

#include <ui/Region.h>
#include <input/Input.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include "dispatcher/Connection.h"
#include "IMiInputDispatcherStub.h"
#include "dispatcher/InputDispatcher.h"
#include "miinput/MiInputManager.h"
constexpr size_t MI_HISTORY_ANR_SIZE = 3;

namespace android {

using namespace inputdispatcher;

class MiInputDispatcherStubImpl : public IMiInputDispatcherStub {
private:
    InputDispatcher* mInputDispatcher = nullptr;
    MiInputManager* mMiInputManager = nullptr;
    int mMoveCount = 0;
    std::string dumpRegion(const Region& region);
    std::map<std::string, std::string> mHistoryAnr;

    // gesture
    enum GestureStatus {
        GESTURE_NOT_INIT = 0,
        GESTURE_NOT_DECIDE_BOTTOM = 1,
        GESTURE_NOT_DECIDE_LEFT = 2,
        GESTURE_NOT_DECIDE_RIGHT = 3,
        GESTURE_YES_BOTTOM = 4,
        GESTURE_YES_LEFT = 5,
        GESTURE_YES_RIGHT = 6,
        GESTURE_NO = 7,
    };

    enum MiuiInputTarget {
        /* 标记事件被发送到了全局手写窗口 */
        FLAG_EVENT_SEND_TO_HANDWRITING_WINDOW = 1 << 30,
        /* 标记窗口被全局手写窗口遮挡 */
        FLAG_WINDOW_IS_OBSCURED_BY_HANDWRITING_WINDOW = 1 << 31,
    };

    GestureStatus mGestureSt = GESTURE_NOT_INIT;
    std::shared_ptr<MotionEntry> mStateDownEntry;
    nsecs_t mGestureTimeOut = 150 * 1000 * 1000; // 150ms
    bool mSkipGestureStubWindows = false;
    bool mSynergyMode = false;
    int32_t mSynergyStatus = 0;
    bool mEnableHover = true;
    bool mSkipMonitor = false;
    bool mNeedUpdateThreadId = true;
    std::thread::id mDispatcherThreadId;
    bool downEntryAddedOnewayFlag = false;

    // MIUI ADD: START Activity Embedding
    PointerCoords actionDownCoords[MAX_POINTERS];
    PointerCoords mappingCoords[MAX_POINTERS];
    // END

    void dispatchHotDown2AppLocked(nsecs_t currentTime, nsecs_t* nextWakeupTime,
                                   int32_t nextAction);
    void copyMotionEntry(std::shared_ptr<MotionEntry> motionEntry);
    // gesture start
    bool isGestureStubWindows(const android::gui::WindowInfo& windowInfo);
    void checkEvent4GestureLocked(nsecs_t currentTime, const MotionEntry* entry,
                                  nsecs_t* nextWakeupTime);
    void addOemFlagLocked(MotionEntry* entry);
    // gesture end
    void checkSynergyInfo(int32_t deviceId);
    void onConfigurationChanged(InputConfigTypeEnum type);

    std::function<void(InputConfigTypeEnum, int32_t)> mOnConfigurationChangedListener =
            [this](InputConfigTypeEnum typeEnum, int32_t changes) { onConfigurationChanged(typeEnum); };

    bool needSkipHoverEventLocked();
    bool isWindowObscuredByHandWritingWindowAtPointLocked(
            const sp<android::gui::WindowInfoHandle>& windowHandle, int32_t x, int32_t y);
    bool needSkipHandWritingWindow(const android::gui::WindowInfo& windowInfo);
    int handleCallbackForOnewayMode(int events, sp<Connection>& connection);

public:
    MiInputDispatcherStubImpl();
    virtual ~MiInputDispatcherStubImpl();
    void init(InputDispatcher* inputDispatcher) override;

    void beforePointerEventFindTargetsLocked(nsecs_t currentTime,
                                             const MotionEntry *entry,
                                             nsecs_t *nextWakeupTime);
    bool getInputDispatcherAll() override;
    void appendDetailInfoForConnectionLocked(const sp<Connection>& connection,
                                             std::string& targetList) override;
    void beforeNotifyKey(const NotifyKeyArgs* args) override;
    void beforeNotifyMotion(const NotifyMotionArgs* args) override;
    void addAnrState(const std::string& time, const std::string& anrState) override;
    void dump(std::string& dump) override;

    // gesture start
    bool needSkipWindowLocked(const android::gui::WindowInfo& windowInfo) override;
    bool needSkipDispatchToSpyWindow() override;

    void checkHotDownTimeoutLocked(nsecs_t* nextWakeupTime) override;
    void afterPointerEventFindTouchedWindowTargetsLocked(MotionEntry* entry) override;
    // gesture end
    bool needPokeUserActivityLocked(const EventEntry& eventEntry) override;
    void afterPublishEvent(status_t status, const sp<Connection>& connection,
                           const DispatchEntry* dispatchEntry) override;
    void accelerateMetaShortcutsAction(const int32_t deviceId, const int32_t action,
                                       int32_t& keyCode, int32_t& metaState) override;
    void modifyDeviceIdBeforeInjectEventInitialize(const int32_t flags, int32_t& resolvedDeviceId,
                                                   const int32_t realDeviceId) override;
    void beforeInjectEventLocked(int32_t injectorPid, const InputEvent* inputEvent) override;

    void beforeSetInputDispatchMode(bool enabled, bool frozen) override;
    void beforeSetInputFilterEnabled(bool enabled) override;
    void beforeSetFocusedApplicationLocked(
            int32_t displayId,
            std::shared_ptr<InputApplicationHandle> inputApplicationHandle) override;
    void beforeSetFocusedWindowLocked(const android::gui::FocusRequest& request) override;
    void beforeOnFocusChangedLocked(const FocusResolver::FocusChanges& changes) override;
    void changeInjectedKeyEventPolicyFlags(const KeyEvent& keyEvent, uint32_t& keyPolicyFlags) override;
    void changeInjectedMotionEventPolicyFlags(const MotionEvent& motionEvent, uint32_t& motionPolicyFlags) override;
    void changeInjectedMotionArgsPolicyFlags(const NotifyMotionArgs* motionArgs, uint32_t& argsPolicyFlags) override;
    bool needSkipEventAfterPopupFromInboundQueueLocked(nsecs_t* nextWakeupTime) override;
    void addExtraTargetFlagLocked(const sp<android::gui::WindowInfoHandle>& windowHandle, int32_t x, int32_t y,
                                  int32_t& targetFlag) override;
    void addMotionExtraResolvedFlagLocked(const std::unique_ptr<DispatchEntry>& dispatchEntry) override;
    void afterFindTouchedWindowAtDownActionLocked(const sp<android::gui::WindowInfoHandle>& newTouchedWindowHandle,
                                               const MotionEntry& entry) override;
    void whileAddGlobalMonitoringTargetsLocked(const Monitor& monitor,
                                               InputTarget& inputTarget) override;
    bool isOnewayMode() override;
    bool isOnewayEvent(const EventEntry& entry) override;
    void addCallbackForOnewayMode(const sp<Connection>& connection, bool output) override;
    bool isBerserkMode() override;
    // MIUI ADD: START Activity Embedding
    bool isNeedMiuiEmbeddedEventMapping(const InputTarget& inputTarget) override;
    std::unique_ptr<DispatchEntry> creatEmbeddedEventMappingEntry(std::unique_ptr<MotionEntry>& combinedMotionEntry, int32_t inputTargetFlags, const InputTarget& inputTarget) override;
    bool addEmbeddedEventMappingPointers(InputTarget& inputTarget, BitSet32 pointerIds, const android::gui::WindowInfo* windowInfo) override;
    void embeddedEventMapping(DispatchEntry* dispatchEntry, const MotionEntry& motionEntry, const PointerCoords* &usingCoords) override;
    // END
    void beforeTransferTouchFocusLocked(const sp<IBinder>& fromToken, const sp<IBinder>& toToken,
                                        bool isDragDrop) override;
private:
    static std::mutex mInstLock;
    static MiInputDispatcherStubImpl* sImplInstance;

public:
    static MiInputDispatcherStubImpl* getInstance();
    const std::unordered_map<int32_t /*displayId*/, android::gui::DisplayInfo>& getDisplayInfos() {
        return mInputDispatcher->getDisplayInfosForStubLock();
    }
};

} // namespace android
