#ifndef ANDROID_ACUTIL_H
#define ANDROID_ACUTIL_H

#include "../common/log.h"

#define logThreadErrorsHere() logThreadErrors(__CALLER__)

class ACUtil {
public:
    static void logThreadErrors(__CALLER_DECLARE__);
};

#endif //ANDROID_ACUTIL_H
