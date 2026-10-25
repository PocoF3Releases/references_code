#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>

#include <binder/Parcel.h>
#include <binder/IServiceManager.h>

namespace android {

static sp<IServiceManager> sm;

int32_t system_info_dump(char *service_name)
{
    if (sm == NULL) {
        sm = defaultServiceManager();
    }
    Vector<String16> args;
    Vector<String16> services = sm->listServices();
    const size_t N = services.size();
    for (size_t i=0; i<N; i++) {
        if (String16(service_name) == services[i]) {
            sp<IBinder> service = sm->checkService(services[i]);
            if (service != NULL) {
                int err = service->dump(STDOUT_FILENO, args);
            }
            return 0;
        }
    }
    return -1;
}

int32_t dumpstate_silent(void)
{
    char buffer[65536];
    int i, s;

    /* start the dumpstate service */
    property_set("ctl.start", "dumpstate:-q");

    /* socket will not be available until service starts */
    for (i = 0; i < 20; i++) {
        s = socket_local_client("dumpstate",
                             ANDROID_SOCKET_NAMESPACE_RESERVED,
                             SOCK_STREAM);
        if (s >= 0)
            break;
        /* try again in 1 second */
        sleep(1);
    }

    if (s < 0) {
        fprintf(stderr, "Failed to connect to dumpstate service\n");
        exit(1);
    }

    while (1) {
        int length = read(s, buffer, sizeof(buffer));
        if (length <= 0)
            break;
        fwrite(buffer, 1, length, stdout);
    }

    close(s);
    return 0;
}

};
