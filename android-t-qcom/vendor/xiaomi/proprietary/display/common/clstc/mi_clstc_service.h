#ifndef __MI_CLSTC_SERVICE_H__
#define __MI_CLSTC_SERVICE_H__

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


namespace clstc {

class IMiClstcService : public android::IInterface
{
public:
    DECLARE_META_INTERFACE(MiClstcService);
    enum {
        COMMAND_LIST_START = android::IBinder::FIRST_CALL_TRANSACTION,
        CONNECT_Mi_CLSTC_CLIENT                = 2,
        SET_LOG                                = 3,
        SHOW_LAYER_INFO                        = 4,
        SET_PCC                                = 5,
        SET_IGC                                = 6,
        SET_GC                                 = 7,
        SET_LUT                                = 8,
        COMMAND_LIST_END                       = 0xFF,
    };


    virtual void connect(const android::sp<qClient::IQClient>& client) = 0;
    virtual android::status_t dispatch(uint32_t command,
            const android::Parcel* inParcel,
            android::Parcel* outParcel) = 0;
};

class BnMiClstcService : public android::BnInterface<IMiClstcService>
{
public:
    virtual android::status_t onTransact( uint32_t code,
                                          const android::Parcel& data,
                                          android::Parcel* reply,
                                          uint32_t flags = 0);
};

class MiClstcService : public BnMiClstcService {
public:
    virtual ~MiClstcService() {};
    virtual void connect(const android::sp<qClient::IQClient>& client);
    virtual android::status_t dispatch(uint32_t command,
            const android::Parcel* data,
            android::Parcel* reply);
    static void init();
private:
    MiClstcService() {};
    android::sp<qClient::IQClient> mClient;
    static MiClstcService *sMiClstcService;
    static bool init_done;
};

class BpMiClstcService : public BpInterface<IMiClstcService>
{
public:
    BpMiClstcService(const sp<IBinder>& impl)
        : BpInterface<IMiClstcService>(impl) {}

    virtual void connect(const sp<IQClient>& client) {
        Parcel data, reply;
        data.writeInterfaceToken(IMiClstcService::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(client));
        remote()->transact(CONNECT_Mi_CLSTC_CLIENT, data, &reply);
    }

    virtual android::status_t dispatch(uint32_t command, const Parcel* inParcel,
            Parcel* outParcel) {
        status_t err = (status_t) android::FAILED_TRANSACTION;
        Parcel data;
        Parcel *reply = outParcel;
        data.writeInterfaceToken(IMiClstcService::getInterfaceDescriptor());
        if (inParcel && inParcel->dataSize() > 0)
            data.appendFrom(inParcel, 0, inParcel->dataSize());
        err = remote()->transact(command, data, reply);
        return err;
    }
};

} // namespace Clstc

#endif // __MI_CLSTC_SERVICE_H__
