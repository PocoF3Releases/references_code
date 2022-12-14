#define LOG_TAG "MiInputDispatcher"

#include "MiInputDispatcherStubImpl.h"
#include <android-base/stringprintf.h>
#include <binder/IServiceManager.h>
#include <gui/WindowInfo.h>
#include <log/log.h>
#include <log/log_event_list.h>
#include <unistd.h>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cstddef>
#include <ctime>
#include <queue>
#include <sstream>
#include "dispatcher/Connection.h"
#include "IMiInputDispatcherStub.h"
#include "dispatcher/InputDispatcher.h"
#include "input/InputTransport.h"
#include "miinput/InputDebugConfig.h"

#define ALOGD_EXT(...) __android_log_print(ANDROID_LOG_DEBUG, "MIUIInput", __VA_ARGS__)
#undef ALOGD
#define ALOGD(...)                                \
    if (mMiInputManager->getInputDispatcherAll()) \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using android::base::StringPrintf;
using android::gui::DisplayInfo;
using android::gui::FocusRequest;
using android::gui::TouchOcclusionMode;
using android::gui::WindowInfo;
using android::gui::WindowInfoHandle;
using android::os::BlockUntrustedTouchesMode;
using android::os::IInputConstants;
using android::os::InputEventInjectionResult;
using android::os::InputEventInjectionSync;

static const bool kBerserkMode = property_get_bool("persist.berserk.mode.support", false);

inline const char* toString(bool value) {
    return value ? "true" : "false";
}

inline nsecs_t now() {
    return systemTime(SYSTEM_TIME_MONOTONIC);
}

// Event log tags. See EventLogTags.logtags for reference.
constexpr int LOGTAG_INPUT_FOCUS = 62001;
// MIUI ADD
constexpr int LOGTAG_INPUT_DISPATCHER = 62198;

namespace android {

MiInputDispatcherStubImpl* MiInputDispatcherStubImpl::sImplInstance = NULL;
std::mutex MiInputDispatcherStubImpl::mInstLock;

MiInputDispatcherStubImpl::MiInputDispatcherStubImpl() {
    mMiInputManager = MiInputManager::getInstance();
    mMiInputManager->addOnConfigurationChangedListener(
            mOnConfigurationChangedListener);
}

MiInputDispatcherStubImpl::~MiInputDispatcherStubImpl() {}


MiInputDispatcherStubImpl* MiInputDispatcherStubImpl::getInstance() {
    std::lock_guard<std::mutex> lock(mInstLock);
    if (!sImplInstance) {
        sImplInstance = new MiInputDispatcherStubImpl();
    }
    return sImplInstance;
}

void MiInputDispatcherStubImpl::init(InputDispatcher* inputDispatcher) {
    this->mInputDispatcher = inputDispatcher;
}

bool MiInputDispatcherStubImpl::getInputDispatcherAll() {
    return mMiInputManager->getInputDispatcherAll();
}

std::string MiInputDispatcherStubImpl::dumpRegion(const Region& region) {
    if (region.isEmpty()) {
        return "<empty>";
    }

    std::string dump;
    bool first = true;
    Region::const_iterator cur = region.begin();
    Region::const_iterator const tail = region.end();
    while (cur != tail) {
        if (first) {
            first = false;
        } else {
            dump += "|";
        }
        dump += StringPrintf("[%d,%d][%d,%d]", cur->left, cur->top, cur->right, cur->bottom);
        cur++;
    }
    return dump;
}

void MiInputDispatcherStubImpl::appendDetailInfoForConnectionLocked(
        const sp<Connection>& connection, std::string& targetList) {
    if (mInputDispatcher == nullptr || connection->monitor) {
        return;
    }
    const sp<WindowInfoHandle> windowHandle = mInputDispatcher->getWindowHandleForStubLocked(
            connection->inputChannel->getConnectionToken());
    if (windowHandle == nullptr) {
        return;
    }
    const WindowInfo* windowInfo = windowHandle->getInfo();
    if (windowInfo->isSpy() || windowInfo->layoutParamsType == WindowInfo::Type::WALLPAPER) {
        return;
    }
    auto flags = windowInfo->layoutParamsFlags;
    auto inputConfig = windowInfo->inputConfig;
    targetList += "{touchableRegion=" + dumpRegion(windowInfo->touchableRegion) +
            ", visible=" + toString(!inputConfig.test(WindowInfo::InputConfig::NOT_VISIBLE)) +
            ", trustedOverlay=" +
            toString(inputConfig.test(WindowInfo::InputConfig::TRUSTED_OVERLAY)) +
            ", flags[NOT_TOUCHABLE=" +
            toString(inputConfig.test(WindowInfo::InputConfig::NOT_TOUCHABLE)) +
            ", NOT_FOCUSABLE=" +
            toString(inputConfig.test(WindowInfo::InputConfig::NOT_FOCUSABLE)) +
            ", NOT_TOUCH_MODAL=" + toString(flags.test(WindowInfo::Flag::NOT_TOUCH_MODAL)) + "]}, ";
}

void MiInputDispatcherStubImpl::beforeNotifyKey(const NotifyKeyArgs* args) {
    bool needLog = false;
    if (mMiInputManager->getInputDispatcherDetail()) {
        needLog = true;
    } else if (mMiInputManager->getInputDispatcherMajor()) {
        needLog = args->keyCode == AKEYCODE_VOLUME_DOWN || args->keyCode == AKEYCODE_VOLUME_UP ||
                args->keyCode == AKEYCODE_POWER || args->keyCode == AKEYCODE_BACK ||
                args->keyCode == AKEYCODE_HOME;
    }
    if (!needLog) {
        return;
    }
    ALOGD_EXT("[KeyEvent] notifyKey - eventTime=%" PRId64
              ", deviceId=%d, source=0x%x, policyFlags=0x%x, action=0x%x, "
              "flags=0x%x, keyCode=0x%x, scanCode=0x%x, metaState=0x%x, downTime=%" PRId64,
              args->eventTime, args->deviceId, args->source, args->policyFlags, args->action,
              args->flags, args->keyCode, args->scanCode, args->metaState, args->downTime);
}

void MiInputDispatcherStubImpl::beforeNotifyMotion(const NotifyMotionArgs* args) {
    if (mMiInputManager->getInputDispatcherDetail()) {
        ALOGD_EXT("[MotionEvent] notifyMotion - eventTime=%" PRId64
                  ", deviceId=%d, source=0x%x, policyFlags=0x%x, "
                  "action=0x%x, actionButton=0x%x, flags=0x%x, metaState=0x%x, buttonState=0x%x,"
                  "edgeFlags=0x%x, xPrecision=%f, yPrecision=%f, downTime=%" PRId64,
                  args->eventTime, args->deviceId, args->source, args->policyFlags, args->action,
                  args->actionButton, args->flags, args->metaState, args->buttonState,
                  args->edgeFlags, args->xPrecision, args->yPrecision, args->downTime);
        for (uint32_t i = 0; i < args->pointerCount; i++) {
            ALOGD_EXT("  Pointer %d: id=%d, toolType=%d, "
                      "x=%f, y=%f, pressure=%f, size=%f, "
                      "touchMajor=%f, touchMinor=%f, toolMajor=%f, toolMinor=%f, "
                      "orientation=%f",
                      i, args->pointerProperties[i].id, args->pointerProperties[i].toolType,
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_X),
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_Y),
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE),
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE),
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR),
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR),
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MAJOR),
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MINOR),
                      args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION));
        }
        return;
    }
    if (mMiInputManager->getInputDispatcherMajor()) {
        int32_t maskedAction = args->action & AMOTION_EVENT_ACTION_MASK;
        if (maskedAction == AMOTION_EVENT_ACTION_MOVE) {
            mMoveCount++;
        }
        if (maskedAction == AMOTION_EVENT_ACTION_DOWN || maskedAction == AMOTION_EVENT_ACTION_UP ||
            maskedAction == AMOTION_EVENT_ACTION_POINTER_DOWN ||
            maskedAction == AMOTION_EVENT_ACTION_POINTER_UP) {
            ALOGD_EXT("[MotionEvent] notifyMotion - eventTime=%" PRId64
                      ", deviceId=%d, source=0x%x, policyFlags=0x%x, "
                      "action=0x%x, flags=0x%x, downTime=%" PRId64 ", moveCount:%d",
                      args->eventTime, args->deviceId, args->source, args->policyFlags,
                      args->action, args->flags, args->downTime, mMoveCount);
            if (maskedAction == AMOTION_EVENT_ACTION_UP) {
                mMoveCount = 0;
            }
        }
    }
}

