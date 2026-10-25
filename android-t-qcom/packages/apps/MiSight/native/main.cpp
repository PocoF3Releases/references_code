
#include <unistd.h>

#include <utils/StrongPointer.h>

#include "log.h"
#include "misight_service.h"
#include "plugin_platform.h"


using namespace android::MiSight;

int main(int argc, char* argv[]) {
    auto&  platform = PluginPlatform::getInstance();
    if (platform.ParseArgs(argc, argv)) {
        MISIGHT_LOGE("parse args failed");
        return -1;
    }
    if (!platform.StartPlatform()) {
        MISIGHT_LOGE("start platform failed");
        return -1;
    }
    android::sp<MiSightService> service = new MiSightService(&platform);
    service->StartService();
    return 0;
}
