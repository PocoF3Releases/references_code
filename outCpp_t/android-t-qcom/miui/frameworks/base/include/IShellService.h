#ifndef MIUI_ISHELLSERVICE_H
#define MIUI_ISHELLSERVICE_H

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class IShellService : public IInterface
{
public:
    DECLARE_META_INTERFACE(ShellService);
    virtual status_t getRuntimeSharedValue(const char *key) = 0;
    virtual status_t setRuntimeSharedValue(const char *key, const long value) = 0;
};

class BnShellService : public BnInterface<IShellService>
{
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel *reply, uint32_t flags = 0);
};

}

#endif
