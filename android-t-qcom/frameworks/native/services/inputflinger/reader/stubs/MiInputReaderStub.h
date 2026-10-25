#ifndef ANDROID_INPUTREADER_IMPL_H
#define ANDROID_INPUTREADER_IMPL_H
#include <mutex>
#include "IMiInputReaderStub.h"
namespace android {

class IMiInputReaderStub;
class MiInputReaderStub {
private:
    static void *LibMiInputReaderImpl;
    static IMiInputReaderStub *ImplInstance;
    static std::mutex StubLock;
    static bool inited;
    static IMiInputReaderStub *GetImplInstance();
    static void DestroyImplInstance();
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmiinputflinger.so";

public:
    MiInputReaderStub();
    virtual ~MiInputReaderStub();
    static void init(InputReader* inputReader);
    static bool getInputReaderAll();
    static void addDeviceLocked(std::shared_ptr<InputDevice> inputDevice);
    static void removeDeviceLocked(std::shared_ptr<InputDevice> inputDevice);
    static bool handleConfigurationChangedLockedIntercept(nsecs_t when);
    static void loopOnceChanges(uint32_t changes);
};
}//namespace android
#endif