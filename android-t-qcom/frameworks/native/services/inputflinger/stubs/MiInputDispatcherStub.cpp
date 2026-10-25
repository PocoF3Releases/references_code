
#include "MiInputDispatcherStub.h"
#include <dlfcn.h>
#include <log/log.h>
//#include "Connection.h"

namespace android {

using namespace inputdispatcher;

void* MiInputDispatcherStub::sLibMiInputDispatcherImpl = nullptr;
IMiInputDispatcherStub* MiInputDispatcherStub::sStubImplInstance = nullptr;
std::mutex MiInputDispatcherStub::sLock;
bool MiInputDispatcherStub::inited = false;

IMiInputDispatcherStub* MiInputDispatcherStub::getImplInstance() {
    std::lock_guard<std::mutex> lock(sLock);
    if (!sLibMiInputDispatcherImpl && !inited) {
        sLibMiInputDispatcherImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (sLibMiInputDispatcherImpl) {
            // typedef function pointer
            typedef IMiInputDispatcherStub* (*Create)();
            // get the createInputDispatcherStub function pointer
            Create create = (Create)dlsym(sLibMiInputDispatcherImpl, "createInputDispatcherStubImpl");
            if (create) {
                // invoke
                sStubImplInstance = create();
            } else {
                ALOGE("dlsym is fail InputDispatcher.");
            }
        } else {
            ALOGE("inputflinger open %s failed! dlopen - %s", LIBPATH, dlerror());
        }
        inited = true;
    }
    return sStubImplInstance;
}

void MiInputDispatcherStub::destroyImplInstance() {
    std::lock_guard<std::mutex> lock(sLock);
    if (!sLibMiInputDispatcherImpl) {
        return;
    }
    // typedef function pointer
    typedef void (*Destroy)(IMiInputDispatcherStub*);
    // get the destroyInputDispatcherStub function
    Destroy destroy = (Destroy)dlsym(sLibMiInputDispatcherImpl, "destroyInputDispatcherStubImpl");
    // destroy sStubImplInstance
    destroy(sStubImplInstance);
    dlclose(sLibMiInputDispatcherImpl);
    sLibMiInputDispatcherImpl = nullptr;
    sStubImplInstance = nullptr;
}

void MiInputDispatcherStub::init(inputdispatcher::InputDispatcher* inputDispatcher) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->init(inputDispatcher);
    }
}

bool MiInputDispatcherStub::getInputDispatcherAll() {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->getInputDispatcherAll();
    }
    return true;
}

void MiInputDispatcherStub::appendDetailInfoForConnectionLocked(const sp<Connection>& connection,
                                                          std::string& targetList) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->appendDetailInfoForConnectionLocked(connection, targetList);
    }
}

void MiInputDispatcherStub::beforeNotifyKey(const NotifyKeyArgs* args) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeNotifyKey(args);
    }
}

void MiInputDispatcherStub::beforeNotifyMotion(const NotifyMotionArgs* args) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeNotifyMotion(args);
    }
}

void MiInputDispatcherStub::addAnrState(const std::string& time, const std::string& anrState) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->addAnrState(time, anrState);
    }
}

void MiInputDispatcherStub::dump(std::string& dump) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->dump(dump);
    }
}

bool MiInputDispatcherStub::needSkipWindowLocked(const android::gui::WindowInfo& windowInfo) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->needSkipWindowLocked(windowInfo);
    }
    return false;
}

bool MiInputDispatcherStub::needSkipDispatchToSpyWindow() {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->needSkipDispatchToSpyWindow();
    }
    return false;
}

void MiInputDispatcherStub::beforePointerEventFindTargetsLocked(nsecs_t currentTime,
                                                                const MotionEntry* entry,
                                                                nsecs_t* nextWakeupTime) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforePointerEventFindTargetsLocked(currentTime, entry, nextWakeupTime);
    }
}

void MiInputDispatcherStub::checkHotDownTimeoutLocked(nsecs_t* nextWakeupTime) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->checkHotDownTimeoutLocked(nextWakeupTime);
    }
}

void MiInputDispatcherStub::afterPointerEventFindTouchedWindowTargetsLocked(MotionEntry* entry) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->afterPointerEventFindTouchedWindowTargetsLocked(entry);
    }
}

bool MiInputDispatcherStub::needPokeUserActivityLocked(const EventEntry& eventEntry) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->needPokeUserActivityLocked(eventEntry);
    }
    // default need poke user activity return true
    return true;
}

void MiInputDispatcherStub::afterPublishEvent(status_t status, const sp<Connection>& connection,
                                              const DispatchEntry* dispatchEntry) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->afterPublishEvent(status, connection, dispatchEntry);
    }
}

void MiInputDispatcherStub::accelerateMetaShortcutsAction(const int32_t deviceId,
                                                          const int32_t action, int32_t& keyCode,
                                                          int32_t& metaState) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->accelerateMetaShortcutsAction(deviceId, action, keyCode, metaState);
    }
}
void MiInputDispatcherStub::modifyDeviceIdBeforeInjectEventInitialize(const int32_t flags,
                                                                      int32_t& resolvedDeviceId,
                                                                      const int32_t realDeviceId) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->modifyDeviceIdBeforeInjectEventInitialize(flags, resolvedDeviceId, realDeviceId);
    }
}
void MiInputDispatcherStub::beforeInjectEventLocked(int32_t injectorPid, const InputEvent* inputEvent) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeInjectEventLocked(injectorPid, inputEvent);
    }
}
void MiInputDispatcherStub::beforeSetInputDispatchMode(bool enabled, bool frozen) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeSetInputDispatchMode(enabled, frozen);
    }
}
void MiInputDispatcherStub::beforeSetInputFilterEnabled(bool enabled) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeSetInputFilterEnabled(enabled);
    }
}
void MiInputDispatcherStub::beforeSetFocusedApplicationLocked(
        int32_t displayId, std::shared_ptr<InputApplicationHandle> inputApplicationHandle) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeSetFocusedApplicationLocked(displayId, inputApplicationHandle);
    }
}

