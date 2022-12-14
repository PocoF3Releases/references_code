//
// Created by tanxiaoyan on 19-5-8.
//
#include "MisysUtil.h"
#include <vendor/xiaomi/hardware/misys/1.0/IMiSys.h>
#include <utils/String8.h>

using namespace android;
using ::vendor::xiaomi::hardware::misys::V1_0::IMiSys;
using ::vendor::xiaomi::hardware::misys::V1_0::IReadResult;

int MisysUtil::cat_vendor_file(const char *path,FILE* fp){
    android::sp<IMiSys> service = IMiSys::getService();
    if (service == nullptr) {
        ALOGE("Unable to initialize the HIDL");
        return -1;
    }
    const char *name = strrchr(path,'/');
    int len  = name - path;
    char dir[len+2];
    snprintf(dir,sizeof(dir),"%s",path);
    fprintf(fp,"------ cat(%s) ------\n",path);
    service->MiSysReadFile(dir, name+1, [&](auto& readFileResult){
        fprintf(fp,"%s\n\n",readFileResult.data.c_str());
    });
    return 0;
}
