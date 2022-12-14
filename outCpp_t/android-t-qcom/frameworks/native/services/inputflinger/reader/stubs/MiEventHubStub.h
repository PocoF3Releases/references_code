#ifndef ANDROID_EVENTHUB_IMPL_H
#define ANDROID_EVENTHUB_IMPL_H
#include <mutex>
#include "IMiEventHubStub.h"
namespace android {

class IMiEventHubStub;
class MiEventHubStub {
private:
    static void *LibMiEventHubImpl;
    static IMiEventHubStub *ImplInstance;
    static std::mutex StubLock;
    static bool inited;
    static IMiEventHubStub *GetImplInstance();
    static void DestroyImplInstance();
    static constexpr const char* LIBPATH = "/system_ext/lib64/libmiinputflinger.so";

public:
    MiEventHubStub();
    virtual ~MiEventHubStub();
    static void init(EventHub* eventHub);
};
}//namespace android
#endif