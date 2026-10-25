#ifndef ANDROID_XASSERT_H
#define ANDROID_XASSERT_H

#include <android/log.h>
#include <cutils/klog.h>

#include <stdlib.h>

#ifndef KLOG_ERROR_LEVEL
#define KLOG_ERROR_LEVEL   3
#endif

namespace xassert {

    inline
    void die(const char * errorMessage) {
        if (errorMessage) {
            klog_write(KLOG_ERROR_LEVEL, "xassert::die: %s. ", errorMessage);
            __android_log_print(ANDROID_LOG_FATAL, "xassert::die", "%s", errorMessage);
        }
        ::abort();
    }

    inline
    void sure(bool b, const char * errorMessage = NULL) {
        if (!b) { die(errorMessage); }
    }

    inline
    void impossible(bool b, const char * errorMessage = NULL) {
        sure(!b, errorMessage);
    }

};

#endif //ANDROID_XASSERT_H