void MiInputDispatcherStubImpl::addAnrState(const std::string& time, const std::string& anrState) {
    if (mHistoryAnr.size() >= MI_HISTORY_ANR_SIZE) {
        mHistoryAnr.erase(mHistoryAnr.begin());
    }
    mHistoryAnr[time] = anrState;
}

void MiInputDispatcherStubImpl::dump(std::string& dump) {
    dump += "\nInput Dispatcher State at time of last three times ANR:\n";
    for (auto& it : mHistoryAnr) {
        std::string history = it.second;
        dump += it.second;
    }
}

// gesture start
bool MiInputDispatcherStubImpl::isGestureStubWindows(const WindowInfo& windowInfo) {
    bool touchHotWinBottom = android::base::EndsWith(windowInfo.name.c_str(), "GestureStub");
    bool touchHotWinLeft = android::base::EndsWith(windowInfo.name.c_str(), "GestureStubLeft");
    bool touchHotWinRight = android::base::EndsWith(windowInfo.name.c_str(), "GestureStubBottom") |
            android::base::EndsWith(windowInfo.name.c_str(), "GestureStubRight");
    bool touchHotWin = touchHotWinBottom | touchHotWinLeft | touchHotWinRight;
    if (!mSkipGestureStubWindows && touchHotWin) {
        if (touchHotWinBottom) {
            mGestureSt = GESTURE_NOT_DECIDE_BOTTOM;
        }
        if (touchHotWinLeft) {
            mGestureSt = GESTURE_NOT_DECIDE_LEFT;
        }
        if (touchHotWinRight) {
            mGestureSt = GESTURE_NOT_DECIDE_RIGHT;
        }
        ALOGD("windowHandle:%s, isGestureStubWindows:%d, mGestureSt:%d", windowInfo.name.c_str(),
              touchHotWin, mGestureSt);
    }
    return touchHotWin;
}

bool MiInputDispatcherStubImpl::needSkipWindowLocked(const android::gui::WindowInfo& windowInfo) {
    if (needSkipHandWritingWindow(windowInfo)) {
        return true;
    }
    // gesture stub
    if (isGestureStubWindows(windowInfo)) {
        if (mSkipGestureStubWindows) {
            return true;
        } else {
            copyMotionEntry(std::static_pointer_cast<MotionEntry>(
                    mInputDispatcher->getPendingEventForStubLocked()));
        }
    }
    return false;
}

bool MiInputDispatcherStubImpl::needSkipDispatchToSpyWindow() {
    // do not dispatch hot down to monitor
    return mSkipMonitor || mSkipGestureStubWindows;
}

void MiInputDispatcherStubImpl::copyMotionEntry(std::shared_ptr<MotionEntry> motionEntry) {
    mStateDownEntry =
            std::make_shared<MotionEntry>(motionEntry->id, motionEntry->eventTime,
                                          motionEntry->deviceId, motionEntry->source,
                                          motionEntry->displayId, motionEntry->policyFlags,
                                          motionEntry->action, motionEntry->actionButton,
                                          motionEntry->flags, motionEntry->metaState,
                                          motionEntry->buttonState, motionEntry->classification,
                                          motionEntry->edgeFlags, motionEntry->xPrecision,
                                          motionEntry->yPrecision, motionEntry->xCursorPosition,
                                          motionEntry->yCursorPosition, motionEntry->downTime,
                                          motionEntry->pointerCount, motionEntry->pointerProperties,
                                          motionEntry->pointerCoords);
}
void MiInputDispatcherStubImpl::beforePointerEventFindTargetsLocked(
        nsecs_t currentTime, const MotionEntry* entry, nsecs_t* nextWakeupTime) {
    const bool isPointerEvent = isFromSource(entry->source, AINPUT_SOURCE_CLASS_POINTER);
    if (isPointerEvent) {
        checkEvent4GestureLocked(currentTime, entry, nextWakeupTime);
    } else {
        // Non touch event.  (eg. trackball)
        // need dispatch to monitor
        mSkipMonitor = false;
    }
}

