#ifndef OCTVM_POWERMODE_HANDLER_H
#define OCTVM_POWERMODE_HANDLER_H

#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/KeyedVector.h>
#include <cutils/properties.h>

#include "octvm.h"
#include "powermode.h"
#include "drv/platform_power.h"

using namespace android;

//power mode handler
class PowerModeHandler {
public:
    PowerModeHandler(struct power_mode *sMode);
    ~PowerModeHandler();

    struct power_mode *getModeProperty();

    struct autosave_params *getCustAutoSaveParams();

    void updateModeProperty(struct power_mode *nMode);

    void updateAutoSaveParam(struct autosave_params *custParams);

    void removeAllEventActions();

    int32_t addEventActions(const char *event_name, KeyedVector<String8, String8>& actions);
    //int32_t addEventActions(int32_t event_id, KeyedVector<String8, String8>& actions);

    int32_t enterMode();

    int32_t handleTriggerEvent(const char *event_name, int32_t flags);
    //int32_t handleTriggerEvent(int32_t event_id, int32_t flags);

    int32_t leaveMode();

    int32_t dumpStatistics(int fd);

    int32_t getAverageCurrentInmA();

    int32_t getTimeInStateInSecond();

protected:
    //used for statistics
    uint64_t totalTime;
    uint64_t totalCounts;
    uint64_t enterTime;
    uint32_t dischargeStartBattery;
    uint32_t dischargeBatteryTotal;
    uint64_t dischargeStartTime;
    uint64_t dischargeTimeTotal;
    void resetStatistics();

private:
    //indicate id, type, and name
    struct power_mode *pMode;
    //if autosave enable, custom with below params
    struct autosave_params *custParams;

    class EventAction {
        friend class PowerModeHandler;
    public:
        EventAction(String8 action_name, String8 action_param, ActionImpl action_impl) {
            actionName = action_name;
            actionParam = action_param;
            doAction = action_impl;
        }
        EventAction(){};
        ~EventAction(){};

        String8 actionName;
        String8 actionParam;
        int32_t (*doAction)(const char *);
    };
    //action for event trigger
    KeyedVector<struct trigger_event*, KeyedVector<String8, EventAction*> > eventActions;

private:
    static KeyedVector<String8, ActionImpl> *capActionTable;
public:
    static const String8 EVENT_CHARGING_START;
    static const String8 EVENT_CHARGING_END;

    static void initPlatformCapability();
    static ActionImpl getActionImplForName(String8 action_name);
    static bool eventOutOfCap(const char *eventName);
};

#endif
