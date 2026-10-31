#define LOG_TAG "octvm_power"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <utils/Timers.h>
#include <utils/KeyedVector.h>

#include "PowerModeHandler.h"
#include "drv/platform_power.h"
#include "UserDefinedPowerProfile.h"

extern "C" {
#include "klo_battery_utils.h"
#include "klo_internal.h"
}

using namespace android;

PowerModeHandler::PowerModeHandler(struct power_mode *sMode)
{
    pMode = sMode;
    custParams = NULL;
    resetStatistics();
}

PowerModeHandler::~PowerModeHandler()
{
    if (pMode != NULL) {
        free(pMode);
    }
    if (custParams != NULL) {
        free(custParams);
    }
    for (int i = 0; i < eventActions.size(); i++) {
        free(eventActions.keyAt(i));
        for (int j = 0; j < eventActions.valueAt(i).size(); j++) {
            delete eventActions.valueAt(i).valueAt(j);
        }
    }
    eventActions.clear();
}

void PowerModeHandler::removeAllEventActions()
{
    for (int i = 0; i < eventActions.size(); i++) {
        free(eventActions.keyAt(i));
        for (int j = 0; j < eventActions.valueAt(i).size(); j++) {
            delete eventActions.valueAt(i).valueAt(j);
        }
    }
    eventActions.clear();
}

int32_t PowerModeHandler::addEventActions(const char *event_name, KeyedVector<String8, String8>& actions)
{
    if (eventOutOfCap(event_name)) {
        fprintf(stderr, "system cannot handle event %s\n", event_name);
        return -2;
    }
    int32_t name_length = strlen(event_name) + 1;
    struct trigger_event *event = (struct trigger_event *)calloc(1, sizeof(*event) + name_length);
    event->len = name_length;
    event->hdr_size = sizeof(*event);
    strncpy(event->event_str, event_name, name_length);
    KeyedVector<String8, EventAction* > _actions;
    //add action to the action list
    for (int i = 0; i < actions.size(); i++) {
        String8 action_name = actions.keyAt(i);
        String8 action_param = actions.valueAt(i);
        ActionImpl actionImpl = getActionImplForName(action_name);
        if (actionImpl != NULL) {
            EventAction *action = new EventAction(action_name, action_param, actionImpl);
            _actions.add(action_name, action);
        }
    }
    // 先删再加, TODO:此处有坑， 并不能真正的remove
    eventActions.removeItem(event);
    eventActions.add(event, _actions);

    return 0;
}

int32_t PowerModeHandler::enterMode()
{
    if (eventActions.isEmpty()) {
        fprintf(stderr, "mode %s do not have any actions, empty group!", pMode->mode_property);
    }
    //trigger the default action
    else {
        handleTriggerEvent(POWER_MODE_START_EVENT, EVENT_TRIGGER_BY_STRING);
    }

    //statistics
    enterTime = nanoseconds_to_milliseconds(systemTime());
    totalCounts++;

    int32_t battery_status = BATTERY_STATUS_UNKNOWN;
    get_battery_status(&battery_status);
    if (battery_status != BATTERY_STATUS_CHARGING) {
        dischargeStartBattery = get_current_battery_uAh();
        dischargeStartTime = enterTime;
    }

    return 0;
}

