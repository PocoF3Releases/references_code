
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "ShellService.h"

namespace android {

ShellService::ShellService()
{
}

const String16 sDump("android.permission.DUMP");

status_t ShellService::dump(int fd, const Vector<String16>& args)
{
    const size_t SIZE = 4096;
    char buffer[SIZE];
    String8 result;

    if (!PermissionCache::checkCallingPermission(sDump)) {
        snprintf(buffer, SIZE, "Permission Denial: "
                "can't dump miuishell from pid=%d, uid=%d\n",
                IPCThreadState::self()->getCallingPid(),
                IPCThreadState::self()->getCallingUid());
        result.append(buffer);
    } else {
        snprintf(buffer, SIZE, "nothing to dump");
        result.append(buffer);
    }
    ::write(fd, result.string(), result.size());
    return NO_ERROR;
}

status_t ShellService::getRuntimeSharedValue(const char *key)
{
    String8 strKey(key);
    long ret;
    if (mRuntimeSharedValues.indexOfKey(strKey) >= 0) {
        ret = mRuntimeSharedValues.valueFor(strKey);
    } else {
        ret = -1;
    }
    return ret;
}

status_t ShellService::setRuntimeSharedValue(const char *key, long value)
{
    String8 strKey(key);
    if (mRuntimeSharedValues.indexOfKey(strKey) >= 0) {
        mRuntimeSharedValues.editValueFor(strKey) = value;
    } else {
        mRuntimeSharedValues.add(strKey, value);
    }
    return 1;
}

}
