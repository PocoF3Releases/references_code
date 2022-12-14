
#include "platform_common.h"

#include "plugin_platform.h"
namespace android {
namespace MiSight {
namespace PlatformCommon {
bool InitPluginPlatform(bool start)
{
    auto &platform = PluginPlatform::getInstance();
    if (platform.IsReady()) {
        return true;
    }

    const int argc = 7;
    char argv0[] = "/system/bin/misight";
    char argv1[] = "-configDir";
    char argv2[] = "/data/test/config/";
    char argv3[] = "-configName";
    char argv4[] = "plugin_config_normal";
    char argv5[] = "-cloudCfgDir";
    char argv6[] = "/data/test/cloudcfg";

    char* argv[argc] = {argv0, argv1, argv2, argv3, argv4, argv5, argv6};
    if (platform.ParseArgs(argc, argv) != 0) {
        printf("parse args failed\n");
        return false;
    }

    if (platform.GetPluginConfigPath() != "/data/test/config/plugin_config_normal") {
        printf("config path invalid\n");
        return false;
    }
    if (start) {
        if (!platform.StartPlatform()) {
            printf("start platform failed\n");
            return false;
        }
        if (!platform.IsReady()) {
            printf("platform not ready\n");
            return false;
        }
    }
    return true;
}
}
}
}