void MiInputDispatcherStubImpl::checkEvent4GestureLocked(nsecs_t currentTime,
                                                         const MotionEntry* entry,
                                                         nsecs_t* nextWakeupTime) {
    if (mGestureSt != GESTURE_NOT_DECIDE_BOTTOM && mGestureSt != GESTURE_NOT_DECIDE_LEFT &&
        mGestureSt != GESTURE_NOT_DECIDE_RIGHT) {
        return;
    }

    ui::Transform transform;
    if (const auto it = mInputDispatcher->getDisplayInfosForStubLock().find(entry->displayId);
           it != mInputDispatcher->getDisplayInfosForStubLock().end()) {
        transform = it->second.transform;
    }
    vec2 down = transform.transform(mStateDownEntry->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X),
        mStateDownEntry->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y));
    vec2 now = transform.transform(entry->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X),
        entry->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y));
    int32_t downX = int32_t(down.x);
    int32_t downY = int32_t(down.y);
    int32_t nowX = int32_t(now.x);
    int32_t nowY = int32_t(now.y);
    int32_t deltaX = std::abs(nowX - downX);
    int32_t deltaY = std::abs(nowY - downY);
    nsecs_t deltaT = currentTime - mStateDownEntry->eventTime;
    int32_t nowAction = entry->action & AMOTION_EVENT_ACTION_MASK;
    if (nowAction == AMOTION_EVENT_ACTION_MOVE) {
        if (deltaT > mGestureTimeOut) {
            mGestureSt = GESTURE_NO;
        } else if (mGestureSt == GESTURE_NOT_DECIDE_BOTTOM) {
            // bottom. do not need check rotate.
            if (deltaX > 2 * deltaY && deltaX > 8) {
                // mTouchSlop  from ViewConfiguration.java
                mGestureSt = GESTURE_NO;
            } else if (deltaY > 20) {
                mGestureSt = GESTURE_YES_BOTTOM;
            }
        } else if (mGestureSt == GESTURE_NOT_DECIDE_LEFT ||
                   mGestureSt == GESTURE_NOT_DECIDE_RIGHT) {
            // left or right
            if (deltaY > 2 * deltaX) {
                mGestureSt = GESTURE_NO;
            } else if ((2 * deltaX >= deltaY) && (deltaX >= 20)) {
                if (mGestureSt == GESTURE_NOT_DECIDE_LEFT) {
                    mGestureSt = GESTURE_YES_LEFT;
                } else {
                    mGestureSt = GESTURE_YES_RIGHT;
                }
            }
        }
    } else if (nowAction == AMOTION_EVENT_ACTION_UP ||
               nowAction == AMOTION_EVENT_ACTION_POINTER_DOWN ||
               nowAction == AMOTION_EVENT_ACTION_CANCEL) {
        //  UP PointerDown Cancel.
        // deltaT < mGestureTimeOut && deltaX <= 30 && deltaY <= 30; not reasonable.
        mGestureSt = GESTURE_NO;
    }

    if (mMiInputManager->getInputDispatcherDetail() ||
        mMiInputManager->getInputDispatcherMajor()) {
        ALOGI("checkEvent4GestureLocked, downTime:%" PRId64 ", downX:%d, downY:%d,"
              " nowAct:%x nowX=%d, nowY=%d, nowTime:%" PRId64
              ", deltaX:%d deltaY:%d deltaT:%" PRId64 ", GestureSt:%d",
              mStateDownEntry->eventTime, downX, downY, entry->action, nowX, nowY, currentTime,
              deltaX, deltaY, deltaT, mGestureSt);
    }
    // do something for gesture result.
    if (mGestureSt == GESTURE_NO) {
        dispatchHotDown2AppLocked(currentTime, nextWakeupTime, nowAction);
    } else if (mGestureSt == GESTURE_YES_BOTTOM || mGestureSt == GESTURE_YES_LEFT ||
               mGestureSt == GESTURE_YES_RIGHT) {
        // MotionEntry not need release, shared_ptr
    }
}

void MiInputDispatcherStubImpl::checkHotDownTimeoutLocked(nsecs_t* nextWakeupTime) {
    if (mGestureSt == GESTURE_NOT_DECIDE_BOTTOM || mGestureSt == GESTURE_NOT_DECIDE_LEFT ||
        mGestureSt == GESTURE_NOT_DECIDE_RIGHT) {
        nsecs_t tempTime = now();
        nsecs_t deltaT = tempTime - mStateDownEntry->eventTime;
        nsecs_t nextCheckTimeout = (mGestureTimeOut - deltaT) + tempTime;
        if (deltaT > mGestureTimeOut) {
            mGestureSt = GESTURE_NO;
            dispatchHotDown2AppLocked(tempTime, nextWakeupTime, 0);
        } else {
            *nextWakeupTime = std::min(*nextWakeupTime, nextCheckTimeout);
        }
        ALOGD("checkHotDownTimeoutLocked, deltaT=%" PRId64 ", nextWakeupTime=%" PRId64
              " mGestureSt=%d",
              deltaT, *nextWakeupTime, mGestureSt);
    }
}

void MiInputDispatcherStubImpl::dispatchHotDown2AppLocked(nsecs_t currentTime,
                                                          nsecs_t* nextWakeupTime,
                                                          int32_t nextAction) {
    // dispachGestureDown.
    std::vector<InputTarget> inputTargets;
    bool conflictingPointerActions = false;
    {
        mSkipGestureStubWindows = true;
        InputEventInjectionResult injectionDownResult =
                mInputDispatcher->findTouchedWindowTargetsForStubLocked(currentTime,
                                                                        *mStateDownEntry,
                                                                        inputTargets,
                                                                        nextWakeupTime,
                                                                        &conflictingPointerActions);
        mSkipGestureStubWindows = false;
        mInputDispatcher->setInjectionResultForStub(*mStateDownEntry, injectionDownResult);
        if (injectionDownResult != InputEventInjectionResult::SUCCEEDED) {
            if (injectionDownResult != InputEventInjectionResult::TARGET_MISMATCH) {
                CancelationOptions::Mode mode(CancelationOptions::CANCEL_POINTER_EVENTS);
                CancelationOptions options(mode, "input event injection failed");
                mInputDispatcher->synthesizeCancelationEventsForMonitorsForStubLocked(options);
            }
            ALOGD("dispatchHotDown2AppLocked , FAILED, return!!!");
            return;
        }

        mInputDispatcher
                ->addGlobalMonitoringTargetsForStubLocked(inputTargets,
                                                          mInputDispatcher
                                                                  ->getTargetDisplayIdForStub(
                                                                          *mStateDownEntry));

        // Dispatch the motion.
        if (conflictingPointerActions) {
            CancelationOptions options(CancelationOptions::CANCEL_POINTER_EVENTS,
                                       "conflicting pointer actions");
            mInputDispatcher->synthesizeCancelationEventsForAllConnectionsForStubLocked(options);
        }
        if (!mStateDownEntry->dispatchInProgress) {
            mStateDownEntry->dispatchInProgress = true;
        }

        if (mMiInputManager->getInputDispatcherDetail()) {
            mStateDownEntry->flags |= AMOTION_EVENT_FLAG_DEBUGINPUT_DETAIL;
        } else if (mMiInputManager->getInputDispatcherDetail()) {
            mStateDownEntry->flags |= AMOTION_EVENT_FLAG_DEBUGINPUT_MAJAR;
        }
        if (mMiInputManager->getInputTransportAll()) {
            mStateDownEntry->flags |= AMOTION_EVENT_FLAG_DEBUGINPUT_TRANSPORT_ALL;
        }

        mInputDispatcher->dispatchEventForStubLocked(currentTime,
                                                     std::make_shared<MotionEntry>(
                                                             *mStateDownEntry),
                                                     inputTargets);

        if (nextAction == AMOTION_EVENT_ACTION_UP) {
            // sleep 32ms; because com.tencent.tmgp.pubgmhd do not response.
            usleep(32 * 1000);
        }
    }
}

