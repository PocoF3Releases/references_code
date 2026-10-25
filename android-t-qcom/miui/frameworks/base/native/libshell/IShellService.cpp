#define LOG_TAG "IShellService"

#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include "IShellService.h"

namespace android {

enum {
    COMMAND_GET_RUNTIME_SHARED_VALUE = IBinder::FIRST_CALL_TRANSACTION,
    COMMAND_SET_RUNTIME_SHARED_VALUE,
};

class BpShellService : public BpInterface<IShellService>
{
public:
    BpShellService(const sp<IBinder>& impl) : BpInterface<IShellService>(impl)
    {
    }

    virtual status_t getRuntimeSharedValue(const char *key)
    {
        Parcel data, reply;

        data.writeInterfaceToken(IShellService::getInterfaceDescriptor());
        data.writeCString(key);

        remote()->transact(COMMAND_GET_RUNTIME_SHARED_VALUE, data, &reply);
        return reply.readInt64();
    }

    virtual status_t setRuntimeSharedValue(const char *key, const long value)
    {
        Parcel data, reply;

        data.writeInterfaceToken(IShellService::getInterfaceDescriptor());
        data.writeCString(key);
        data.writeInt64(value);

        remote()->transact(COMMAND_SET_RUNTIME_SHARED_VALUE, data, &reply);

        return reply.readInt32();
    }

};

IMPLEMENT_META_INTERFACE(ShellService, "miui.IShellService");

#define CHECK_MIUI_SHELL_PERMISSION  \
    if (! checkCallingPermission(String16("android.miui.permission.SHELL"))) { \
        ALOGE("Permission Denial: can't use the miui shell uid=%d pid=%d", \
               IPCThreadState::self()->getCallingPid(), IPCThreadState::self()->getCallingUid()); \
        return PERMISSION_DENIED; \
    }

status_t BnShellService::onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
{
    switch (code) {
    case COMMAND_GET_RUNTIME_SHARED_VALUE:
    {
        CHECK_MIUI_SHELL_PERMISSION;
        CHECK_INTERFACE(IShellService, data, reply);
        reply->writeInt64(getRuntimeSharedValue(data.readCString()));
        return NO_ERROR;
    }
    case COMMAND_SET_RUNTIME_SHARED_VALUE:
    {
        CHECK_MIUI_SHELL_PERMISSION;
        CHECK_INTERFACE(IShellService, data, reply);
        const char *key = data.readCString();
        const long value = data.readInt64();
        reply->writeInt32(setRuntimeSharedValue(key, value));
        return NO_ERROR;
    }
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

}
