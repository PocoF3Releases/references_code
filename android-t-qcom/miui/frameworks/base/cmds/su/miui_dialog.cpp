#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "su.h"

using namespace android;

static const uint32_t OPEN_CONTENT_URI_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION + 4;

void miui_show_disabled() {
    char uri[PATH_MAX] = {};
    sprintf(uri, "content://%s/disabled", REQUESTOR_CONTENT_PROVIDER);

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> am = sm->getService(String16("activity"));
    if (am != NULL) {
        Parcel data, reply;
        data.writeInterfaceToken(String16("android.app.IActivityManager"));
        data.writeString16(String16(uri));
        am->transact(OPEN_CONTENT_URI_TRANSACTION, data, &reply);
    }
}
