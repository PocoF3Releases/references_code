#define LOG_TAG "octvm_power"

#include <stdio.h>
#include <stdlib.h>
#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/RWLock.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/PermissionCache.h>
#include <private/android_filesystem_config.h>
#include <cutils/properties.h>

#include "octvm.h"
#include "octvm/PowerStateService.h"

namespace android {
//dump permission
static const String16 sDump("android.permission.DUMP");

//service name
const String8 PowerStateService::SName = String8("miui.whetstone.power");

enum OPERATION_TYPE {OPERATION_TYPE_REMOVE=1, OPERATION_TYPE_UPDATE = 2, OPERATION_TYPE_MAX = 3};

PowerStateService::PowerStateService(): isPowerStatesCustomed(false),currentModeHandler(NULL)
{
    //init the capability table
    PowerModeHandler::initPlatformCapability();
    //new autosaving thread with 0
    mAutoSavingThread = new AutoSavingThread(&(gVMPowerParams->default_params));
    mFrozenCheckThread = new FrozenCheckThread();
    //init default power mode handler and enter mode 0
    PowerModeHandler* pmHandler = initDefaultPowerState();
    if (pmHandler != NULL) {
        handleModeEnter(pmHandler);
    }
}

PowerStateService::~PowerStateService(){}

const char* PowerStateService::getServiceName() {
    return SName;
}

bool PowerStateService::isServiceStarted() {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = sm->checkService(String16(SName));
    if (binder != 0) {
        ALOGI("miui.whetstone.power is already started");
        return true;
    } else {
        ALOGI("miui.whetstone.power is not started");
        return false;
    }
}

bool PowerStateService::isFeatureOn()
{
    return true;
}

int32_t PowerStateService::FeatureEnable(bool enable)
{
    return 0;
}

PowerModeHandler* PowerStateService::getModeHandler(int32_t mode_id)
{
    if (gHandler.indexOfKey(mode_id) >= 0) {
        return gHandler.valueFor(mode_id);
    }
    return NULL;
}

PowerModeHandler* PowerStateService::getModeHandler(String16& modeName)
{
    for (int i = 0; i < gHandler.size(); i++) {
        PowerModeHandler* pmHandler = gHandler.valueAt(i);
        char *mName = pmHandler->getModeProperty()->mode_property;
        if (modeName == String16(mName)) {
            return pmHandler;
        }
    }
    return NULL;
}

PowerModeHandler* PowerStateService::initDefaultPowerState()
{
    if (sDefaultPowerModes->size() == 0 || sDefaultPowerModes->indexOfKey(0) < 0) {
        //generate a mode handler which system default use
        char *modeName = "sys_default";
        struct power_mode *defaultMode = NULL;
        int32_t length = strlen(modeName) + 1;
        defaultMode = (struct power_mode *)malloc(sizeof(struct power_mode)+length);
        defaultMode->len = length;
        defaultMode->hdr_size = sizeof(struct power_mode);
        defaultMode->mode_id = 0x0;
        defaultMode->type_id = 0x0;
        defaultMode->autosave = 0x0;
        strncpy(defaultMode->mode_property, modeName, length);
        defaultMode->mode_property[length-1] = '\0';
        PowerModeHandler* pmHandler = new PowerModeHandler(defaultMode);
        gHandler.add(defaultMode->mode_id, pmHandler);
    }
    for(int i = 0; i < sDefaultPowerModes->size(); i++) {
        struct power_mode *pMode = sDefaultPowerModes->valueAt(i);
        struct power_mode *nMode = (struct power_mode *)malloc(sizeof(struct power_mode) + pMode->len);
        memcpy(nMode, pMode, sizeof(struct power_mode) + pMode->len);
        ALOGD("add mode[%s %d]", nMode->mode_property, nMode->mode_id);
        PowerModeHandler* pmHandler = new PowerModeHandler(nMode);
        gHandler.add(pMode->mode_id, pmHandler);
    }

    return gHandler.valueFor(0);
}

int32_t PowerStateService::handleModeEnter(PowerModeHandler *pmHandler)
{
    if (currentModeHandler != NULL) {
        currentModeHandler->leaveMode();
    }
    pmHandler->enterMode();
    currentModeHandler = pmHandler;
    mAutoSavingThread->onPowerModeChanged(pmHandler);
    return 0;
}

int32_t PowerStateService::power_save_feature_enable(int32_t enable) {
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }
    return FeatureEnable(enable);
}