int32_t PowerModeHandler::handleTriggerEvent(const char *event_name, int32_t flags)
{
    int32_t ret;
    for (int i = 0; i < eventActions.size(); i++) {
        struct trigger_event *event = eventActions.keyAt(i);
        if (strcmp(event->event_str, event_name) == 0) {
            ret = -1;
            for (int j = 0; j < eventActions.valueAt(i).size(); j++) {
                EventAction *action = eventActions.valueAt(i).valueAt(j);
                printf("Action: %s(%s) for [%s]", action->actionName.string(), action->actionParam.string(), event_name);
                if (action->actionName == "set_cpu_custprofile") {
                    String8 profileName = action->actionParam;
                    if (UserDefinedPowerProfile::checkUserPowerProfile(profileName)) {
                        ret = action->doAction(profileName.string());
                    } else {
                        fprintf(stderr, "probably not find profile %s\n", profileName.string());
                    }
                }
                else {
                    ret = action->doAction(action->actionParam.string());
                }
                if (ret < 0) {
                    fprintf(stderr, "Action %s failed.", action->actionName.string());
                    //there maybe something to do
                }
            }
            //anyway set ret to zero in this case
            ret = 0;
            break;
        }
    }
    if (ret == -1) {
        fprintf(stderr, "unsupported flags[%d] or no event trigger found", flags);
    }

    //redirect event to klo module
    redirect_event_2klo(event_name);

    // update power statistics
    if (EVENT_CHARGING_START == event_name) {
        int32_t currentBattery = get_current_battery_uAh();
        if (has_accuracy_battery_counter && (currentBattery > dischargeStartBattery) && dischargeStartTime) {
            dischargeBatteryTotal = currentBattery - dischargeStartBattery;
            dischargeTimeTotal = nanoseconds_to_milliseconds(systemTime()) - dischargeStartTime;
        }
        else if ((!has_accuracy_battery_counter) && (currentBattery < dischargeStartBattery) && dischargeStartTime) {
            dischargeBatteryTotal = dischargeStartBattery - currentBattery;
            dischargeTimeTotal = nanoseconds_to_milliseconds(systemTime()) - dischargeStartTime;
        }
        dischargeStartTime = 0;
    }
    if (EVENT_CHARGING_END == event_name) {
        dischargeStartBattery = get_current_battery_uAh();
        dischargeStartTime = enterTime;
    }

    return ret;
}

int32_t PowerModeHandler::leaveMode()
{
    uint64_t this_time = nanoseconds_to_milliseconds(systemTime()) - enterTime;
    totalTime += this_time;
    enterTime = 0;

    int32_t battery_status = BATTERY_STATUS_UNKNOWN;
    get_battery_status(&battery_status);
    if (battery_status != BATTERY_STATUS_CHARGING) {
        int32_t currentBattery = get_current_battery_uAh();
        if (has_accuracy_battery_counter && (currentBattery > dischargeStartBattery) && dischargeStartTime) {
            dischargeBatteryTotal += currentBattery - dischargeStartBattery;
            dischargeTimeTotal += nanoseconds_to_milliseconds(systemTime()) - dischargeStartTime;
        }
        else if ((!has_accuracy_battery_counter) && (currentBattery < dischargeStartBattery) && dischargeStartTime) {
            dischargeBatteryTotal += dischargeStartBattery - currentBattery;
            dischargeTimeTotal += nanoseconds_to_milliseconds(systemTime()) - dischargeStartTime;
        }
        dischargeStartTime = 0;
    }

    return 0;
}

int32_t PowerModeHandler::getAverageCurrentInmA()
{
    uint64_t computeTime = dischargeTimeTotal;
    int32_t dischargeTotal = dischargeBatteryTotal;
    int32_t battery_status = BATTERY_STATUS_UNKNOWN;
    get_battery_status(&battery_status);
    if ((enterTime != 0) && (battery_status != BATTERY_STATUS_CHARGING)) {
        int32_t currentBattery = get_current_battery_uAh();
        if (has_accuracy_battery_counter && (currentBattery > dischargeStartBattery) && dischargeStartTime) {
            dischargeTotal += currentBattery - dischargeStartBattery;
            computeTime += nanoseconds_to_milliseconds(systemTime()) - dischargeStartTime;
        }
        else if ((!has_accuracy_battery_counter) && (currentBattery < dischargeStartBattery) && dischargeStartTime) {
            dischargeTotal += dischargeStartBattery - currentBattery;
            computeTime += nanoseconds_to_milliseconds(systemTime()) - dischargeStartTime;
        }
    }
    // time for compute average current should more than 1min
    if (computeTime/(1000.0f*60) >= 1) {
        return dischargeTotal/(computeTime/3600.0f);
    }
    else {
        return 0;
    }
}

int32_t PowerModeHandler::getTimeInStateInSecond()
{
    uint64_t realTotalTime = totalTime;
    if (enterTime != 0) {
        realTotalTime += (nanoseconds_to_milliseconds(systemTime()) - enterTime);
    }
    return (int32_t)(realTotalTime/1000);
}

int32_t PowerModeHandler::dumpStatistics(int fd)
{
    String8 dBuffer;
    uint64_t realTotalTime = totalTime;
    if (enterTime != 0) {
        realTotalTime += (nanoseconds_to_milliseconds(systemTime()) - enterTime);
    }
    dBuffer.appendFormat("%s[%s %d][autosave %s]:Stay for %llu ms(%llu times, average current: %d mA)\n",
            (enterTime != 0)? ">": " ", pMode->mode_property, pMode->mode_id, (pMode->autosave == 0)? "off" : "on",
            realTotalTime, totalCounts, getAverageCurrentInmA());
    write(fd, dBuffer.string(), dBuffer.length());
    return 0;
}

