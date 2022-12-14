#include "TouchMain.h"
#include "TouchDevice.h"
#include "TouchServer.h"

int main()
{
    sp<TouchDevice> touch_device;
    sp<TouchServer> touch_server;

    touch_device = new TouchDevice();
    if (touch_device == NULL) {
        ALOGE("Failed to create touch_device\n");
        return 1;
    }

    touch_server = new TouchServer();
    if (touch_server == NULL) {
        ALOGE("Failed to create touch_server\n");
        return 1;
    }

    if (touch_device->findInputDevice() <= 0) {
        ALOGE("Failed to get touch device\n");
        return 1;
    }

    register_check_func_list();

    touch_device->threadRun();
    touch_server->run("touchevent-server", PRIORITY_BACKGROUND);

    touch_device->join();
    touch_server->join();

    unregister_check_func_list();
    return 0;
}
