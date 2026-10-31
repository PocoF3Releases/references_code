#ifndef MIUI_UTILS_LOG_H
#define MIUI_UTILS_LOG_H

#ifdef BUILD_FOR_NDK
#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "MiuiSDK"
#endif

#ifndef LOG_PRI
#define LOG_PRI __android_log_print
#endif

#define ALOG(TAG, LEVEL, ...) do {__android_log_print(ANDROID_LOG_##LEVEL, TAG, __VA_ARGS__); } while(0)
#define LOGD(...) ALOG(LOG_TAG, DEBUG, __VA_ARGS__)

#define ALOGI(...) ALOG(LOG_TAG, INFO, __VA_ARGS__)
#define ALOGD(...) ALOG(LOG_TAG, DEBUG, __VA_ARGS__)
#define ALOGE(...) ALOG(LOG_TAG, ERROR, __VA_ARGS__)
#define ALOGW(...) ALOG(LOG_TAG, WARN, __VA_ARGS__)
#define ALOGV(...) ALOG(LOG_TAG, VERBOSE, __VA_ARGS__)

#endif


#endif
