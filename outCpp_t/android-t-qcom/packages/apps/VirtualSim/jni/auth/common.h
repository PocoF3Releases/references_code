#ifndef COMMON_H
#define COMMON_H

#include <android/log.h>

#define IS_DEBUG 0

typedef unsigned char uint8;

// qcom定义了COMMON_LOG_H, MTK没有定义
#ifndef COMMON_LOG_H
#define COMMON_LOG_H
#define LOGD(format, ...)  \
__android_log_print(ANDROID_LOG_DEBUG, "VSIM", (format), ##__VA_ARGS__ )

#define LOGW(format, ...)  \
__android_log_print(ANDROID_LOG_WARN, "VSIM", (format), ##__VA_ARGS__ )

#define LOGE(format, ...)  \
__android_log_print(ANDROID_LOG_ERROR, "VSIM", (format), ##__VA_ARGS__ )

#define LOGI(format, ...)  \
__android_log_print(ANDROID_LOG_INFO, "VSIM", (format), ##__VA_ARGS__ )
#endif

void print_data
(
  const uint8                  *data_ptr,
  uint32_t                        data_len
);


#endif
