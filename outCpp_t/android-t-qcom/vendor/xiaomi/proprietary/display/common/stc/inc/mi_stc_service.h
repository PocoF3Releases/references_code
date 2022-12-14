#ifndef __MI_STC_SERVICE_H__
#define __MI_STC_SERVICE_H__

#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <IQClient.h>
#include <IQService.h>

using android::BpInterface;
using qClient::IQClient;
using android::Parcel;
using android::status_t;
using android::sp;
using android::IBinder;
using qService::IQService;

struct RGB {
    uint32_t r;
    uint32_t g;
    uint32_t b;
};

struct FACTORY_DATA {
    RGB in;
    RGB out_min;
    RGB out_max;
};

enum MiImplType {
    kMiImplPrimary = 0,
    kMiImplSecondaryBuiltIn,
    kMaxNumImpl,
};

namespace snapdragoncolor {

class IMiStcService : public android::IInterface
{
public:
    DECLARE_META_INTERFACE(MiStcService);
    enum {
        COMMAND_LIST_START = android::IBinder::FIRST_CALL_TRANSACTION,
        CONNECT_Mi_STC_CLIENT = 2,
        SET_PCC = 3,
        SET_PA = 4,
        SET_IGC = 5,
        SET_GC = 6,
        SET_LUT = 7,
        SET_SRE_IGC = 8,
        SET_DPPS_PROP = 9,
        ENABLE_STC = 10,
        SET_LOG = 11,
        SET_DISPLAY_STATE = 12,
        SET_PARAM = 13,
        SET_IGC_DITHER = 14,
        SET_SRE = 15,
        SET_EYECARE = 17,
        GET_UPDATE_STATUS = 18,
        RESTORE_BL_PCC = 19,
        SET_SRE_GC = 20,
        STC_DUMP = 21,
        SET_EYECARE_PCC = 22,
        SET_EXIF = 29,
        COMMAND_LIST_END = 0xFF,
    };

    enum PARAM_TYPE {
        PARAM_AI_DIM_STEP,
        PARAM_TYPE_MAX,
    };

    virtual void connect(const android::sp<qClient::IQClient>& client) = 0;
    virtual android::status_t dispatch(uint32_t command,
            const android::Parcel* inParcel,
            android::Parcel* outParcel) = 0;
};

class BnMiStcService : public android::BnInterface<IMiStcService>
{
public:
    virtual android::status_t onTransact( uint32_t code,
                                          const android::Parcel& data,
                                          android::Parcel* reply,
                                          uint32_t flags = 0);
};

class MiStcService : public BnMiStcService {
public:
    virtual ~MiStcService() {};
    virtual void connect(const android::sp<qClient::IQClient>& client);
    virtual android::status_t dispatch(uint32_t command,
            const android::Parcel* data,
            android::Parcel* reply);
    static void init();
private:
    MiStcService() {};
    android::sp<qClient::IQClient> mClient;
    static MiStcService *sMiStcService;
};

class BpMiStcService : public BpInterface<IMiStcService>
{
public:
    BpMiStcService(const sp<IBinder>& impl)
        : BpInterface<IMiStcService>(impl) {}

    virtual void connect(const sp<IQClient>& client) {
        Parcel data, reply;
        data.writeInterfaceToken(IMiStcService::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(client));
        remote()->transact(CONNECT_Mi_STC_CLIENT, data, &reply);
    }

    virtual android::status_t dispatch(uint32_t command, const Parcel* inParcel,
            Parcel* outParcel) {
        status_t err = (status_t) android::FAILED_TRANSACTION;
        Parcel data;
        Parcel *reply = outParcel;
        data.writeInterfaceToken(IMiStcService::getInterfaceDescriptor());
        if (inParcel && inParcel->dataSize() > 0)
            data.appendFrom(inParcel, 0, inParcel->dataSize());
        err = remote()->transact(command, data, reply);
        return err;
    }
};

} // namespace snapdragoncolor

#endif // __MI_STC_SERVICE_H__
