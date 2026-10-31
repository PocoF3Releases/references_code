#include <iostream>
#include <mutex>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#ifdef QXWZ_ANDROID
#include <android/log.h>
#elif !defined(QXWZ_LINUX)
#include <log_util.h>
#endif
#include "NtripClient.h"
#include "qxwz_sdk.h"
#include "qxwz_types.h"
#include "QxwzAccount.h"

using namespace std;

#define LOG_NDEBUG 0
#define LOG_TAG "qxwzrtcm"

#ifdef QXWZ_ANDROID
#define QXWZ_LOGE(...)                  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define QXWZ_LOGW(...)                  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define QXWZ_LOGI(...)                  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#ifndef QXWZ_RELEASE
#define QXWZ_LOGD(...)                  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define QXWZ_LOGV(...)                  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#else
#define QXWZ_LOGD(...)
#define QXWZ_LOGV(...) 
#endif
#elif defined(QXWZ_LINUX)
#define QXWZ_LOGE(...) printf(__VA_ARGS__)
#define QXWZ_LOGW(...) printf(__VA_ARGS__)
#define QXWZ_LOGI(...) printf(__VA_ARGS__)
#ifndef QXWZ_RELEASE
#define QXWZ_LOGD(...) printf(__VA_ARGS__)
#define QXWZ_LOGV(...) printf(__VA_ARGS__)
#else
#define QXWZ_LOGD(...)
#define QXWZ_LOGV(...) 
#endif
#else
#define QXWZ_LOGE(...) ALOGE(__VA_ARGS__)
#define QXWZ_LOGW(...) ALOGW(__VA_ARGS__)
#define QXWZ_LOGI(...) ALOGI(__VA_ARGS__)
#ifndef QXWZ_RELEASE
#define QXWZ_LOGD(...) ALOGD(__VA_ARGS__)
#define QXWZ_LOGV(...) ALOGV(__VA_ARGS__)
#else
#define QXWZ_LOGD(...)
#define QXWZ_LOGV(...) 
#endif
#endif

#define CLIENT_STATE_NONE 0
#define CLIENT_STATE_STARTING 1
#define CLIENT_STATE_STARTED 2

#define SDK_AUTH_STATE_NONE 0
#define SDK_AUTH_STATE_AUTHING 1
#define SDK_AUTH_STATE_AUTHED 2

#define SDK_START_STATE_NONE 0
#define SDK_START_STATE_STARTING 1
#define SDK_START_STATE_STARTED 2

extern "C" NtripClient* getNtripClientInstance();
static qxwz_int32_t printf_handler(const qxwz_char_t *buf);
static qxwz_void_t sdk_on_auth(qxwz_int32_t status_code, qxwz_sdk_cap_info_t *cap_info);
static qxwz_void_t sdk_on_status(int code);
static qxwz_void_t sdk_on_data(qxwz_sdk_data_type_e type, const qxwz_void_t *data, qxwz_uint32_t len);
static qxwz_void_t sdk_on_start(qxwz_int32_t status_code, qxwz_uint32_t cap_id);
static qxwz_void_t *start(qxwz_void_t *arg);
static CorrectionDataCb globCorrectionDataCb = nullptr;
static mutex glob_api_mutex;
static int32_t sdk_auth_flag = SDK_AUTH_STATE_NONE;
static int32_t sdk_start_flag = SDK_START_STATE_NONE;
static int32_t glob_client_state = CLIENT_STATE_NONE;
static int32_t glob_loop;
static pthread_t glob_start_tid;
static NtripClient* instance = nullptr;
static string start_gga = "";
static string ak = "";
static string as = "";

static qxwz_int32_t printf_handler(const qxwz_char_t *buf)
{
    QXWZ_LOGD("%s\n", buf);
    return 0;
}

