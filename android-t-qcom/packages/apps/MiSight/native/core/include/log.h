/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight log common define head file
 *
 */

#ifndef MISIGHT_LOG_BASE_H
#define MISIGHT_LOG_BASE_H
#include <string>

#include <log/log.h>

namespace android {
namespace MiSight {

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "misight"
#ifndef LOG_PRINT
#define LOG_PRINT 0

#endif

#define MISIGHT_LOGV(...) ALOGI(__VA_ARGS__)
#define MISIGHT_LOGD(fmt, args...) \
    do {\
        (!LOG_PRINT) ?  ((void)ALOG(LOG_DEBUG, LOG_TAG, "%s: " fmt, __func__, ##args)) \
            : ((void)printf("%s %s: " fmt"\n", LOG_TAG, __func__, ##args)); \
    } while (0);

#define MISIGHT_LOGI(fmt, args...) \
    do {\
        (!LOG_PRINT) ?  ((void)ALOG(LOG_INFO, LOG_TAG, "%s: " fmt, __func__, ##args)) \
            : ((void)printf("%s %s: " fmt"\n", LOG_TAG, __func__, ##args)); \
    } while (0);

#define MISIGHT_LOGW(fmt, args...) \
    do {\
        (!LOG_PRINT) ?  ((void)ALOG(LOG_WARN, LOG_TAG, "%s: " fmt, __func__, ##args)) \
            : ((void)printf("%s %s: " fmt"\n", LOG_TAG, __func__, ##args)); \
    } while (0);

#define MISIGHT_LOGE(fmt, args...) \
    do {\
        (!LOG_PRINT) ?  ((void)ALOG(LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ##args)) \
            : ((void)printf("%s %s: " fmt"\n", LOG_TAG, __func__, ##args)); \
    } while (0);

}// namespace MiSight
}// namespace andrid
#endif