void PowerModeHandler::resetStatistics()
{
    totalTime = 0;
    totalCounts = 0;
    enterTime = 0;
    dischargeStartBattery = 0;
    dischargeBatteryTotal = 0;
    dischargeStartTime = 0;
    dischargeTimeTotal = 0;
}

struct power_mode * PowerModeHandler::getModeProperty()
{
    return pMode;
}

void PowerModeHandler::updateModeProperty(struct power_mode *nMode)
{
    if (pMode != NULL) {
        free(pMode);
    }
    pMode = nMode;
}

struct autosave_params * PowerModeHandler::getCustAutoSaveParams()
{
    return custParams;
}

void PowerModeHandler::updateAutoSaveParam(struct autosave_params *newParams)
{
    if (custParams != NULL) {
        free(custParams);
    }
    custParams = newParams;
}

const String8 PowerModeHandler::EVENT_CHARGING_START = String8("start_charging");
const String8 PowerModeHandler::EVENT_CHARGING_END = String8("stop_charging");

KeyedVector<String8, ActionImpl>* PowerModeHandler::capActionTable = new KeyedVector<String8, ActionImpl>();

static bool isThermalSwitchEnable() {
    int32_t thermalConfig = 0;
#ifdef THERMAL_CONFIG_SWITCH_ENABLE
    // 老方案仍走这个逻辑
    thermalConfig = 1;
#else
    #if (PLATFORM_SDK_VERSION >= 29)
        // 新方案逻辑
        if (access("/sys/class/thermal/thermal_message/sconfig", F_OK) == 0) {
            // file exist
            thermalConfig = 2;
        }
    #endif
#endif
    fprintf(stderr, "isThermalSwitchEnable config=%d\n", thermalConfig);
    return thermalConfig > 0;
}

static bool getBooleanProperty(const char *key) {
    char value[PROPERTY_VALUE_MAX];
    memset(value, 0, PROPERTY_VALUE_MAX);
    property_get(key, value, "");
    printf("getBooleanProperty key=%s value=%s", key, value);
    if (strcmp(value, "true") == 0) {
        return true;
    }
    return false;
}

void PowerModeHandler::initPlatformCapability()
{
    capActionTable->clear();
#ifdef DISPLAY_FEATURE_CABC_SUPPORT
    if (!getBooleanProperty("ro.vendor.cabc.enable")) {
        // 老方案仍走这个逻辑
        fprintf(stderr, "add platform_set_lcd_mode");
        capActionTable->add(String8("set_lcd_mode"), platform_set_lcd_mode);
    }
#endif
    capActionTable->add(String8("set_cpu_powermode"), platform_set_cpu_powermode);
    capActionTable->add(String8("set_cpu_custprofile"), platform_set_cpu_custprofile);
    capActionTable->add(String8("set_dfps_mode"), platform_set_dfps_mode);
    if (isThermalSwitchEnable()) {
        capActionTable->add(String8("set_thermal_config"), platform_set_thermal_mode);
    }
#ifdef DISPLAY_FEATURE_BCBC_SUPPORT
    if (!getBooleanProperty("ro.vendor.bcbc.enable")) {
        // 老方案仍走这个逻辑
        fprintf(stderr, "add platform_set_bcbc_cust_state");
        capActionTable->add(String8("set_bcbc_cust_state"), platform_set_bcbc_cust_state);
    }
#endif
#ifdef DISPLAY_FEATURE_DFPS_SUPPORT
    if (!getBooleanProperty("ro.vendor.dfps.enable")) {
        // 老方案仍走这个逻辑
        fprintf(stderr, "add platform_set_dfps_cust_state");
        capActionTable->add(String8("set_dfps_cust_state"), platform_set_dfps_cust_state);
    }
#endif

    //init device powerstate
    init_device_powerstate();
}

ActionImpl PowerModeHandler::getActionImplForName(String8 actionName)
{
    ActionImpl actionImpl = NULL;
    if (capActionTable->indexOfKey(actionName) >= 0) {
        actionImpl = capActionTable->valueFor(actionName);
    }
    return actionImpl;
}

bool PowerModeHandler::eventOutOfCap(const char *eventName)
{
    return false;
}
