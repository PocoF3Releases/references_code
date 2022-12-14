#ifndef URL_HOOK_LOG_H
#define URL_HOOK_LOG_H

#ifndef LOG_TAG
#define LOG_TAG "QaDaemonCommand: "
#endif

extern bool g_url_hook_enable;

#define DEBUG(...)                                                             \
  do {                                                                         \
    if (g_url_hook_enable) {                                                   \
      ALOGI(LOG_TAG __VA_ARGS__);                                                      \
    }                                                                          \
  } while (0)

#define CAN_DEBUG (g_url_hook_enable)

#define ENABLE_DEBUG(v)                                                        \
  do {                                                                         \
    g_url_hook_enable = (v);                                                   \
  } while (0)

#include <cutils/log.h>

#endif
