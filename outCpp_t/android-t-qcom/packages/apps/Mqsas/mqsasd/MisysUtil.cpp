//
// Created by tanxiaoyan on 19-5-8.
//
#include "MisysUtil.h"

using namespace android;

int MisysUtil::cat_vendor_file(const char *path,FILE* fp){
    ALOGE("There is no misys on this device");
    fprintf(fp,"------ cat(%s) ------\n",path);
    fprintf(fp,"can not cat this file because there is no misys.\n");
    return -1;
}
