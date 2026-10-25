#ifndef MIDEBUGGERDIMPL_LOG_H_
#define MIDEBUGGERDIMPL_LOG_H_

#include <async_safe/log.h>

#define MIDEBUGGERDIMPL_LOG_TAG "MiDebuggerdImpl"

#define LOGE(...) async_safe_format_log(ANDROID_LOG_ERROR, MIDEBUGGERDIMPL_LOG_TAG, ##__VA_ARGS__)
#define LOGW(...) async_safe_format_log(ANDROID_LOG_WARN, MIDEBUGGERDIMPL_LOG_TAG, ##__VA_ARGS__)
#define LOGI(...) async_safe_format_log(ANDROID_LOG_INFO, MIDEBUGGERDIMPL_LOG_TAG, ##__VA_ARGS__)
#define LOGV_IMPL(...) async_safe_format_log(ANDROID_LOG_VERBOSE, MIDEBUGGERDIMPL_LOG_TAG, ##__VA_ARGS__)

#ifdef NDEBUG
#define LOGV(...)                  \
  do {                             \
    if (0) {                       \
      LOGV_IMPL(__VA_ARGS__);      \
    }                              \
  } while (0)
#else
#define LOGV(...) LOGV_IMPL(__VA_ARGS__)
#endif

#define LOG_ALWAYS_FATAL(...) async_safe_fatal(__VA_ARGS__)

#endif  // MIDEBUGGERDIMPL_LOG_H_