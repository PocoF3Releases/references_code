#ifndef _MIUINDBG_LOG_H_
#define _MIUINDBG_LOG_H_

#include <cutils/log.h>

#ifdef NDEBUG
#define MILOGD(...)
#else
#define MILOGD(...) ((void)__android_log_buf_print(LOG_ID_MAIN, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif
#define MILOGI(...) ((void)__android_log_buf_print(LOG_ID_MAIN, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define MILOGW(...) ((void)__android_log_buf_print(LOG_ID_MAIN, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define MILOGE(...) ((void)__android_log_buf_print(LOG_ID_MAIN, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

#endif
