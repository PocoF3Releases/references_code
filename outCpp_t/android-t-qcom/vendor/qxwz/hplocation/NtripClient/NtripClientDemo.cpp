#include "NtripClient.h"
#include <dlfcn.h>
#include <iostream>
#include <unistd.h>
#include "string.h"

#define DEMO_GGA_STR        "$GPGGA,000001,3112.518576,N,12127.901251,E,1,8,1,0,M,-32,M,3,0*4B"

using namespace std;

typedef NtripClient* (*creator_t)();

void corrDataCb(uint8_t *correctionData, uint32_t lengthInBytes) {
    cout << "recv rtcm, len=" << lengthInBytes << endl;
}

int main() {
    int tick = 0;
    void* handle = dlopen("libqxwz_oss_client.so", RTLD_LAZY);
    creator_t creator = (creator_t)dlsym(handle, "getNtripClientInstance");
    NtripClient* ntripClient = creator();
    struct DataStreamingNtripSettings setting;
    setting.userName.assign("xiaomi");
    setting.password.assign("xiaomi");
    setting.mountPoint.assign("M2011K2C");
    setting.nmeaGGA.assign(DEMO_GGA_STR);
    ntripClient->startCorrectionDataStreaming(setting, corrDataCb);
    string nmea(DEMO_GGA_STR);
    while (tick++ < 30) {
        ntripClient->updateNmeaToNtripCaster(nmea);
        sleep(1);
    }
    ntripClient->stopCorrectionDataStreaming();
    sleep(10);
    return 0;
}