int32_t PowerStateService::power_state_mode_start(String16& modeName, int32_t modId)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    int retVal = -1;
    RWLock::AutoRLock lock(mRWLock);
    PowerModeHandler* pmHandler = getModeHandler(modId);
    if (pmHandler == NULL) {
        pmHandler = getModeHandler(modeName); //only use modeName when id is not present
    }
    if (pmHandler != NULL) {
        if (currentModeHandler != NULL && currentModeHandler->getModeProperty()->mode_id == modId) {
            ALOGD("already in mode %d, ignore this transition", modId);
        }
        else {
            ALOGI("power mode [%s] start %ld", pmHandler->getModeProperty()->mode_property, time(NULL));
            handleModeEnter(pmHandler);
        }
        retVal = 0;
    }
    else {
        ALOGE("cannot find mode config for mode_id[%d]", modId);
    }
    return retVal;
}

int32_t PowerStateService::power_state_trigger_action(String16& eventName, int32_t eventId)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    int retVal = -1;
    int32_t flags = EVENT_TRIGGER_BY_STRING; //TODO:eventId not used
    if (eventName != String16("")) {
        RWLock::AutoRLock lock(mRWLock);
        ALOGI("handle event trigger %s", String8(eventName).string());
        retVal = currentModeHandler->handleTriggerEvent(String8(eventName).string(), flags);
    }
    return retVal;
}

int32_t PowerStateService::dump(int fd, const Vector<String16>& args)
{
    String8 result;
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    pid_t callingPid = IPCThreadState::self()->getCallingPid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM && callingUid != AID_SHELL &&
            !PermissionCache::checkPermission(sDump, callingPid, callingUid))
    {
        result.appendFormat("Permission Denial: "
            "can't dump SurfaceFlinger from pid=%d, uid=%d\n", callingPid, callingUid);
        write(fd, result.string(), result.size());
        return 0;
    }
    //dump global parameter
    String8 gParamsInfo;
    gParamsInfo.appendFormat("Global autosave flag:%d\n", gVMPowerParams->default_autosave);
    gParamsInfo.appendFormat("System default power mode:\n");
    for(int i = 0; i < sDefaultPowerModes->size(); i++) {
        struct power_mode *pMode = sDefaultPowerModes->valueAt(i);
        gParamsInfo.appendFormat("[%s %d][default autosave %s]\n",
            pMode->mode_property, pMode->mode_id, (pMode->autosave == 0)? "off" : "on");
    }
    gParamsInfo.appendFormat("\n");
    write(fd, gParamsInfo.string(), gParamsInfo.length());
    //dump all using power mode info and statistics
    {
        RWLock::AutoRLock lock(mRWLock);
        for (int i = 0; i < gHandler.size(); i++) {
            gHandler.valueAt(i)->dumpStatistics(fd);
        }
    }
    //dump autosaving thread
    mAutoSavingThread->dumpParams(fd);
    mFrozenCheckThread->dumpParams(fd);
    return 0;
}

int32_t PowerStateService::power_state_update_config(const Parcel& argParcel)
{
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }

    //hold write lock until update complete
    RWLock::AutoWLock lock(mRWLock);
    int32_t operationType = argParcel.readInt32();
    if (operationType < OPERATION_TYPE_REMOVE || operationType >= OPERATION_TYPE_MAX ) {
        // 非法操作
        return -1;
    }
    int32_t modeId = argParcel.readInt32();
    ALOGW("power_state_update_config type=%d, modeId=%d", operationType, modeId);
    if (operationType == OPERATION_TYPE_REMOVE) {
        bool isSystemMode = getSystemPowerMode(modeId) != NULL;
        if (isSystemMode) {
            // Forbid remove system mode
            return -1;
        }
        // handle currentModeHandler
        if (currentModeHandler != NULL && currentModeHandler->getModeProperty()->mode_id == modeId) {
            // transition to default for preparing for removing current
            handleModeEnter(gHandler.valueFor(0));
        }
        for (int i = gHandler.size(); i-- > 0;) {
            PowerModeHandler* pmHandler = gHandler.valueAt(i);
            if (pmHandler->getModeProperty()->mode_id == modeId) {
                ALOGW("power_state_update_config removed success");
                removePowerState(pmHandler);
                gHandler.removeItemsAt(i);
            }
        }
        return 0;
    }

    // Add operation
    String8 modeName = String8(argParcel.readString16());
    int32_t name_length = strlen(modeName.string()) + 1;
    struct power_mode * sMode = (struct power_mode *)malloc(sizeof(struct power_mode) + name_length);
    sMode->len = name_length;
    sMode->hdr_size = sizeof(struct power_mode);
    sMode->mode_id = modeId;
    sMode->type_id = 0;
    sMode->autosave = gVMPowerParams->default_autosave;
    strncpy(sMode->mode_property, modeName.string(), name_length);
    sMode->mode_property[name_length - 1] = '\0';
    PowerModeHandler* pmHandler = addPowerState(sMode);
    pmHandler->removeAllEventActions();

    int32_t eventSize = argParcel.readInt32();
    ALOGW("power_state_update_config modeName=%s, eventSize=%d", modeName.string(), eventSize);
    for(int i = 0; i < eventSize; i++) {
        String8 eventName = String8(argParcel.readString16());
        int32_t actionSize = argParcel.readInt32();
        ALOGW("power_state_update_config eventName=%s, actionSize=%d", eventName.string(), actionSize);
        KeyedVector<String8, String8> actions;
        for(int j=0; j < actionSize; j++) {
           String8 actionName = String8(argParcel.readString16());
           String8 actionParam = String8(argParcel.readString16());
           ALOGW("power_state_update_config actionName=%s,actionParam=%s", actionName.string(), actionParam.string());
           actions.add(actionName, actionParam);
        }
        pmHandler->addEventActions(eventName.string(), actions);
    }
    return 0;
}

