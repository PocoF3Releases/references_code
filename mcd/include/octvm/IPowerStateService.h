#ifndef OCTVM_IPOWERSTATESERVICE_H
#define OCTVM_IPOWERSTATESERVICE_H

#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include "powermode.h"
#include "freezeProcessApi.h"

namespace android {

class IPowerStateService: public IInterface {
public:
    enum {
        POWER_SAVE_FEATURE_ENABLE = IBinder::FIRST_CALL_TRANSACTION + 0,
        POWER_STATE_MODE_START = IBinder::FIRST_CALL_TRANSACTION + 1,
        POWER_STATE_TRIGGER_ACTION = IBinder::FIRST_CALL_TRANSACTION + 2,
        POWER_STATE_UPDATE_CONFIG = IBinder::FIRST_CALL_TRANSACTION + 3,
        POWER_STATE_GET_AVG_CURRENT = IBinder::FIRST_CALL_TRANSACTION + 4,
        POWER_STATE_GET_TIME_IN_STATE = IBinder::FIRST_CALL_TRANSACTION + 5,
        POWER_STATE_APP_FROZEN  = IBinder::FIRST_CALL_TRANSACTION + 6,
        POWER_STATE_APP_THAWED  = IBinder::FIRST_CALL_TRANSACTION + 7,
    };

    DECLARE_META_INTERFACE(PowerStateService);

    // -------------------------------------------------------------------------
    // power state transition operation
    // -------------------------------------------------------------------------
    virtual int32_t power_state_mode_start(String16& modeName, int32_t modId) = 0;

    virtual int32_t power_state_trigger_action(String16& eventName, int32_t eventId) = 0;

    virtual int32_t power_state_update_config(const Parcel& argParcel) = 0;

    virtual int32_t power_save_feature_enable(int32_t enable) = 0;

    virtual int32_t power_state_get_avg_current(int32_t modId) = 0;

    virtual int32_t power_state_time_in_state(int32_t modId) = 0;

    virtual int32_t frozenApp(int32_t uid, Vector<int32_t>& processes) = 0;

    virtual int32_t thawedApp(int32_t uid, Vector<int32_t>& processes) = 0;
};

// ----------------------------------------------------------------------------

class BnPowerStateService: public BnInterface<IPowerStateService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
            uint32_t flags = 0);
};

} // namespace android

#endif
