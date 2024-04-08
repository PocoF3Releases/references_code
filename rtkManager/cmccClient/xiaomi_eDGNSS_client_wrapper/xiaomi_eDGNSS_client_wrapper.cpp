#include <dlfcn.h>
#include <iostream>
#include <unistd.h>
#include <mutex>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <log_util.h>
#include <cutils/properties.h>
#include "NtripClient.h"
#ifdef LOG_NDEBUG
#undef LOG_NDEBUG
#endif
#define LOG_NDEBUG 0
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "xiaomi_eDGNSS_client_wrapper"
#ifdef LOGD
#undef LOGD
#endif
#define LOGD(...) ALOGD(__VA_ARGS__)
using namespace std;

enum vendor {
    QXWZ,
    CMCC
};
void  loadClient();
vendor whoIsClientVendor();
extern "C" NtripClient* getNtripClientInstance();
static NtripClient* ntripClient = nullptr;
static void* handle = nullptr;

void  loadClient() {
    LOGD(" loadClient.");
    //void* handle = nullptr;
    typedef NtripClient* (*creator_t)();
    if(whoIsClientVendor() == CMCC) {
        LOGD(" loadCmccClient.");
        handle = dlopen("libcmcc_wrapper_client.so", RTLD_NOW);
    } else {
        LOGD(" loadQxwzClient.");
        handle = dlopen("libqxwz_oss_client.so", RTLD_NOW);
    }

    creator_t creator = (creator_t)dlsym(handle, "getNtripClientInstance");
    if (nullptr != creator) {
        ntripClient = creator();
    }
    //free handle;
    //handle = nullptr;
    return;
}

vendor whoIsClientVendor() {
    char vendor[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.vendor.gnss.edgnss.vendor", vendor, "qxwz");
    if(strcmp("cmcc", vendor) == 0) {
        LOGD(" use cmcc client");
        return CMCC;
    } else {
        LOGD(" use qxwz client");
        return QXWZ;
    }
}


class NtripClientIm:public NtripClient {
public:
    ~NtripClientIm() {};
    void startCorrectionDataStreaming(DataStreamingNtripSettings& ntripSettings,
                         CorrectionDataCb corrDataCb);
    void stopCorrectionDataStreaming();
    void updateNmeaToNtripCaster(string& nmea);

};

void NtripClientIm::startCorrectionDataStreaming(DataStreamingNtripSettings& ntripSettings, CorrectionDataCb corrDataCb) {
    LOGD(" startCorrectionDataStreaming is called.");
    loadClient();
    if(ntripClient != nullptr) {
        ntripClient->startCorrectionDataStreaming(ntripSettings, corrDataCb);
    }else {
	LOGD(" startCorrectionDataStreaming is called with nullptr");
    }
}

void NtripClientIm::stopCorrectionDataStreaming() {
    LOGD(" stopCorrectionDataStreaming is called.");
    if(ntripClient != nullptr) {
        ntripClient->stopCorrectionDataStreaming();
    }else {
        LOGD(" stopCorrectionDataStreaming is called with nullptr");
    }
}

void NtripClientIm::updateNmeaToNtripCaster(string& nmea) {
    LOGD(" updateNmeaToNtripCaster is called.");
    if(ntripClient != nullptr) {
        ntripClient->updateNmeaToNtripCaster(nmea);
    }else {
        LOGD(" updateNmeaToNtripCaster is called with nullptr");
    }
}


NtripClient* getNtripClientInstance() {
    LOGD(" getNtripClientInstance is called.");
    //loadClient();
    return new NtripClientIm();
}