void MiInputDispatcherStubImpl::addOemFlagLocked(MotionEntry* entry) {
    ALOGD("addGestureFlagLocked, flags=0x%x, action=0x%x, mGestureSt:%d", entry->flags,
          entry->action, mGestureSt);
    switch (mGestureSt) {
        case GESTURE_NOT_INIT:
            // do not need add any flag.
            break;
        case GESTURE_NOT_DECIDE_BOTTOM:
        case GESTURE_NOT_DECIDE_LEFT:
        case GESTURE_NOT_DECIDE_RIGHT:
            entry->flags |= AMOTION_EVENT_FLAG_GESTURE_PENDING;
            break;
        case GESTURE_YES_BOTTOM:
            entry->flags |= AMOTION_EVENT_FLAG_GESTURE_BOTTOM;
            break;
        case GESTURE_YES_LEFT:
            entry->flags |= AMOTION_EVENT_FLAG_GESTURE_LEFT;
            break;
        case GESTURE_YES_RIGHT:
            entry->flags |= AMOTION_EVENT_FLAG_GESTURE_RIGHT;
            break;
        case GESTURE_NO:
            entry->flags |= AMOTION_EVENT_FLAG_GESTURE_NO;
            break;
    }

    if (mMiInputManager->getInputDispatcherDetail()) {
        entry->flags |= AMOTION_EVENT_FLAG_DEBUGINPUT_DETAIL;
    } else if (mMiInputManager->getInputDispatcherMajor()) {
        entry->flags |= AMOTION_EVENT_FLAG_DEBUGINPUT_MAJAR;
    }
    if (mMiInputManager->getInputTransportAll()) {
        entry->flags |= AMOTION_EVENT_FLAG_DEBUGINPUT_TRANSPORT_ALL;
    }

    if (entry->action == AMOTION_EVENT_ACTION_DOWN) {
        if (isOnewayMode()) {
            entry->flags |= AMOTION_EVENT_FLAG_ONEWAY;
            downEntryAddedOnewayFlag = true;
        }
    } else if (downEntryAddedOnewayFlag) {
        entry->flags |= AMOTION_EVENT_FLAG_ONEWAY;
        if (entry->action == AMOTION_EVENT_ACTION_UP ||
            entry->action == AMOTION_EVENT_ACTION_CANCEL) {
            downEntryAddedOnewayFlag = false;
        }
    }
}
// gesture end

void MiInputDispatcherStubImpl::checkSynergyInfo(int32_t deviceId) {
    int32_t tempSynergy = (deviceId == -100) ? 1 : 0;
    int32_t synergyMode = mMiInputManager->getSynergyMode();
    ALOGD("checkSynergyInfo mode:%d info:%d %d, id:%d", synergyMode, mSynergyStatus, tempSynergy,
          deviceId);
    if (synergyMode && (mSynergyStatus != tempSynergy)) {
        mSynergyStatus = tempSynergy;
        sp<IBinder> service =
                android::defaultServiceManager()->getService(String16("input_method"));
        Parcel data;
        Parcel reply;
        data.writeInterfaceToken(String16("com.android.internal.view.IInputMethodManager"));
        data.writeInt32(tempSynergy);
        int32_t CODE_SYNERGY_OPERATE = 0x00ffffff - 4; // IBinder.LAST_CALL_TRANSACTION -4;
        service->transact(CODE_SYNERGY_OPERATE, data, &reply, 0);
        reply.readExceptionCode();
    }
}

void MiInputDispatcherStubImpl::afterPointerEventFindTouchedWindowTargetsLocked(
        MotionEntry* entry) {
    addOemFlagLocked(entry);
    checkSynergyInfo(entry->deviceId);
}

bool MiInputDispatcherStubImpl::needPokeUserActivityLocked(const EventEntry& eventEntry) {
    if (eventEntry.type != EventEntry::Type::MOTION) {
        return true;
    }
    const MotionEntry& motionEntry = static_cast<const MotionEntry&>(eventEntry);
    return !(mMiInputManager->getSynergyMode() && motionEntry.deviceId == -100);
}

void MiInputDispatcherStubImpl::afterPublishEvent(status_t status, const sp<Connection>& connection,
                                                  const DispatchEntry* dispatchEntry) {
    if (status || connection->monitor) {
        return;
    }
    const EventEntry& eventEntry = *(dispatchEntry->eventEntry);
    if (eventEntry.type == EventEntry::Type::KEY) {
        const KeyEntry& keyEntry = static_cast<const KeyEntry&>(eventEntry);
        ALOGD_EXT("[KeyEvent] publisher action:0x%x,  keyCode:%d , %ld,channel '%s' ",
                  dispatchEntry->resolvedAction, keyEntry.keyCode, (long)ns2ms(keyEntry.eventTime),
                  connection->getInputChannelName().c_str());
    } else if (eventEntry.type == EventEntry::Type::MOTION) {
        if (dispatchEntry->resolvedAction == AMOTION_EVENT_ACTION_MOVE ||
            dispatchEntry->resolvedAction == AMOTION_EVENT_ACTION_SCROLL) {
            return;
        }
        const MotionEntry& motionEntry = static_cast<const MotionEntry&>(eventEntry);
        ALOGD_EXT("[MotionEvent] publisher action=0x%x, %ld, channel '%s'",
                  dispatchEntry->resolvedAction, (long)ns2ms(motionEntry.eventTime),
                  connection->getInputChannelName().c_str());
    }
}

