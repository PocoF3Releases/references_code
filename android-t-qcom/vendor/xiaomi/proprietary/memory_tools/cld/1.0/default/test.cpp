#include <utils/Log.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <iostream>
#include "Cld.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "mem_cld-test"

using namespace android;
using std::string;
using namespace::std;

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_vec;
using ::vendor::xiaomi::hardware::cld::V1_0::ICld;
using ::vendor::xiaomi::hardware::cld::V1_0::FragmentLevel;
using ::vendor::xiaomi::hardware::cld::V1_0::CldOperationStatus;
using ::vendor::xiaomi::hardware::cld::V1_0::ICldCallback;

const std::vector<std::string> frag_levels { "ANALYSIS", "CLEAN", "MEDIUM", "SEVERE", "UNKNOWN" };

class CldCallback : public ICldCallback {
    public:
        CldCallback() = default;
        CldCallback(int n) : num(n) { cout << "Constructing callback(" << num << ");\n"; }
        ~CldCallback() { }
        Return<void> notifyStatusChange(FragmentLevel level) {
            cout << "onNotify: fragment level after CLD: " << frag_levels[static_cast<int>(level)] << endl;
            return Void();
        }

    private:
        int num = 0;
};

int main(void)
{
    cout << "CLD test starting...\n";

    sp<ICld> service = ICld::getService();
    if (service == nullptr) {
        cerr << "Unable to initialize the HIDL!\n";
        return -1;
    }

    cout << "Initialize the CLD HIDL successfully.\n";

    // Check whether CLD is supported
    bool support = service->isCldSupported();
    if (!support) {
        cerr << "CLD is not supported on current device!\n";
        return -1;
    }

    cout << "CLD supported!\n";

    cout << "Set callback!\n";
    sp<CldCallback> callback0 = new CldCallback(0);
    service->registerCallback(callback0);

    sp<CldCallback> callback1 = new CldCallback(1);
    service->registerCallback(callback1);

    // Get current defragment level
    FragmentLevel level = service->getFragmentLevel();
    cout << "Fragment level before CLD: " << frag_levels[static_cast<int>(level)] << endl;

    // Trigger defragmentation
    service->triggerCld(1);
    cout << "Trigger CLD!\n";

    // Wait for defragment done
    cout << "Now we wait 5 seconds for uevent callback.\n";
    sleep(5);
    cout << "5 seconds passed. If you do not receive any messages, please check driver.\n";

    service->unregisterCallback(callback0);
    service->unregisterCallback(callback1);

    // Get new defragment level
    level = service->getFragmentLevel();
    cout << "Manually retrieve fragment level: " << frag_levels[static_cast<int>(level)] << endl;

    cout << "CLD test done!\n";
    return 0;
}
