#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "su.h"
extern "C"
{
#include "utils.h"
}

using namespace android;

// Why choosing this? because this IPC TRANSACTION CODE is constant
static const uint32_t OPEN_CONTENT_URI_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION + 4;
static const uint32_t OPEN_CONTENT_URI_TRANSACTION_O = IBinder::FIRST_CALL_TRANSACTION;

int send_request(struct su_context *ctx) {
    char uri[PATH_MAX] = { 0 };
    snprintf(uri, sizeof(uri), "content://%s%s", REQUESTOR_CONTENT_PROVIDER, ctx->sock_path);
    int fd = -1;
    int api_version = get_api_version();
    int code = api_version > 25 ? OPEN_CONTENT_URI_TRANSACTION_O : OPEN_CONTENT_URI_TRANSACTION;

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> am = sm->getService(String16("activity"));
    if (am != NULL) {
        Parcel data, reply;
        data.writeInterfaceToken(String16("android.app.IActivityManager"));
        data.writeString16(String16(uri));
        status_t ret = am->transact(code, data, &reply);
        if (ret == NO_ERROR) {
            int32_t exceptionCode = reply.readInt32();
            if (!exceptionCode) {
                // Success is indicated here by a nonzero int followed by the fd;
                // failure by a zero int with no data following.
                if (reply.readInt32() != 0) {
                    if (api_version >= 23) {
                        // android 6.0 should eat an extra int32
                        reply.readInt32();
                    }
                    fd = dup(reply.readFileDescriptor());
                }
            } else {
            }
        }
    }

    return fd;
}