void MiInputDispatcherStubImpl::accelerateMetaShortcutsAction(const int32_t deviceId,
                                                              const int32_t action,
                                                              int32_t& keyCode,
                                                              int32_t& metaState) {
    bool isOnPadMode = mMiInputManager->isPadMode();
    if (isOnPadMode && keyCode == AKEYCODE_ESCAPE && (metaState & AMETA_SHIFT_ON) == 0 &&
               action == AKEY_EVENT_ACTION_DOWN &&
               (!mMiInputManager->isInputMethodShown() ||
                mMiInputManager->isCustomizeInputMethod())) {
        int32_t newKeyCode = AKEYCODE_BACK;
        mInputDispatcher->addToReplacedKeysForStub(keyCode, deviceId, newKeyCode);
        keyCode = newKeyCode;
    } else if (isOnPadMode && keyCode == AKEYCODE_ESCAPE && (metaState & AMETA_SHIFT_ON) == 0 &&
               action == AKEY_EVENT_ACTION_DOWN &&
               (mMiInputManager->isInputMethodShown() &&
                !mMiInputManager->isCustomizeInputMethod())) {
        int32_t newKeyCode = AKEYCODE_GRAVE;
        mInputDispatcher->addToReplacedKeysForStub(keyCode, deviceId, newKeyCode);
        keyCode = newKeyCode;
    }
}

void MiInputDispatcherStubImpl::modifyDeviceIdBeforeInjectEventInitialize(
        const int32_t flags, int32_t& resolvedDeviceId, const int32_t realDeviceId) {
    if ((flags & AMOTION_EVENT_FLAG_KEEP_DEVID) == AMOTION_EVENT_FLAG_KEEP_DEVID) {
        resolvedDeviceId = realDeviceId;
    }
}

void MiInputDispatcherStubImpl::beforeInjectEventLocked(int32_t injectorPid,
                                                  const InputEvent* inputEvent) {
    if (mInputDispatcher->isInputFilterEnabledForStubLocked()) {
        return;
    }
    int32_t type = inputEvent->getType();
    if (type == AINPUT_EVENT_TYPE_KEY) {
        const KeyEvent& keyEvent = static_cast<const KeyEvent&>(*inputEvent);
        int32_t action = keyEvent.getAction();
        int32_t keyCode = keyEvent.getKeyCode();
        ALOGD_EXT("Input event injection from pid %d, KeyEvent:%s, keycode:%d", injectorPid,
                  KeyEvent::actionToString(action), keyCode);
    } else if (type == AINPUT_EVENT_TYPE_MOTION) {
        const MotionEvent& motionEvent = static_cast<const MotionEvent&>(*inputEvent);
        int32_t action = motionEvent.getAction();
        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_UP) {
            ALOGD_EXT("Input event injection from pid %d, MotionEvent: %s", injectorPid,
                      MotionEvent::actionToString(action).c_str());
        }
    }
}

void MiInputDispatcherStubImpl::beforeSetInputDispatchMode(bool enabled, bool frozen) {
    std::string message = "Dispatcher mode: enabled = " + std::to_string(enabled) +
            " frozen = " + std::to_string(frozen);
    android_log_event_list(LOGTAG_INPUT_DISPATCHER) << message << LOG_ID_EVENTS;
}

void MiInputDispatcherStubImpl::beforeSetInputFilterEnabled(bool enabled) {
    std::string message = "Dispatcher InputFilter: enabled = " + std::to_string(enabled);
    android_log_event_list(LOGTAG_INPUT_DISPATCHER) << message << LOG_ID_EVENTS;
}

void MiInputDispatcherStubImpl::beforeSetFocusedApplicationLocked(
        int32_t displayId, std::shared_ptr<InputApplicationHandle> inputApplicationHandle) {
    if (inputApplicationHandle == nullptr) {
        return;
    }
    std::string message = std::string("displayId :") + std::to_string(displayId) +
            std::string(", foucsApplication has changed to ") +
            inputApplicationHandle->getName().c_str();
    android_log_event_list(LOGTAG_INPUT_FOCUS) << message << LOG_ID_EVENTS;
}

void MiInputDispatcherStubImpl::beforeSetFocusedWindowLocked(const FocusRequest& request) {
    std::string message = std::string("Focus receive :") + request.windowName.c_str();
    std::string reason = std::string("reason=setFocusedWindow");
    android_log_event_list(LOGTAG_INPUT_FOCUS) << message << reason << LOG_ID_EVENTS;
}

void MiInputDispatcherStubImpl::beforeOnFocusChangedLocked(
        const FocusResolver::FocusChanges& changes) {
    if (changes.newFocus) {
        mInputDispatcher->resetNoFocusedWindowTimeoutForStubLocked();
    }
}

void MiInputDispatcherStubImpl::onConfigurationChanged(InputConfigTypeEnum config) {
    if (config == INPUT_CONFIG_TYPE_DEBUG_CONFIG) {
        if (mInputDispatcher) {
            InputChannel::switchInputLog(mMiInputManager->getInputDispatcherDetail());
        }
    } else if (config == INPUT_CONFIG_TYPE_COMMON_CONFIG){
        mSynergyMode = mMiInputManager->mInputCommonConfig->mSynergyMode;
    }
}

void MiInputDispatcherStubImpl::changeInjectedKeyEventPolicyFlags(const KeyEvent& keyEvent, uint32_t& keyPolicyFlags) {
    if((keyPolicyFlags & POLICY_FLAG_INJECTED) && mSynergyMode) {
        keyPolicyFlags |= POLICY_FLAG_PASS_TO_USER;
    }
}

void MiInputDispatcherStubImpl::changeInjectedMotionEventPolicyFlags(const MotionEvent& motionEvent, uint32_t& motionPolicyFlags) {
    if((motionPolicyFlags & POLICY_FLAG_INJECTED) && mSynergyMode) {
        motionPolicyFlags |= POLICY_FLAG_PASS_TO_USER;
    }
}

void MiInputDispatcherStubImpl::changeInjectedMotionArgsPolicyFlags(const NotifyMotionArgs* motionArgs, uint32_t& argsPolicyFlags) {
    if((argsPolicyFlags & POLICY_FLAG_INJECTED) && mSynergyMode) {
        argsPolicyFlags |= POLICY_FLAG_PASS_TO_USER;
    }
}

bool MiInputDispatcherStubImpl::needSkipEventAfterPopupFromInboundQueueLocked(
        nsecs_t* nextWakeupTime) {
    if (mNeedUpdateThreadId) {
        // the caller must in dispatcher threed, so we could update thread id here.
        mDispatcherThreadId = std::this_thread::get_id();
        mNeedUpdateThreadId = false;
    }
    if (needSkipHoverEventLocked()) {
        ALOGI("Because pointer was down, so drop this hover event.");
        mInputDispatcher->releasePendingEventForStubLocked();
        *nextWakeupTime = LONG_LONG_MIN;
        return true;
    }
    return false;
}

