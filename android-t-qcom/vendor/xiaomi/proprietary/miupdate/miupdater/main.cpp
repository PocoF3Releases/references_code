
#include <android-base/logging.h>

#include "mi_updater.h"

//using mi_update_android::MiUpdater;

int main(int argc, char *argv[]) {

    char buf[PROPERTY_VALUE_MAX];

    //获得package路径
    property_get(PKG_SETUP_PROP, buf, "N/A");
    const std::string zip_path = buf;


    //const std::string zip_path = argv[1];

    LOG(INFO) << "package path: " << zip_path;

    mi_update_android::MiUpdater mi_updater{zip_path};

    if(!mi_updater.Run()){
        LOG(ERROR) << "update faild";
        property_set(PKG_SETUP_PROP, mi_update_android::MiUpdater::status[0]);
        return -1;
    }

    property_set(PKG_SETUP_PROP, mi_update_android::MiUpdater::status[1]);
    LOG(INFO) << "update success";

    return 0;
}
