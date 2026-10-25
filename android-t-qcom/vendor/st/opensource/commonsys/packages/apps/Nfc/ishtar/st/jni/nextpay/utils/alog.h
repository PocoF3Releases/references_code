//
// Created by Feng on 1/1/21.
//

#ifndef MI_CRACKER_ALOG_H
#define MI_CRACKER_ALOG_H

#include <android/log.h>
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "TSMClient_native", __VA_ARGS__)
#define LOGD(...)
#define LOGE(...)

#define PRINT(...) LOGD(__VA_ARGS__)
#define ERR(...)  LOGE(__VA_ARGS__ )



#endif //MI_CRACKER_ALOG_H