bool MiInputDispatcherStubImpl::needSkipHoverEventLocked(){
    bool isOnPadMode = mMiInputManager->isPadMode();
    if (!isOnPadMode){
        return false;
    }
    const std::shared_ptr<EventEntry>& pendingEvent =
            mInputDispatcher->getPendingEventForStubLocked();
    if (pendingEvent->type != EventEntry::Type::MOTION) {
        return false;
    }
    std::shared_ptr<MotionEntry> testMotionEntry =
            std::static_pointer_cast<MotionEntry>(pendingEvent);
    int32_t maskedAction = testMotionEntry->action & AMOTION_EVENT_ACTION_MASK;
    if (!isFromSource(testMotionEntry->source, AINPUT_SOURCE_STYLUS)) {
        // Event not from stylus and event is down, so recovery hover event.
        if (maskedAction == AMOTION_EVENT_ACTION_DOWN ||
            maskedAction == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            mEnableHover = true;
        }
        return false;
    }
    bool isHoverAction = (maskedAction == AMOTION_EVENT_ACTION_HOVER_MOVE ||
                          maskedAction == AMOTION_EVENT_ACTION_HOVER_ENTER ||
                          maskedAction == AMOTION_EVENT_ACTION_HOVER_EXIT);
    if (isHoverAction) {
        return !mEnableHover;
    }
    if (maskedAction == AMOTION_EVENT_ACTION_MOVE) {
        // move action not process
        return false;
    }
    // if the stylus event action is down, we disable hover event,
    // and recovery it at stylus action up or other tools action down.
    mEnableHover = !(maskedAction == AMOTION_EVENT_ACTION_DOWN ||
                     maskedAction == AMOTION_EVENT_ACTION_POINTER_DOWN);
    return false;
}

bool haveSameToken(const sp<WindowInfoHandle>& first, const sp<WindowInfoHandle>& second) {
    if (first == second) {
        return true;
    }
    if (first == nullptr || second == nullptr) {
        return false;
    }
    return first->getToken() == second->getToken();
}

/**
 * Indicate whether one window handle should be considered as obscuring
 * another window handle. We only check a few preconditions. Actually
 * checking the bounds is left to the caller.
 */
static bool canBeObscuredByHandWritingWindow(const sp<WindowInfoHandle>& windowHandle,
                                             const sp<WindowInfoHandle>& otherHandle) {
    // Compare by token so cloned layers aren't counted
    if (haveSameToken(windowHandle, otherHandle)) {
        return false;
    }
    auto info = windowHandle->getInfo();
    auto otherInfo = otherHandle->getInfo();
    if (otherInfo->inputConfig.test(WindowInfo::InputConfig::NOT_VISIBLE)) {
        return false;
    } else if (otherInfo->displayId != info->displayId) {
        return false;
    }
    return true;
}

bool MiInputDispatcherStubImpl::isWindowObscuredByHandWritingWindowAtPointLocked(
        const sp<android::gui::WindowInfoHandle>& windowHandle, int32_t x, int32_t y) {
    int32_t displayId = windowHandle->getInfo()->displayId;
    const std::vector<sp<WindowInfoHandle>>& windowHandles =
            mInputDispatcher->getWindowHandlesForStubLocked(displayId);
    for (const sp<WindowInfoHandle>& otherHandle : windowHandles) {
        if (windowHandle == otherHandle) {
            break; // All future windows are below us. Exit early.
        }
        const WindowInfo* otherInfo = otherHandle->getInfo();
        const auto inputConfig = otherInfo->inputConfig;
        const bool isHandWritingWindow = inputConfig.test(WindowInfo::InputConfig::NOT_TOUCHABLE) &&
                otherInfo->interceptsStylus();
        if (isHandWritingWindow && canBeObscuredByHandWritingWindow(windowHandle, otherHandle) &&
            otherInfo->frameContainsPoint(x, y)) {
            return true;
        }
    }
    return false;
}

void MiInputDispatcherStubImpl::addExtraTargetFlagLocked(
        const sp<android::gui::WindowInfoHandle>& windowHandle, int32_t x, int32_t y,
        int32_t& targetFlag) {
    if (isWindowObscuredByHandWritingWindowAtPointLocked(windowHandle, x, y)) {
        targetFlag |= MiuiInputTarget::FLAG_WINDOW_IS_OBSCURED_BY_HANDWRITING_WINDOW;
    }
}

void MiInputDispatcherStubImpl::addMotionExtraResolvedFlagLocked(
        const std::unique_ptr<DispatchEntry>& dispatchEntry) {
    if (dispatchEntry->targetFlags & MiuiInputTarget::FLAG_EVENT_SEND_TO_HANDWRITING_WINDOW) {
        dispatchEntry->resolvedFlags |= AMOTION_EVENT_FLAG_SEND_TO_HANDWRITING_WINDOW;
    }
    if (dispatchEntry->targetFlags &
        MiuiInputTarget::FLAG_WINDOW_IS_OBSCURED_BY_HANDWRITING_WINDOW) {
        dispatchEntry->resolvedFlags |= AMOTION_EVENT_FLAG_SKIP_HANDWRITING_WINDOW;
    }
}

bool isPointerFromStylus(const MotionEntry& entry, int32_t pointerIndex) {
    return isFromSource(entry.source, AINPUT_SOURCE_STYLUS) &&
            (entry.pointerProperties[pointerIndex].toolType == AMOTION_EVENT_TOOL_TYPE_STYLUS ||
             entry.pointerProperties[pointerIndex].toolType == AMOTION_EVENT_TOOL_TYPE_ERASER);
}

void MiInputDispatcherStubImpl::afterFindTouchedWindowAtDownActionLocked(
        const sp<android::gui::WindowInfoHandle>& newTouchedWindowHandle,
        const MotionEntry& entry) {
    mSkipMonitor = newTouchedWindowHandle != nullptr &&
            newTouchedWindowHandle->getInfo()->interceptsStylus() &&
            isPointerFromStylus(entry, 0 /*pointerIndex*/);
}

void MiInputDispatcherStubImpl::whileAddGlobalMonitoringTargetsLocked(
        const Monitor& monitor, InputTarget& inputTarget) {
    if (mSkipMonitor) {
        inputTarget.flags |= MiuiInputTarget::FLAG_EVENT_SEND_TO_HANDWRITING_WINDOW;
    }
}

bool isHandWritingInjectMotionEvent(const MotionEntry& entry, int32_t pointerIndex) {
    // event is from stylus but tool type is not stylus, means that event is injected from
    // handwriting
    return isFromSource(entry.source, AINPUT_SOURCE_STYLUS) &&
            !(entry.pointerProperties[pointerIndex].toolType == AMOTION_EVENT_TOOL_TYPE_STYLUS ||
              entry.pointerProperties[pointerIndex].toolType == AMOTION_EVENT_TOOL_TYPE_ERASER);
}

