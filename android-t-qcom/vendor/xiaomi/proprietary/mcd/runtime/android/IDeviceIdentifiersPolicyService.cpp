#define LOG_TAG "octvm"
#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <utils/Log.h>
#include "octvm.h"

#include "IDeviceIdentifiersPolicyService.h"

namespace android {
class BpDeviceIdentifiersPolicyService : public BpInterface<IDeviceIdentifiersPolicyService>
{
public:
    BpDeviceIdentifiersPolicyService(const sp<IBinder>& impl)
        : BpInterface<IDeviceIdentifiersPolicyService>(impl)
    {
    }

    virtual String16 getSerial()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IDeviceIdentifiersPolicyService::getInterfaceDescriptor());
        status_t status = remote()->transact(android::TRANSACT_GETSERIAL, data, &reply);
        if (status != NO_ERROR) {
            ALOGE("getSerial could not contract remote: %d\n", status);
            return String16("");
        }
        int32_t err = reply.readExceptionCode();
        String16 ret =  reply.readString16();
        if (err < 0) {
            ALOGE("getSerial caught exception: %d\n", err);
            return String16("");
        }
        return ret;
    }
};

IMPLEMENT_META_INTERFACE(DeviceIdentifiersPolicyService, "android.os.IDeviceIdentifiersPolicyService");
};