static qxwz_void_t sdk_on_auth(qxwz_int32_t status_code, qxwz_sdk_cap_info_t *cap_info)
{
    QXWZ_LOGI("on auth, code=%d\n", status_code);
    if (status_code == QXWZ_SDK_STAT_AUTH_SUCC) {
        sdk_auth_flag = SDK_AUTH_STATE_AUTHED;
    } else {
        sdk_auth_flag = SDK_AUTH_STATE_NONE;
    }
}

static qxwz_void_t sdk_on_start(qxwz_int32_t status_code, qxwz_uint32_t cap_id) {
    QXWZ_LOGI("on start:status_code=%d, cap_id=%lu\n", status_code, cap_id);
    if (status_code == QXWZ_SDK_STAT_CAP_START_SUCC) {
        sdk_start_flag = SDK_START_STATE_STARTED;
    } else {
        sdk_start_flag = SDK_START_STATE_NONE;
    }
}


static qxwz_void_t sdk_on_status(int code)
{
    QXWZ_LOGI("on status code:%d\n", code);
}

static qxwz_void_t sdk_on_data(qxwz_sdk_data_type_e type, const qxwz_void_t *data, qxwz_uint32_t len)
{
    QXWZ_LOGI("on data: %d, len=%lu\n", type, len);
    if (type == QXWZ_SDK_DATA_TYPE_RAW_NOSR) {
        if (globCorrectionDataCb != nullptr) {
            globCorrectionDataCb((uint8_t*)data, len);
        }
    }
}

static qxwz_void_t *start(qxwz_void_t *arg)
{
    int32_t ret;
    struct timespec tp = {0};
    if (ak.length() == 0 || as.length() == 0) {
        QXWZ_LOGE("empty ak or as\n");
        return NULL;
    }
    while (glob_loop && glob_client_state == CLIENT_STATE_STARTING) {
        glob_api_mutex.lock();
        if (sdk_auth_flag == SDK_AUTH_STATE_NONE) {
            ret = qxwz_sdk_auth();
            QXWZ_LOGI("auth ret:%d\n", ret);
            if (ret != QXWZ_SDK_STAT_OK) {
                glob_client_state = CLIENT_STATE_NONE;
                glob_api_mutex.unlock();
                break;
            }
            sdk_auth_flag = SDK_AUTH_STATE_AUTHING;
        } else if (sdk_auth_flag == SDK_AUTH_STATE_AUTHED) {
            if (sdk_start_flag == SDK_START_STATE_NONE) {
                ret = qxwz_sdk_start(QXWZ_SDK_CAP_ID_NOSR);
                QXWZ_LOGI("start ret:%d\n", ret);
                if (ret != QXWZ_SDK_STAT_OK) {
                    glob_client_state = CLIENT_STATE_NONE;
                    glob_api_mutex.unlock();
                    break;
                }
                sdk_start_flag = SDK_START_STATE_STARTING;
            } else if (sdk_start_flag == SDK_START_STATE_STARTED) {
                glob_client_state = CLIENT_STATE_STARTED;
		        if (start_gga.length() > 0) {
                    qxwz_sdk_upload_gga(start_gga.c_str(), start_gga.length());
		        }
            }
        }
        glob_api_mutex.unlock();
        usleep(1000 * 100);
    }

    return NULL;
}

static int32_t get_account() {
    char account[128] = {0};
    int32_t ret;
    if (ak.length() == 0 || as.length() == 0) {
       ret = getAccount(account, 128);
       if (ret == -1) {
            QXWZ_LOGE("get account failed.\n");
            return -1;
        }
        char* pos = strchr(account, '|');
        if (pos != NULL) {
            *pos = '\0';
            ak.assign(account);
	    as.assign(pos + 1);
	    return 0;
        } else {
            QXWZ_LOGE("Failed to parse account, account=>%s\n", account);
	    return -1;
	}
    }
    return 0;
}

class NtripClientImpl:public NtripClient {
public:
    ~NtripClientImpl() {};
    void startCorrectionDataStreaming(DataStreamingNtripSettings& ntripSettings,
                         CorrectionDataCb corrDataCb);
    void stopCorrectionDataStreaming();
    void updateNmeaToNtripCaster(string& nmea);

};