bool MiInputDispatcherStubImpl::needSkipHandWritingWindow(
        const android::gui::WindowInfo& windowInfo) {
    // this method can only invoke in dispatcher thread, because we should use mPendingEvent.
    if (std::this_thread::get_id() != mDispatcherThreadId) {
        return false;
    }
    bool cannotAcceptInjectEvent = windowInfo.inputConfig.test(
            WindowInfo::InputConfig::SKIP_HANDWRITING_INJECT_MOTION_EVENT);
    // not handwriting drawing window, not skip
    if (!cannotAcceptInjectEvent) {
        return false;
    }
    const std::shared_ptr<EventEntry>& pendingEvent =
            mInputDispatcher->getPendingEventForStubLocked();
    if (pendingEvent->type != EventEntry::Type::MOTION) {
        return false;
    }
    std::shared_ptr<MotionEntry> motionEntry = std::static_pointer_cast<MotionEntry>(pendingEvent);
    return isHandWritingInjectMotionEvent(*motionEntry, /*pointerIndex*/ 0);
}

bool MiInputDispatcherStubImpl::isOnewayMode() {
    return mMiInputManager->isOnewayMode();
}

bool MiInputDispatcherStubImpl::isOnewayEvent(const EventEntry& eventEntry) {
    if (eventEntry.type == EventEntry::Type::MOTION) {
        const MotionEntry& motionEntry = static_cast<const MotionEntry&>(eventEntry);
        if((motionEntry.flags & AMOTION_EVENT_FLAG_ONEWAY) != 0) {
            return true;
        }
    }
    return false;
}

// MIUI ADD: START Activity Embedding
bool MiInputDispatcherStubImpl::isNeedMiuiEmbeddedEventMapping(const InputTarget& inputTarget) {
    const PointerInfo& firstPointerInfo =
        inputTarget.pointerInfos[inputTarget.pointerIds.firstMarkedBit()];
    return firstPointerInfo.isNeedMiuiEmbeddedEventMapping;
}

std::unique_ptr<DispatchEntry> MiInputDispatcherStubImpl::creatEmbeddedEventMappingEntry(std::unique_ptr<MotionEntry>& combinedMotionEntry, int32_t inputTargetFlags, const InputTarget& inputTarget) {
    const PointerInfo& firstPointerInfo =
            inputTarget.pointerInfos[inputTarget.pointerIds.firstMarkedBit()];
    const ui::Transform& firstPointerTransform =
            inputTarget.pointerTransforms[inputTarget.pointerIds.firstMarkedBit()];
    std::unique_ptr<DispatchEntry> dispatchEntry =
            std::make_unique<DispatchEntry>(std::move(combinedMotionEntry),  inputTargetFlags,
                                            firstPointerTransform, inputTarget.displayTransform, inputTarget.globalScaleFactor,
                                            firstPointerInfo.isNeedMiuiEmbeddedEventMapping,
                                            firstPointerInfo.miuiEmbeddedMidLeft,
                                            firstPointerInfo.miuiEmbeddedMidRight,
                                            firstPointerInfo.miuiEmbeddedHotMarginLeftRight,
                                            firstPointerInfo.miuiEmbeddedHotMarginTopBottom);
    return dispatchEntry;
}

bool MiInputDispatcherStubImpl::addEmbeddedEventMappingPointers(InputTarget& inputTarget, BitSet32 pointerIds, const android::gui::WindowInfo* windowInfo) {
    if(windowInfo->isNeedMiuiEmbeddedEventMapping) {
        Rect miuiEmbeddedMidR = windowInfo->miuiEmbeddedMidRegion.getBounds();
        Rect miuiEmbeddedHotR = windowInfo->miuiEmbeddedHotRegion.getBounds();
        Rect touchableR = windowInfo->touchableRegion.getBounds();
        inputTarget.addPointers(pointerIds, windowInfo->transform,
            windowInfo->isNeedMiuiEmbeddedEventMapping, miuiEmbeddedMidR.left, miuiEmbeddedMidR.right,
            miuiEmbeddedHotR.left - touchableR.left, miuiEmbeddedHotR.top - touchableR.top);
        return true;
    } else {
        return false;
    }
}

