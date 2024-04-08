//#include <vendor/xiaomi/hardware/naruto/1.0/ITouchFeature.h>
#include <vendor/xiaomi/hw/touchfeature/1.0/ITouchFeature.h>
#include <hidl/Status.h>
#include <hidl/LegacySupport.h>
#include <utils/misc.h>
#include <hidl/HidlSupport.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Singleton.h>
#include <utils/SortedVector.h>
#include <utils/String8.h>
#include <utils/threads.h>
#include <sys/prctl.h>

using ::vendor::xiaomi::hw::touchfeature::V1_0::ITouchFeature;
//using ::android::hardware::hidl_string;

//using ::vendor::xiaomi::hardware::touchfeature::V1_0::types;

using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_version;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_handle;
using namespace android;

enum MODE_CMD {
    SET_CUR_VALUE = 0,
    GET_CUR_VALUE,
    GET_DEF_VALUE,
    GET_MIN_VALUE,
    GET_MAX_VALUE,
    GET_MODE_VALUE,
    RESET_MODE,
    SET_LONG_VALUE,
};


enum  MODE_TYPE {
    Touch_Game_Mode        = 0,
    Touch_Active_MODE      = 1,
    Touch_UP_THRESHOLD     = 2,
    Touch_Tolerance		   = 3,
    Touch_Wgh_Min	       = 4,
    Touch_Wgh_Max	       = 5,
    Touch_Wgh_Step	       = 6,
    Touch_Edge_Filter      = 7,
    Touch_Mode_NUM         = 8,
};

int main(int argc, char *argv[])
{
    int ret;
    int i;
    int mode;
    int value;
    int touchId;
    hidl_vec<int32_t> result;
    result.resize(10);
    static android::sp<ITouchFeature> service = NULL;

    service = ITouchFeature::getService();
    if (service == NULL) {
        printf("cant get touch service");
    } else {
        if (argc == 1) {
            printf("touchapp s[set] [touchId] [mode] [value]\n");
            printf("or \ntouchapp g[set] [touchId] [mode]\n");
            printf("or \ntouchapp p [touchId]\n");
                return -1;
            }
            if (argc == 3) {
                    touchId = atoi(argv[2]);
                    printf("\n touchId:%d\n", touchId);
                    if (argv[1][0] == 'p') {
                            for(i = 0; i < Touch_Mode_NUM; i++) {
                                    ret = service->getModeCurValue(touchId, i);
                                    printf("mode:%4d, Cur:%4d\n", i, ret);
                                    ret = service->getModeDefaultValue(touchId, i);
                                    printf("mode:%4d, Def:%4d\n", i, ret);
                                    ret = service->getModeMinValue(touchId, i);
                                    printf("mode:%4d, Min:%4d\n", i, ret);
                                    ret = service->getModeMaxValue(touchId, i);
                                    printf("mode:%4d, Max:%4d\n", i, ret);
                                    service->getModeValue(touchId, i, [&] (auto &result) {
                                    printf("all:%d,%d, %d, %d\n", result[0], result[1], result[2], result[3]);
                                    });
                            }
                    }
                    if (argv[1][0] == 'r') {
                            ret = service->modeReset(touchId, 0);
                            printf("reset all mode\n");
                    }
                    return 0;
            }

            if (argc == 5) {
                    touchId = atoi(argv[2]);
                    printf("\n touchId:%d\n", touchId);
                    if (argv[1][0] != 's') {
                            printf("please input:\n  touchapp s[set] [touchId] mode   value\n");
                            return -1;
                    }

                    mode = atoi(argv[3]);
                    if (mode < 0 && mode >= Touch_Mode_NUM) {
                            printf("mode value error: mode > 0 && mdoe < Touch_Mode_NUM, but currut mode:%d\n", mode);
                            return -1;
                    }

                    value = atoi(argv[4]);
                    printf("%c mode:%d value:%d\n", argv[1][0], mode, value);
            } else if (argc == 4) {
                    touchId = atoi(argv[2]);
                    printf("\n touchId:%d\n", touchId);
                    if (argv[1][0] != 'g') {
                            printf("please input:\n  touchapp g[get]  [touchId] mode\n");
                            return -1;
                    }
                    mode = atoi(argv[3]);
                    if (mode < 0 && mode >= Touch_Mode_NUM) {
                            printf("mode value error: mode > 0 && mdoe < Touch_Mode_NUM, but currut mode:%d\n", mode);
                            return -1;
                    }
                    printf("%c mode:%d\n", argv[1][0], mode);
            }

            if (argv[1][0] == 's') {
                    ret = service->setModeValue(touchId, mode, value);
            } else if (argv[1][0] == 'l') {
                    touchId = atoi(argv[2]);
                    int long_mode = atoi(argv[3]);
                    int buflen = argc - 4;
                    hidl_vec<int32_t> buf(256);

                    for (int i = 0; i < buflen; i++)
                        buf[i] = atoi(argv[4 + i]);
                    printf("setModeLongValue: id:%d, mode:%d, len: %d, value1:%s\n", touchId, long_mode, buflen, argv[4]);

                    ret = service->setModeLongValue(touchId, long_mode, buflen, (const hidl_vec<int32_t>)buf);
            } else {
                    ret = service->getModeCurValue(touchId, mode);
                    printf("get the cur value:%d\n", ret);
            }

    }
        return 0;
}