void NtripClientImpl::startCorrectionDataStreaming(DataStreamingNtripSettings& ntripSettings, CorrectionDataCb corrDataCb) {
    QXWZ_LOGI("startCorrectionDataStreaming is called.\n");
    QXWZ_LOGI("sdk verison is %s, build info is %s.\n", qxwz_sdk_version(), qxwz_sdk_get_build_info());
    int32_t ret;
    qxwz_sdk_config_t sdk_config = {};
    qxwz_sdk_serv_conf_t serv_conf = {"cs.api.qxwz.com", 8443, "cs.oss.qxwz.com", 8009};
    if (corrDataCb == nullptr) {
        QXWZ_LOGE("corrDataCb is null.\n");
        return;
    }
    glob_api_mutex.lock();
    if (glob_client_state != CLIENT_STATE_NONE) {
        QXWZ_LOGE("starting or started, try later.\n");
        glob_api_mutex.unlock();
        return;
    }
    QXWZ_LOGD("gga=>%s\n", ntripSettings.nmeaGGA.c_str());

    start_gga.assign(ntripSettings.nmeaGGA);	
    ret = get_account();
    if (ret < 0) {
        glob_api_mutex.unlock();
        return;
    }
    glob_client_state = CLIENT_STATE_STARTING;
    sdk_config.key_type = QXWZ_SDK_KEY_TYPE_AK,
    strcpy(sdk_config.key, ak.c_str());
    strcpy(sdk_config.secret, as.c_str());
    strcpy(sdk_config.dev_type, ntripSettings.mountPoint.c_str());

    sdk_config.status_cb = sdk_on_status;
    sdk_config.auth_cb = sdk_on_auth;
    sdk_config.start_cb = sdk_on_start;
    sdk_config.data_cb = sdk_on_data;

    qxwz_sdk_set_printf_handler(printf_handler);
    qxwz_sdk_config(QXWZ_SDK_CONF_SERV, &serv_conf);
    qxwz_sdk_init(&sdk_config);
    glob_loop = 1;
    ret = pthread_create(&glob_start_tid, NULL, start, NULL);
    if (ret < 0) {
        glob_client_state = CLIENT_STATE_NONE;
        glob_api_mutex.unlock();
        return;
    }
    globCorrectionDataCb = corrDataCb;
    glob_api_mutex.unlock();
}

void NtripClientImpl::stopCorrectionDataStreaming() {
    QXWZ_LOGI("stopCorrectionDataStreaming is called.\n");
    glob_api_mutex.lock();
    if (glob_client_state == CLIENT_STATE_NONE) {
        glob_api_mutex.unlock();
        return;
    }
    qxwz_sdk_cleanup();
    globCorrectionDataCb = nullptr;
    glob_loop = 0;
    glob_api_mutex.unlock();
    pthread_join(glob_start_tid, NULL);
    glob_client_state = CLIENT_STATE_NONE;
    sdk_auth_flag = SDK_AUTH_STATE_NONE;
    sdk_start_flag = SDK_START_STATE_NONE;
}

void NtripClientImpl::updateNmeaToNtripCaster(string& nmea) {
    QXWZ_LOGI("updateNmeaToNtripCaster is called.\n");
    glob_api_mutex.lock();
    if (glob_client_state == CLIENT_STATE_STARTED) {
        qxwz_sdk_upload_gga(nmea.c_str(), nmea.length());
    }
    glob_api_mutex.unlock();
}

NtripClient* getNtripClientInstance() {
    QXWZ_LOGI("getNtripClientInstance is called.\n");
    if (instance == nullptr) {
        glob_api_mutex.lock();
        if (instance == nullptr) {
            instance = new NtripClientImpl();
        }
	glob_api_mutex.unlock();
    }
    return instance;
}
