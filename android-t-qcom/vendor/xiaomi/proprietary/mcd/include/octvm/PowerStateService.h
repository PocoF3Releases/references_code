#ifndef OCTVM_POWERSTATEPROXY_H
#define OCTVM_POWERSTATEPROXY_H

#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/KeyedVector.h>
#include <utils/RWLock.h>
#include <binder/BinderService.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "powermode.h"
#include "utils/cJSON.h"
#include "AutoSavingThread.h"
#include "FrozenCheckThread.h"
#include "PowerModeHandler.h"
#include "UserDefinedPowerProfile.h"
#include "octvm/IPowerStateService.h"
#include "freezeProcessApi.h"

namespace android {

//power state transition service
class PowerStateService : public BinderService<PowerStateService>, public BnPowerStateService
{
    friend class BinderService<PowerStateService>;
public:
    PowerStateService();
    ~PowerStateService();

    static bool isFeatureOn();

    static bool isServiceStarted();

    static const char* getServiceName();

    int32_t power_save_feature_enable(int32_t enable);

    int32_t power_state_mode_start(String16& modeName, int32_t modId);

    int32_t power_state_trigger_action(String16& eventName, int32_t eventId);

    int32_t power_state_update_config(const Parcel& argParcel);

    int32_t power_state_get_avg_current(int32_t modId);

    int32_t power_state_time_in_state(int32_t modId);

    int32_t frozenApp(int32_t uid, Vector<int32_t>& processes);

    int32_t thawedApp(int32_t uid, Vector<int32_t>& processes);

    int32_t dump(int fd, const Vector<String16>& args);

protected:

    int32_t FeatureEnable(bool enable);

    PowerModeHandler* getModeHandler(int32_t modId);

    PowerModeHandler* getModeHandler(String16& modeName);

    PowerModeHandler* initDefaultPowerState();

    int32_t handleModeEnter(PowerModeHandler *pmHandler);

    PowerModeHandler* addPowerState(struct power_mode *pMode);

    int32_t removePowerState(PowerModeHandler *pmHandler);

protected:
    static const String8 SName;

private:
    int32_t isPowerStatesCustomed;
    PowerModeHandler* currentModeHandler;

    RWLock mRWLock;
    KeyedVector<uint32_t, PowerModeHandler*> gHandler;
    sp<AutoSavingThread> mAutoSavingThread;
    sp<FrozenCheckThread> mFrozenCheckThread = NULL;

public:
    static struct power_mode * getSystemPowerMode(int id);
    static void addSystemDefaultPowerMode(struct power_mode *pMode);
private:
    static KeyedVector<uint32_t, struct power_mode *> *sDefaultPowerModes;
};

};

#endif
