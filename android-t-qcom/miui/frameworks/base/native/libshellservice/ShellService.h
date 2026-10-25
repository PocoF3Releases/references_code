#ifndef MIUI_SHELLSERVICE_H
#define MIUI_SHELLSERVICE_H

#include <utils/Errors.h>
#include <binder/BinderService.h>
#include <binder/PermissionCache.h>
#include <utils/KeyedVector.h>

#include "IShellService.h"

namespace android {


class ShellService : public BinderService<ShellService>, public BnShellService
{
    friend class BinderService<ShellService>;

public:
    static char const* getServiceName() { return "miui.shell"; }

    ShellService();
    virtual status_t dump(int fd, const Vector<String16>& args);
    virtual status_t getRuntimeSharedValue(const char *key);
    virtual status_t setRuntimeSharedValue(const char *key, long value);

private:
    KeyedVector<String8, long> mRuntimeSharedValues;
};

}

#endif