void MiInputDispatcherStub::beforeSetFocusedWindowLocked(
        const android::gui::FocusRequest& request) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeSetFocusedWindowLocked(request);
    }
}

void MiInputDispatcherStub::beforeOnFocusChangedLocked(const FocusResolver::FocusChanges& changes) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeOnFocusChangedLocked(changes);
    }
}

void MiInputDispatcherStub::changeInjectedKeyEventPolicyFlags(const KeyEvent& keyEvent,
                                                              uint32_t& policyFlags) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->changeInjectedKeyEventPolicyFlags(keyEvent, policyFlags);
    }
}

void MiInputDispatcherStub::changeInjectedMotionEventPolicyFlags(const MotionEvent& motionEvent,
                                                                 uint32_t& policyFlags) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->changeInjectedMotionEventPolicyFlags(motionEvent, policyFlags);
    }
}

void MiInputDispatcherStub::changeInjectedMotionArgsPolicyFlags(const NotifyMotionArgs* motionArgs,
                                                                uint32_t& policyFlags) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->changeInjectedMotionArgsPolicyFlags(motionArgs, policyFlags);
    }
}

bool MiInputDispatcherStub::needSkipEventAfterPopupFromInboundQueueLocked(nsecs_t* nextWakeupTime) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->needSkipEventAfterPopupFromInboundQueueLocked(nextWakeupTime);
    }
    return false;
}

void MiInputDispatcherStub::addExtraTargetFlagLocked(
        const sp<android::gui::WindowInfoHandle>& windowHandle, int32_t x, int32_t y,
        int32_t& targetFlag) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->addExtraTargetFlagLocked(windowHandle, x, y, targetFlag);
    }
}
void MiInputDispatcherStub::addMotionExtraResolvedFlagLocked(
        const std::unique_ptr<DispatchEntry>& dispatchEntry) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->addMotionExtraResolvedFlagLocked(dispatchEntry);
    }
}

void MiInputDispatcherStub::afterFindTouchedWindowAtDownActionLocked(
        const sp<android::gui::WindowInfoHandle>& newTouchedWindowHandle,
        const MotionEntry& entry) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->afterFindTouchedWindowAtDownActionLocked(newTouchedWindowHandle, entry);
    }
}

void MiInputDispatcherStub::whileAddGlobalMonitoringTargetsLocked(const Monitor& monitor,
                                                                  InputTarget& inputTarget) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->whileAddGlobalMonitoringTargetsLocked(monitor, inputTarget);
    }
}

bool MiInputDispatcherStub::isOnewayMode() {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->isOnewayMode();
    }
    return false;
}

bool MiInputDispatcherStub::isOnewayEvent(const EventEntry& eventEntry) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->isOnewayEvent(eventEntry);
    }
    return false;
}

void MiInputDispatcherStub::addCallbackForOnewayMode(const sp<Connection>& connection, bool output) {
    IMiInputDispatcherStub *stub = getImplInstance();
    if (stub) {
        stub->addCallbackForOnewayMode(connection, output);
    }
}

bool MiInputDispatcherStub::isBerserkMode() {
    IMiInputDispatcherStub *stub = getImplInstance();
    if (stub) {
        return stub->isBerserkMode();
    }
    return false;
}

// MIUI ADD: START Activity Embedding
bool MiInputDispatcherStub::isNeedMiuiEmbeddedEventMapping(const InputTarget& inputTarget){
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->isNeedMiuiEmbeddedEventMapping(inputTarget);
    }
    return false;
}

std::unique_ptr<DispatchEntry> MiInputDispatcherStub::creatEmbeddedEventMappingEntry(std::unique_ptr<MotionEntry>& combinedMotionEntry, int32_t inputTargetFlags, const InputTarget& inputTarget) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->creatEmbeddedEventMappingEntry(combinedMotionEntry, inputTargetFlags, inputTarget);
    }
    return nullptr;
}

bool MiInputDispatcherStub::addEmbeddedEventMappingPointers(InputTarget& inputTarget, BitSet32 pointerIds, const android::gui::WindowInfo* windowInfo) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        return stub->addEmbeddedEventMappingPointers(inputTarget, pointerIds, windowInfo);
    }
    return false;
}

void MiInputDispatcherStub::embeddedEventMapping(DispatchEntry* dispatchEntry, const MotionEntry& motionEntry, const PointerCoords* &usingCoords) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->embeddedEventMapping(dispatchEntry, motionEntry, usingCoords);
    }
}
// END

void MiInputDispatcherStub::beforeTransferTouchFocusLocked(const sp<IBinder>& fromToken,
                                                           const sp<IBinder>& toToken,
                                                           bool isDragDrop) {
    IMiInputDispatcherStub* stub = getImplInstance();
    if (stub) {
        stub->beforeTransferTouchFocusLocked(fromToken, toToken, isDragDrop);
    }
}

} // namespace android
