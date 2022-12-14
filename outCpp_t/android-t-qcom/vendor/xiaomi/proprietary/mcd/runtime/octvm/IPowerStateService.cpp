#define LOG_TAG "octvm_runtime"

#include <stdint.h>
#include <sys/types.h>

#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "octvm/IPowerStateService.h"

namespace android {

enum {
    EX_SECURITY = -1,
    EX_BAD_PARCELABLE = -2,
    EX_ILLEGAL_ARGUMENT = -3,
    EX_NULL_POINTER = -4,
    EX_ILLEGAL_STATE = -5,
    EX_NETWORK_MAIN_THREAD = -6,
    EX_HAS_REPLY_HEADER = -128,  // special; see below
};

class BpPowerStateService: public BpInterface<IPowerStateService>
{
public:
    BpPowerStateService(const sp<IBinder>& impl)
        : BpInterface<IPowerStateService>(impl)
    {
    }

    //enable/disable power save feature
    virtual int32_t power_save_feature_enable(int32_t enable)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IPowerStateService::getInterfaceDescriptor());
        data.writeInt32(enable);
        status_t status = remote()->transact(BnPowerStateService::POWER_SAVE_FEATURE_ENABLE, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("power_save_feature_enable() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("power_save_feature_enable() caught exception %d\n", err);
            return -1;
        }
        return 0;
    }
    //preparing to enter the customed power mode
    virtual int32_t power_state_mode_start(String16& modeName, int32_t modId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IPowerStateService::getInterfaceDescriptor());
        data.writeString16(modeName);
        data.writeInt32(modId);
        status_t status = remote()->transact(BnPowerStateService::POWER_STATE_MODE_START, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("power_state_mode_start() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("power_state_mode_start() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //when in a mode, some event may trigger an action
    virtual int32_t power_state_trigger_action(String16& eventName, int32_t eventId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IPowerStateService::getInterfaceDescriptor());
        data.writeString16(eventName);
        data.writeInt32(eventId);
        status_t status = remote()->transact(BnPowerStateService::POWER_STATE_TRIGGER_ACTION, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("power_state_trigger_action() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("power_state_trigger_action() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //update power mode config, not require to restart the process
    virtual int32_t power_state_update_config(const Parcel& argParcel)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IPowerStateService::getInterfaceDescriptor());
        data.appendFrom(&argParcel, 0, argParcel.dataSize());
        status_t status = remote()->transact(BnPowerStateService::POWER_STATE_UPDATE_CONFIG, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("power_state_update_config() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("power_state_update_config() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //get the average current when stay in the mode
    virtual int32_t power_state_get_avg_current(int modId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IPowerStateService::getInterfaceDescriptor());
        data.writeInt32(modId);
        status_t status = remote()->transact(BnPowerStateService::POWER_STATE_GET_AVG_CURRENT, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("power_state_get_avg_current() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("power_state_get_avg_current() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //get the total time during stay in the mode
    virtual int32_t power_state_time_in_state(int modId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IPowerStateService::getInterfaceDescriptor());
        data.writeInt32(modId);
        status_t status = remote()->transact(BnPowerStateService::POWER_STATE_GET_TIME_IN_STATE, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("power_state_time_in_state() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("power_state_time_in_state() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }

    virtual int32_t frozenApp(int32_t uid, Vector<int32_t>& processes)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IPowerStateService::getInterfaceDescriptor());
        data.writeInt32(uid);
        int pNum = processes.size();
        data.writeInt32(pNum);
        for (size_t i = 0; i < pNum; i++) {
            data.writeInt32(processes[i]);
        }
        remote()->transact(POWER_STATE_APP_FROZEN, data, &reply);
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            return -1;
        }
        return reply.readInt32();
    }

    virtual int32_t thawedApp(int32_t uid, Vector<int32_t>& processes)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IPowerStateService::getInterfaceDescriptor());
        data.writeInt32(uid);
        int pNum = processes.size();
        data.writeInt32(pNum);
        for (size_t i = 0; i < pNum; i++) {
            data.writeInt32(processes[i]);
        }
        remote()->transact(POWER_STATE_APP_THAWED, data, &reply);
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            return -1;
        }
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(PowerStateService, "miui.whetstone.power");

// ----------------------------------------------------------------------

status_t BnPowerStateService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case POWER_SAVE_FEATURE_ENABLE: {
            CHECK_INTERFACE(IPowerStateService, data, reply);
            int32_t enable = data.readInt32();
            int32_t ret = power_save_feature_enable(enable);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case POWER_STATE_MODE_START: {
            CHECK_INTERFACE(IPowerStateService, data, reply);
            String16 modeName = data.readString16();
            int32_t modeId = data.readInt32();
            int32_t ret = power_state_mode_start(modeName, modeId);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case POWER_STATE_TRIGGER_ACTION: {
            CHECK_INTERFACE(IPowerStateService, data, reply);
            String16 eventName = data.readString16();
            int32_t eventId = data.readInt32();
            int32_t ret = power_state_trigger_action(eventName, eventId);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case POWER_STATE_UPDATE_CONFIG: {
            CHECK_INTERFACE(IPowerStateService, data, reply);
            int32_t ret = power_state_update_config(data);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case POWER_STATE_GET_AVG_CURRENT: {
            CHECK_INTERFACE(IPowerStateService, data, reply);
            int32_t modeId = data.readInt32();
            int32_t ret = power_state_get_avg_current(modeId);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case POWER_STATE_GET_TIME_IN_STATE: {
            CHECK_INTERFACE(IPowerStateService, data, reply);
            int32_t modeId = data.readInt32();
            int32_t ret = power_state_time_in_state(modeId);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case POWER_STATE_APP_FROZEN: {
            CHECK_INTERFACE(IPowerStateService, data, reply);
            int uid = data.readInt32();
            int pNum = data.readInt32();
            Vector<int32_t> processes;
            for (int i = 0; i < pNum; i++) {
                processes.add(data.readInt32());
            }
            int32_t ret = frozenApp(uid, processes);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case POWER_STATE_APP_THAWED: {
            CHECK_INTERFACE(IPowerStateService, data, reply);
            int uid = data.readInt32();
            int pNum = data.readInt32();
            Vector<int32_t> processes;
            for (int i = 0; i < pNum; i++) {
                processes.add(data.readInt32());
            }
            int32_t ret = thawedApp(uid, processes);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
