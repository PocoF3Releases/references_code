#include <utils/Log.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <iostream>
#include "BatteryLife.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "batterylife-test"

using namespace android;
using std::string;
using namespace::std;

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::vendor::xiaomi::hardware::batterylife::V1_0::IBatteryLife;

int main(void)
{
    int value = 0;
    int chargefull = 0;

    sp<IBatteryLife> service = IBatteryLife::getService();
    if (service == nullptr) {
        cerr << "Unable to initialize the HIDL!" << endl;
        return -1;
    }

    cout << "Initialize the BatteryLife HIDL successfully." << endl;

    // Check BatteryLife control is supported on such device
    const bool support = service->isBatteryLifeFunctionSupported();
    if (!support) {
        cout << "BatteryLife predict not supported on such device!" << endl;
        return -1;
    }

    cout << "BatteryLife predict supported." << endl;

    value = service->getBatteryLifeValue();
    if (value == -1) {
        cout << "can not read batterylife sysfs node!" << endl;
        return -1;
    }

    chargefull = service->getBatteryChargeFullValue();
    if (chargefull == -1) {
        cout << "can not read chargefull sysfs node!" << endl;
        return -1;
    }

    cout << "read batterylife sysfs node successfully!" << endl;
    cout << "batterylife value is :" << value << " !" << endl;
    cout << "chargefull value is :" << chargefull << " !" << endl;

    return 0;
}

