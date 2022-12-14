#include "NtripClient.h"
#include <dlfcn.h>
#include <iostream>
#include <unistd.h>

#define DEMO_GGA_STR        "$GPGGA,000001,3112.518576,N,12127.901251,E,1,8,1,0,M,-32,M,3,0*4B"
#define LOG_NDEBUG 0
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "cmccSdkTest"
using namespace std;

typedef NtripClient* (*creator_t)();

void corrDataCb(uint8_t *correctionData, uint32_t lengthInBytes) {
    cout << "recv rtcm, len=" << lengthInBytes << endl;
    //ALOGD("recv rtcm, len=%d",lengthInBytes);
}

int main() {
    int tick = 0;
    void* handle = dlopen("libcmcc_wrapper_client.so", RTLD_NOW);
    creator_t creator = (creator_t)dlsym(handle, "getNtripClientInstance");
    NtripClient* ntripClient = creator();
    struct DataStreamingNtripSettings setting;
    ntripClient->startCorrectionDataStreaming(setting, corrDataCb);
    string nmea(DEMO_GGA_STR);
    while (tick++ < 1000) {
        ntripClient->updateNmeaToNtripCaster(nmea);
        sleep(1);
    }
    ntripClient->stopCorrectionDataStreaming();
    sleep(10);
    return 0;
}
