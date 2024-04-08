#ifndef _DF_LOG_H_
#define _DF_LOG_H_
namespace android {

#define DF_LOGV(fmt, args...) ({\
                if(DFLOG::loglevel <= 0)\
                    ALOGD("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define DF_LOGD(fmt, args...) ({\
                if(DFLOG::loglevel <= 1)\
                    ALOGD("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define DF_LOGI(fmt, args...) ({\
                if(DFLOG::loglevel <= 2)\
                    ALOGI("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define DF_LOGW(fmt, args...) ({\
                if(DFLOG::loglevel <= 3)\
                    ALOGW("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define DF_LOGE(fmt, args...) ({\
                if(DFLOG::loglevel <= 4)\
                    ALOGE("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })


class DFLOG {
public:
    static int loglevel;
};

}
#endif
