#ifndef _IDEVICE_IDENTIFIERS_POLICYSERVICE_H_
#define _IDEVICE_IDENTIFIERS_POLICYSERVICE_H_

#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <utils/String16.h>

namespace android {

enum {
    TRANSACT_GETSERIAL = IBinder::FIRST_CALL_TRANSACTION,
};

class IDeviceIdentifiersPolicyService : public IInterface {
public:
    DECLARE_META_INTERFACE(DeviceIdentifiersPolicyService);

    virtual String16 getSerial() = 0;
};

};

#endif
