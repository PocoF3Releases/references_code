#define LOG_TAG "MiuiInputMapper"

#include "MiuiInputMapper.h"
#include <android-base/stringprintf.h>
#include <cinttypes>
#include <gui/constants.h>

#define INDENT  "  "
#define INDENT2 "    "
#define INDENT3 "      "
#define INDENT4 "        "
#define INDENT5 "          "
#undef ALOGD
#define ALOGD(...) if (MiInputManager::getInstance()->mInputDebugConfig->mInputReaderAll) \
__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using android::base::StringPrintf;
namespace android {

static inline nsecs_t now() {
    return systemTime(SYSTEM_TIME_MONOTONIC);
}

MiuiInputMapper::MiuiInputMapper(InputListenerInterface &listener)
        : mIdGenerator(IdGenerator::Source::INPUT_READER), mListener(listener) {
    MiInputManager::getInstance()->addOnConfigurationChangedListener(
            mOnConfigurationChangedListener);
    mMotionState.reset();
}

void logNotifyKey(const char *prefix, const NotifyKeyArgs *args) {
    ALOGD("%s notifyKey - eventTime=%" PRId64 ", deviceId=%d, source=0x%x, displayId=%" PRId32
        "policyFlags=0x%x, action=0x%x, "
        "flags=0x%x, keyCode=0x%x, scanCode=0x%x, metaState=0x%x, downTime=%" PRId64, prefix,
        args->eventTime, args->deviceId, args->source, args->displayId, args->policyFlags,
        args->action, args->flags, args->keyCode, args->scanCode, args->metaState,
        args->downTime);
}

void logNotifyMotionArgs(const char *prefix, const NotifyMotionArgs *args) {
    ALOGD("%s notifyMotionArgs - id=%" PRIx32 " eventTime=%" PRId64 ", deviceId=%d, source=0x%x, "
        "displayId=%" PRId32 ", policyFlags=0x%x, "
        "action=0x%x, actionButton=0x%x, flags=0x%x, metaState=0x%x, buttonState=0x%x, "
        "edgeFlags=0x%x, xPrecision=%f, yPrecision=%f, xCursorPosition=%f, "
        "yCursorPosition=%f, downTime=%" PRId64, prefix,
        args->id, args->eventTime, args->deviceId, args->source, args->displayId,
        args->policyFlags, args->action, args->actionButton, args->flags, args->metaState,
        args->buttonState, args->edgeFlags, args->xPrecision, args->yPrecision,
        args->xCursorPosition, args->yCursorPosition, args->downTime);
    for (uint32_t i = 0; i < args->pointerCount; i++) {
        ALOGD("  Pointer %d: id=%d, toolType=%d, "
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
}

void MiuiInputMapper::notifyConfigurationChanged(const NotifyConfigurationChangedArgs *args) {
    // pass through
    std::scoped_lock _l(mLock);
    mListener.notifyConfigurationChanged(args);
}

void MiuiInputMapper::notifyKey(const NotifyKeyArgs *args) {
    std::scoped_lock _l(mLock);
    logNotifyKey("MiuiInputMapper notifyKey", args);
    mListener.notifyKey(args);
}

void MiuiInputMapper::notifySwitch(const NotifySwitchArgs *args) {
    // pass through
    std::scoped_lock _l(mLock);
    mListener.notifySwitch(args);
}

void MiuiInputMapper::notifySensor(const NotifySensorArgs *args) {
    // pass through
    std::scoped_lock _l(mLock);
    mListener.notifySensor(args);
}

void MiuiInputMapper::notifyVibratorState(const NotifyVibratorStateArgs *args) {
    // pass through
    std::scoped_lock _l(mLock);
    mListener.notifyVibratorState(args);
}

void MiuiInputMapper::notifyDeviceReset(const NotifyDeviceResetArgs *args) {
    // pass through
    std::scoped_lock _l(mLock);
    mListener.notifyDeviceReset(args);
}

void MiuiInputMapper::notifyPointerCaptureChanged(const NotifyPointerCaptureChangedArgs *args) {
    // pass through
    std::scoped_lock _l(mLock);
    mListener.notifyPointerCaptureChanged(args);
}

void MiuiInputMapper::notifyMotion(const NotifyMotionArgs* args) {
    std::scoped_lock _l(mLock);
    if(args->source != AINPUT_SOURCE_TOUCHSCREEN){
        mListener.notifyMotion(args);
        return;
    }
    //自动连招录制
    if(MiInputManager::getInstance()->mInputCommonConfig->mRecordEventStatus) {
        MotionEvent event;
        ui::Transform transform;
        event.initialize(args->id, args->deviceId, args->source, args->displayId, {0},
                         args->action, args->actionButton, args->flags, args->edgeFlags,
                         args->metaState, args->buttonState, args->classification, transform,
                         args->xPrecision, args->yPrecision, args->xCursorPosition,
                         args->yCursorPosition, transform, args->downTime, args->eventTime,
                         args->pointerCount, args->pointerProperties, args->pointerCoords);
        if (mPolicy != nullptr) {
            mPolicy->notifyTouchMotionEvent(&event);
        } else {
            ALOGE("MiuiInputMapper.mPolicy is null");
        }
    }
    if(!MiInputManager::getInstance()->mInputCommonConfig->mInjectEventStatus){
        mListener.notifyMotion(args);
        logNotifyMotionArgs("MiuiInputMapper notifyMotion:",args);
    } else {//将原始Touch事件加到mMotionState,publish
        mMotionState.updateState(args);
        mMotionState.publish(mListener);
    }
}

void MiuiInputMapper::injectMotionEvent(const MotionEvent* motionEvent, int32_t mode){
    std::scoped_lock _l(mLock);
    ALOGD("MiuiInputMapper.injectMotionEvent action %d,mode  %d", motionEvent->getAction(), mode);
    //将Inject Touch事件加到mMotionState,publish
    if(MiInputManager::getInstance()->mInputCommonConfig->mInjectEventStatus){
        mMotionState.updateState(motionEvent, mode, mIdGenerator.nextId());
        mMotionState.publish(mListener);
    }
}

void MiuiInputMapper::setPolicy(const sp<MiuiInputMapperPolicyInterface>& policy) {
    mPolicy = policy;
}

void MiuiInputMapper::onConfigurationChanged(InputConfigTypeEnum type, int32_t changes) {
    if(type == INPUT_CONFIG_TYPE_COMMON_CONFIG){
        if((changes & CONFIG_INJECTEVENT_STATUS) != 0) {
            mMotionState.reset();
        }
    }
}

void MiuiInputMapper::dump(std::string &dump) {
    if(MiInputManager::getInstance()->mInputCommonConfig->mInjectEventStatus){
        mMotionState.dump(dump);
    }
}

void MiuiInputMapper::MotionState::dump(std::string& dump) {
    dump += StringPrintf(INDENT2 "MiuiInputMapper MotionState:\n");
    dump += StringPrintf(INDENT3 "action: %d:\n", actionMask);
    dump += StringPrintf(INDENT3 "actionIndex: %d:\n", actionIndex);
    dump += StringPrintf(INDENT3 "pointerInfos\n");
    for(int i = 0; i < (int)MAX_POINTERS; i++){
        if(!pointerCoords[i].isEmpty()){
            dump += StringPrintf(INDENT4 "%d, id:%d, x:%f y:%f\n", i,
                                 pointerProperties[i].id,
                                 pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_X),
                                 pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_Y));
        }
    }
}

void MiuiInputMapper::MotionState::reset() {
    ALOGI("MotionState reset");
    id  = 0;
    eventTime = 0;
    deviceId = -1;
    source = AINPUT_SOURCE_TOUCHSCREEN;
    displayId = ADISPLAY_ID_DEFAULT;
    policyFlags = 0;
    actionButton = 0;
    flags = 0;
    metaState = 0;
    buttonState = 0;
    classification = MotionClassification::NONE;
    edgeFlags = 0;
    actionMask = AMOTION_EVENT_ACTION_UP;
    actionIndex = 0;
    pointerCount = 0;
    xPrecision = 1;
    yPrecision = 1;
    xCursorPosition = 0;
    yCursorPosition = 0;
    downTime = 0;
    readTime = 0;
    videoFrames = {};
    for(int i = 0; i < (int)MAX_POINTERS;i++){
        pointerProperties[i].clear();
        pointerCoords[i].clear();
    }
}
  
void MiuiInputMapper::MotionState::updateState(const MotionEvent* event, int mode, int32_t seq) {
    uint32_t rawActionIndex = event->getActionIndex();
    actionMask = event->getActionMasked();
    id = seq;
    for(uint32_t i = 0; i < event->getPointerCount(); i++){
        int pointerId = adjustPointerId(event->getPointerId(i), mode);
        int32_t index = addPointer(*event->getRawPointerCoords(i), pointerId);
        if(i == rawActionIndex) {
            actionIndex = index;
        }
    }
}
  
void MiuiInputMapper::MotionState::updateState(const NotifyMotionArgs* args) {
    uint32_t rawActionIndex = (args->action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    for(uint32_t i = 0; i < args->pointerCount; i++){
        int32_t index = addPointer(args->pointerCoords[i],  args->pointerProperties[i].id);
        if(i == rawActionIndex) {
            actionIndex = index;
        }
    }
    id = args->id;
    actionMask = args->action & AMOTION_EVENT_ACTION_MASK;
}

//添加或者更新Pointer data
int MiuiInputMapper::MotionState::addPointer(PointerCoords coords,int32_t pointerId) {
    for(int32_t j = 0; j < (int)MAX_POINTERS; j++){
        if(!pointerCoords[j].isEmpty() && (pointerProperties[j].id == pointerId)){
            pointerCoords[j].copyFrom(coords);
            return j;
        }
    }
    for(int32_t j = 0; j < (int)MAX_POINTERS; j++){
        if(pointerCoords[j].isEmpty()){
            pointerCoords[j].copyFrom(coords);
            pointerProperties[j].id = pointerId;
            pointerProperties[j].toolType = AMOTION_EVENT_TOOL_TYPE_FINGER;
            return j;
        }
    }
    ALOGE("addPointer Failed, current PointerCount was 16!");
    return -1;
}
  
int MiuiInputMapper::MotionState::adjustPointerId(int pointerId, int mode) {
    constexpr int firstTouchID = 16;
    if(mode == MODE_LEFT_SHOULDERKEY){ //左肩键
        pointerId += firstTouchID;
    }else if (mode == MODE_RIGHT_SHOULDERKEY){ //右肩键
        pointerId += firstTouchID + 1;
    }else if(mode == MODE_MACRO){ //连招
        pointerId += firstTouchID + 2;
    }
    return pointerId;
}
  
void MiuiInputMapper::MotionState::publish(InputListenerInterface& listener) {
    PointerCoords tmpPointerCoords[MAX_POINTERS];
    PointerProperties tmpPointerProperties[MAX_POINTERS];
    int32_t tmpActionIndex;
    int32_t tmpAction;
    pointerCount = 0;
    for(int i = 0; i < (int)MAX_POINTERS; i ++){
        if(pointerCoords[i].isEmpty()){
            continue;
        }
        tmpPointerCoords[pointerCount].copyFrom(pointerCoords[i]);
        tmpPointerProperties[pointerCount].copyFrom(pointerProperties[i]);
        tmpPointerProperties[pointerCount].id = i;
        tmpPointerCoords[pointerCount].setAxisValue(AMOTION_EVENT_AXIS_GENERIC_1,
                                                    pointerProperties[i].id);
        if(i == actionIndex){ //记录ActionIndex
            tmpActionIndex = pointerCount;
        }
        pointerCount += 1;
    }
    if(pointerCount == 0){
        ALOGE("buildNotifyMotionArgs Error, pointerCount is 0 !!!");
        return;
    }
    tmpAction = actionMask;
    if(pointerCount > 1 && actionMask != AMOTION_EVENT_ACTION_MOVE) {
        if (actionMask == AMOTION_EVENT_ACTION_DOWN) {
            tmpAction = AMOTION_EVENT_ACTION_POINTER_DOWN;
        } else if (actionMask == AMOTION_EVENT_ACTION_UP) {
            tmpAction = AMOTION_EVENT_ACTION_POINTER_UP;
        }
        tmpAction |=  tmpActionIndex << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    }
    if(actionMask == AMOTION_EVENT_ACTION_DOWN){
        downTime = now();
    }
    eventTime = now();
    readTime = now();
    NotifyMotionArgs args(id, eventTime, readTime, deviceId, source,
                         displayId, policyFlags, tmpAction, actionButton, flags, metaState,
                         buttonState, classification, edgeFlags, pointerCount, tmpPointerProperties,
                         tmpPointerCoords, xPrecision, yPrecision, xCursorPosition, yCursorPosition,
                         downTime, videoFrames);
    logNotifyMotionArgs("MiuiInputMapper publish:", &args);
    listener.notifyMotion(&args);
    if(actionMask == AMOTION_EVENT_ACTION_UP ||
       actionMask == AMOTION_EVENT_ACTION_POINTER_UP){
        pointerCoords[actionIndex].clear();
        pointerProperties[actionIndex].clear();
    }
    if(actionMask == AMOTION_EVENT_ACTION_CANCEL){
        reset();
    }
}

}// namespace android