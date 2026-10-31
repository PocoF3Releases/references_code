#include <iostream>
#include <mutex>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <log_util.h>
#include <cutils/properties.h>
#include "NtripClient.h"
#include "libapi.h"

#ifdef LOG_NDEBUG
#undef LOG_NDEBUG
#endif
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_NDEBUG 0
#define LOG_TAG "XM_cmcc_ClientWrapper"
#define WRAPPER_LOGE(...) ALOGE(__VA_ARGS__)
#define WRAPPER_LOGW(...) ALOGW(__VA_ARGS__)
#define WRAPPER_LOGI(...) ALOGI(__VA_ARGS__)
#define WRAPPER_LOGD(...) ALOGD(__VA_ARGS__)
#define WRAPPER_LOGV(...) ALOGV(__VA_ARGS__)

using namespace std;
extern "C" NtripClient* getNtripClientInstance();

static CorrectionDataCb globCorrectionDataCb = nullptr;
static mutex glob_api_mutex;

//static pthread_t glob_start_tid;
static NtripClient* instance = nullptr;
//static const char *p_gga = nullptr;
//static int flag_sendGGA  = 0;
/*
static void *start(void *arg)
{
    int ret;

    while (true) {
        glob_api_mutex.lock();
        WRAPPER_LOGD("try to send ");
        if(flag_sendGGA  == 1 && p_gga != nullptr){
        WRAPPER_LOGD("send gga: %s\n", p_gga);
        sendGGA(p_gga, strlen(p_gga));
        flag_sendGGA  = 0;
    }

        glob_api_mutex.unlock();

        usleep(1000 * 1000 * 30); //30secs
    }
    return NULL;
}
*/
static void correctionData_cb (void *data, int length, long count) {
    if(globCorrectionDataCb != nullptr){
        WRAPPER_LOGD("get correction data length: %d; count = %ld\n", length, count);
        globCorrectionDataCb((uint8_t*)data, length);
    }
    return;
}

static void status_cb (int status) {
    WRAPPER_LOGD("get status: %d\n", status);
    return;
}

class NtripClientImp:public NtripClient {
public:
    ~NtripClientImp() {};
    void startCorrectionDataStreaming(DataStreamingNtripSettings& ntripSettings,
                         CorrectionDataCb corrDataCb);
    void stopCorrectionDataStreaming();
    void updateNmeaToNtripCaster(string& nmea);

};

void NtripClientImp::startCorrectionDataStreaming(DataStreamingNtripSettings& ntripSettings, CorrectionDataCb corrDataCb) {
    WRAPPER_LOGD("startCorrectionDataStreaming is called.");
    //int ret;

    glob_api_mutex.lock();
/*
    ret = pthread_create(&glob_start_tid, NULL, start, NULL);
    if (ret < 0) {
        WRAPPER_LOGE("creat listening thread error");
        glob_api_mutex.unlock();
        return;
    }
*/

    globCorrectionDataCb = corrDataCb;
    //TODO: call
    //JniLib.getSdkVersion();
    char cpuId[PROPERTY_VALUE_MAX] = {0};
    property_get("ro.boot.cpuid", cpuId, "ZY"); // thanks for zhaoyuan3@xiaomi.com help
    WRAPPER_LOGD("get cpuId: %s\n", cpuId);
    // get unique id then call
    //JniLib.setDeviceID();
    // starcat string
    char deviceId[PROPERTY_VALUE_MAX] = {0};
    strcpy(deviceId, "XIMI");
    strcat(deviceId, cpuId);
    RtcmDataResponse rtcm_data = {};
    rtcm_data.cb_rtcmdata = correctionData_cb;
    RtcmStatusResponse status_data = {};
    status_data.cb_status = status_cb;
    startSdk(&rtcm_data, &status_data);
    WRAPPER_LOGD("set deviceId: %s\n", deviceId);
    setDeviceID(deviceId);
    const char *p_gga = nullptr;
    p_gga = ntripSettings.nmeaGGA.c_str();
    WRAPPER_LOGD("updateNmeaToNtripCaster is called.");
    sendGGA(p_gga, strlen(p_gga));

    glob_api_mutex.unlock();
}
void NtripClientImp::stopCorrectionDataStreaming() {
    WRAPPER_LOGD("stopCorrectionDataStreaming is called.");
    glob_api_mutex.lock();
    //TODO
    //JniLib.stopSdk();
    stopSdk();
    globCorrectionDataCb = nullptr;

    glob_api_mutex.unlock();
    //pthread_join(glob_start_tid, NULL);
}
void NtripClientImp::updateNmeaToNtripCaster(string& nmea) {
    WRAPPER_LOGD("updateNmeaToNtripCaster is called.");
    //WRAPPER_LOGD("nmea is: %s\n", nmea);
    glob_api_mutex.lock();
    //JniLib.sendGGA(nmea);
    //JniLib.getRtcmStatus(); judge if send ok
    //char gga[] = "$GPGGA,023352.55,3113.2180600,N,12148.8000000,E,1,00,1.0,-6.078,M,11.078,M,0.0,*60\r\n";
    /*
    if(flag_sendGGA == 0) {
        flag_sendGGA = 1;
        p_gga = nmea.c_str();
    }
    */
    const char *p_gga = nullptr;
    p_gga = nmea.c_str();
    sendGGA(p_gga, strlen(p_gga));
    glob_api_mutex.unlock();
}

NtripClient* getNtripClientInstance() {
    WRAPPER_LOGD("getNtripClientInstance is called.");
    if (instance == nullptr) {
        glob_api_mutex.lock();
        instance = new NtripClientImp();
        glob_api_mutex.unlock();
    }
    return instance;
}
