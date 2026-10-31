//
// Created by tanxiaoyan on 19-5-8.
//

#ifndef ANDROID_MISYSUTIL_H
#define ANDROID_MISYSUTIL_H

#include <cutils/log.h>

namespace android{
    class MisysUtil{
        public:
            int cat_vendor_file(const char *path, FILE* fp);
    };
}

#endif //ANDROID_MISYSUTIL_H
