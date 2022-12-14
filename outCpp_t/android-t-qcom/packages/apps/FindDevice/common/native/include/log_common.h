#ifndef MY_LOG_TAG
#error "MY_LOG_TAG not defined. "
#endif // MY_LOG_TAG

#ifndef ANDROID_LOG_COMMON_H
#define ANDROID_LOG_COMMON_H

#include <android/log.h>
#include <cutils/klog.h>


#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, MY_LOG_TAG, __VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN, MY_LOG_TAG, __VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, MY_LOG_TAG, __VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, MY_LOG_TAG, __VA_ARGS__)
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, MY_LOG_TAG, __VA_ARGS__)


#ifndef KLOG_ERROR_LEVEL
#define KLOG_ERROR_LEVEL   3
#endif
#ifndef KLOG_WARNING_LEVEL
#define KLOG_WARNING_LEVEL 4
#endif
#ifndef KLOG_NOTICE_LEVEL
#define KLOG_NOTICE_LEVEL  5
#endif
#ifndef KLOG_INFO_LEVEL
#define KLOG_INFO_LEVEL    6
#endif
#ifndef KLOG_DEBUG_LEVEL
#define KLOG_DEBUG_LEVEL   7
#endif

#define KLOGE(...)   klog_write(KLOG_ERROR_LEVEL, MY_LOG_TAG ": " __VA_ARGS__)
#define KLOGW(...)   klog_write(KLOG_WARNING_LEVEL, MY_LOG_TAG ": " __VA_ARGS__)
#define KLOGD(...)   klog_write(KLOG_NOTICE_LEVEL, MY_LOG_TAG ": " __VA_ARGS__)
#define KLOGI(...)   klog_write(KLOG_INFO_LEVEL, MY_LOG_TAG ": " __VA_ARGS__)
#define KLOGV(...)   klog_write(KLOG_DEBUG_LEVEL, MY_LOG_TAG ": " __VA_ARGS__)


#define __CALLER__  __FILE__, __FUNCTION__, __LINE__
#define __CALLER_DECLARE__ const char * __file__, const char * __function__, unsigned int __line__
#define __CALLER_USE__ __file__, __function__, __line__
#define __CALLER_FORMAT__ "file: %s, function: %s, line: %u"


#endif //ANDROID_LOG_COMMON_H