int32_t PowerStateService::power_state_get_avg_current(int modId)
{
    RWLock::AutoRLock lock(mRWLock);
    PowerModeHandler* pmHandler = getModeHandler(modId);
    if (pmHandler != NULL) {
        return pmHandler->getAverageCurrentInmA();
    }
    ALOGE("cannot find the mode for the given mode_id");
    return 0;
}

int32_t PowerStateService::power_state_time_in_state(int modId)
{
    RWLock::AutoRLock lock(mRWLock);
    PowerModeHandler* pmHandler = getModeHandler(modId);
    if (pmHandler != NULL) {
        return pmHandler->getTimeInStateInSecond();
    }
    ALOGE("cannot find the mode for the given mode_id");
    return 0;
}

int32_t PowerStateService::frozenApp(int32_t uid, Vector<int32_t>& processes) {
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }
    if (mFrozenCheckThread != NULL) {
        mFrozenCheckThread->addUidToCheckList(uid);
    }
    struct tfp_app_entity app;
    app.uid = uid;
    app.pidCnt = processes.size();
    if (app.pidCnt > APP_MAX_PID_NUM) {
        app.pidCnt = APP_MAX_PID_NUM;
    }
    for (int i = 0; i < app.pidCnt; i++) {
        app.pid[i] = processes[i];
    }
    return FROZEN_APP(&app);
}

int32_t PowerStateService::thawedApp(int32_t uid, Vector<int32_t>& processes) {
    uid_t callingUid = IPCThreadState::self()->getCallingUid();
    if (callingUid != AID_ROOT && callingUid != AID_SYSTEM) {
        ALOGW("permission denied for current calling uid %d", callingUid);
        return PERMISSION_DENIED;
    }
    //processes not used
    if (mFrozenCheckThread != NULL) {
        mFrozenCheckThread->delUidFromCheckList(uid);
    }
    return THAWED_APP(uid);
}

PowerModeHandler* PowerStateService::addPowerState(struct power_mode *pMode)
{
    PowerModeHandler* pmHandler;
    if (gHandler.indexOfKey(pMode->mode_id) >= 0) {
        pmHandler = gHandler.valueFor(pMode->mode_id);
        pmHandler->updateModeProperty(pMode);
        ALOGW("addPowerState modify");
    }
    else {
        ALOGD("add mode[%s %d]", pMode->mode_property, pMode->mode_id);
        pmHandler = new PowerModeHandler(pMode);
        gHandler.add(pMode->mode_id, pmHandler);
    }
    return pmHandler;
}

int32_t PowerStateService::removePowerState(PowerModeHandler *pmHandler)
{
    delete pmHandler;
    return 0;
}

KeyedVector<uint32_t, struct power_mode *>* PowerStateService::sDefaultPowerModes = new KeyedVector<uint32_t, struct power_mode *>();

struct power_mode* PowerStateService::getSystemPowerMode(int id)
{
    if (sDefaultPowerModes->indexOfKey(id) >= 0) {
        return sDefaultPowerModes->valueFor(id);
    }
    return NULL;
}

void PowerStateService::addSystemDefaultPowerMode(struct power_mode *pMode)
{
    sDefaultPowerModes->add(pMode->mode_id, pMode);
}

}; // namespace android