void MiInputDispatcherStubImpl::embeddedEventMapping(DispatchEntry* dispatchEntry, const MotionEntry& motionEntry, const PointerCoords* &usingCoords) {
    if(dispatchEntry->isNeedMiuiEmbeddedEventMapping) {
        ui::Transform embeddedTransform = dispatchEntry->transform;
        float xOffset = embeddedTransform.tx();
        bool rot_90 = embeddedTransform.getOrientation() == ui::Transform::ROT_90;
        int action = dispatchEntry->resolvedAction & AMOTION_EVENT_ACTION_MASK;
        int index = (dispatchEntry->resolvedAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        if(action == AMOTION_EVENT_ACTION_DOWN
            || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            actionDownCoords[index] = usingCoords[index];
        }
        float downY = actionDownCoords[0].getY();
        float miuiEmbeddedHotLeft = dispatchEntry->miuiEmbeddedHotMarginLeftRight;
        float miuiEmbeddedHotRight = dispatchEntry->miuiEmbeddedMidLeft + dispatchEntry->miuiEmbeddedMidRight - dispatchEntry->miuiEmbeddedHotMarginLeftRight;
        if((downY < dispatchEntry->miuiEmbeddedMidLeft && downY > miuiEmbeddedHotLeft)
            || (downY > dispatchEntry->miuiEmbeddedMidRight && downY < miuiEmbeddedHotRight)) {
            for (uint32_t i = 0; i < motionEntry.pointerCount; i++) {
                mappingCoords[i] = usingCoords[i];
            }
            bool hasMove = false;
            float mappingXOffset = 0.0f;
            for (uint32_t i = 0; i < motionEntry.pointerCount; i++) {
                if(actionDownCoords[i].getY() < dispatchEntry->miuiEmbeddedMidLeft
                    && actionDownCoords[i].getY() > miuiEmbeddedHotLeft) {
                     mappingXOffset = rot_90 ? (-(dispatchEntry->miuiEmbeddedMidLeft + miuiEmbeddedHotLeft) + xOffset) :(-miuiEmbeddedHotLeft - xOffset);
                } else if(actionDownCoords[i].getY() > dispatchEntry->miuiEmbeddedMidRight
                    && actionDownCoords[i].getY() < miuiEmbeddedHotRight) {
                     mappingXOffset = rot_90 ? (-miuiEmbeddedHotRight + xOffset) : (-(dispatchEntry->miuiEmbeddedMidRight - miuiEmbeddedHotLeft) - xOffset);
                }
                //mappingCoords[i].applyOffset(mappingXOffset, 0);
                mappingCoords[i].setAxisValue(AMOTION_EVENT_AXIS_Y, mappingCoords[i].getY() + mappingXOffset);
                if(fabs(usingCoords[i].getY() - actionDownCoords[i].getY()) > 100
                    || fabs(usingCoords[i].getX() - actionDownCoords[i].getX()) > 100) {
                    hasMove = true;
                }
            }
            if(action == AMOTION_EVENT_ACTION_UP
                || action == AMOTION_EVENT_ACTION_POINTER_UP) {
                if((actionDownCoords[index].getY() < dispatchEntry->miuiEmbeddedMidLeft && downY > miuiEmbeddedHotLeft)
                    || (actionDownCoords[index].getY() > dispatchEntry->miuiEmbeddedMidRight && downY < miuiEmbeddedHotRight)) {
                    if(fabs(usingCoords[index].getY() - actionDownCoords[index].getY()) < 48
                        && fabs(usingCoords[index].getX() - actionDownCoords[index].getX()) < 48
                        && !hasMove) {
                        dispatchEntry->resolvedAction = AMOTION_EVENT_ACTION_CANCEL;
                    }
                }
                if (action == AMOTION_EVENT_ACTION_POINTER_UP) {
                    for (uint32_t i = index; i < motionEntry.pointerCount - 1; i++) {
                        actionDownCoords[index] = actionDownCoords[index + 1];
                    }
                }
            }
            usingCoords = mappingCoords;
            ALOGI("EmbeddedEventMapping xOffset = %f, action = %d, downY = %f index = %d", xOffset, action, downY, index);
        }
    }
}
// END

class LooperEventCallback : public LooperCallback {
public:
    LooperEventCallback(std::function<int(int events)> callback) : mCallback(callback) {}

    int handleEvent(int /*fd*/, int events, void * /*data*/) override {
        return mCallback(events);
    }

private:
    std::function<int(int events)> mCallback;
};

void MiInputDispatcherStubImpl::addCallbackForOnewayMode(const sp<Connection>& connection,
                                                         bool output) {
    int fd = connection->inputChannel->getFd();
    std::function<int(int events)> callback = std::bind(
        &MiInputDispatcherStubImpl::handleCallbackForOnewayMode, this, std::placeholders::_1,
        connection);
    if (output) {
        if(!connection->fdOutputEvent) {
            mInputDispatcher->getLooper()->addFd(fd, 0, ALOOPER_EVENT_OUTPUT | ALOOPER_EVENT_INPUT,
                                                new LooperEventCallback(callback), nullptr);
            connection->fdOutputEvent = true;
            ALOGD("add ALOOPER_EVENT_OUTPUT,ALOOPER_EVENT_INPUT callback for oneway mode,"
                  "connection:%s", connection->getWindowName().c_str());
        }
    } else {
        mInputDispatcher->getLooper()->addFd(fd, 0, ALOOPER_EVENT_INPUT,
                                             new LooperEventCallback(callback), nullptr);
        connection->fdOutputEvent = false;
        ALOGD("add ALOOPER_EVENT_INPUT callback for oneway mode, connection:%s",
              connection->getWindowName().c_str());
    }
}

int MiInputDispatcherStubImpl::handleCallbackForOnewayMode(int events, sp<Connection>& connection) {
    // Allowed return values of this function as documented in LooperCallback::handleEvent
    constexpr int REMOVE_CALLBACK = 0;
    constexpr int KEEP_CALLBACK = 1;

    if (connection == nullptr) {
        ALOGD("Received looper callback for unknown connection");
        return REMOVE_CALLBACK;
    }

    ALOGD("handle callback for oneway mode, events:%d, connection:%s", events,
          connection->getWindowName().c_str());

    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP | ALOOPER_EVENT_INPUT)) {
        sp<IBinder> token = connection->inputChannel->getConnectionToken();
        return mInputDispatcher->handleReceiveCallbackForStub(events, token);
    }

    if (events & ALOOPER_EVENT_OUTPUT) {
        mInputDispatcher->startDispatchCycleForStub(now(), connection);
        if (connection->outboundQueue.empty()) {
            // The queue is now empty. Tell looper there's no more output to expect.
            addCallbackForOnewayMode(connection, false);
        }
    }
    return KEEP_CALLBACK;
}

bool MiInputDispatcherStubImpl::isBerserkMode() {
    return kBerserkMode;
}

void MiInputDispatcherStubImpl::beforeTransferTouchFocusLocked(const sp<IBinder>& fromToken,
                                                               const sp<IBinder>& toToken,
                                                               bool isDragDrop) {
    if (!isDragDrop) {
        // Only drag drop behavior need change from token
        return;
    }
    sp<WindowInfoHandle> fromWindowHandle =
            mInputDispatcher->getWindowHandleForStubLocked(fromToken);
    if (fromWindowHandle == nullptr) {
        ALOGW("Can not find window from all windows by original from token");
        return;
    }
    const bool windowCanTransferAnyTouch = fromWindowHandle->getInfo()->inputConfig.test(
            WindowInfo::InputConfig::TRANSFER_ANY_TOUCH_FOR_DRAG);
    if (!windowCanTransferAnyTouch) {
        ALOGW("The from window have no permission to transfer any touch.");
        return;
    }
    // Get first touched foreground window
    const int32_t displayId = fromWindowHandle->getInfo()->displayId;
    sp<WindowInfoHandle> changeToWindowHandle =
            mInputDispatcher->findTouchedForegroundWindowForStubLocked(displayId);
    if (changeToWindowHandle == nullptr) {
        ALOGW("Can not find any foreground window from current display %d", displayId);
        return;
    }
    // Modify from token
    sp<IBinder>& noConstFromToken = (sp<IBinder>&)fromToken;
    noConstFromToken = changeToWindowHandle->getToken();
    // Because of the gesture stub logic may inject a down(time out or not a gesture scene), it will
    // cause conflicting point event, so we should clean stub logic here
    bool cleanGestureLogic = false;
    if (mGestureSt == GESTURE_NOT_DECIDE_BOTTOM || mGestureSt == GESTURE_NOT_DECIDE_LEFT ||
        mGestureSt == GESTURE_NOT_DECIDE_RIGHT) {
        mGestureSt = GESTURE_NO;
        cleanGestureLogic = true;
    }
    ALOGW("From token change success, current window name is %s, clean gesture logic = %s",
          changeToWindowHandle->getName().c_str(), toString(cleanGestureLogic));
}

extern "C" IMiInputDispatcherStub* createInputDispatcherStubImpl() {
    return MiInputDispatcherStubImpl::getInstance();
}

extern "C" void destroyInputDispatcherStubImpl(IMiInputDispatcherStub* instance) {
    delete instance;
}
} // namespace android
