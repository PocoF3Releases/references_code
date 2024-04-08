#include "MiFocusResolverStub.h"
#include <android/gui/FocusRequest.h>
#include <binder/Binder.h>
#include <dlfcn.h>
#include <log/log.h>
#include "IMiFocusResolverStub.h"

namespace android {

void* MiFocusResolverStub::sLibMiFocusResolverImpl = nullptr;
IMiFocusResolverStub* MiFocusResolverStub::sStubImplInstance = nullptr;
std::mutex MiFocusResolverStub::sLock;
bool MiFocusResolverStub::inited = false;

IMiFocusResolverStub* MiFocusResolverStub::getImplInstance() {
    std::lock_guard<std::mutex> lock(sLock);
    if (!sLibMiFocusResolverImpl && !inited) {
        sLibMiFocusResolverImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (sLibMiFocusResolverImpl) {
            // typedef function pointer
            typedef IMiFocusResolverStub* (*Create)();
            // get the createFocusResolverStub function pointer
            Create create = (Create)dlsym(sLibMiFocusResolverImpl, "createFocusResolverStubImpl");
            if (create) {
                // invoke
                sStubImplInstance = create();
            } else {
                ALOGE("dlsym is fail InputDispatcher.");
            }
        } else {
            ALOGE("inputflinger open %s failed! dlopen - %s", LIBPATH, dlerror());
        }
        inited = true;
    }
    return sStubImplInstance;
}

void MiFocusResolverStub::destroyImplInstance() {
    std::lock_guard<std::mutex> lock(sLock);
    if (!sLibMiFocusResolverImpl) {
        return;
    }
    // typedef function pointer
    typedef void (*Destroy)(IMiFocusResolverStub*);
    // get the destroyInputDispatcherStub function
    Destroy destroy = (Destroy)dlsym(sLibMiFocusResolverImpl, "destroyFocusResolverStubImpl");
    // destroy sStubImplInstance
    destroy(sStubImplInstance);
    dlclose(sLibMiFocusResolverImpl);
    sLibMiFocusResolverImpl = nullptr;
    sStubImplInstance = nullptr;
}

void MiFocusResolverStub::afterSetInputWindowsFocusChecked(
        const std::optional<gui::FocusRequest>& request, int32_t displayId, int result,
        int previousResult, const sp<IBinder>& currentFocus) {
    IMiFocusResolverStub* stub = getImplInstance();
    if (stub) {
        return stub->afterSetInputWindowsFocusChecked(request, displayId, result, previousResult,
                                                      currentFocus);
    }
}

void MiFocusResolverStub::afterSetFocusedWindowFocusChecked(const gui::FocusRequest& request,
                                                            int32_t displayId, int result,
                                                            int previousResult,
                                                            const sp<IBinder>& currentFocus) {
    IMiFocusResolverStub* stub = getImplInstance();
    if (stub) {
        return stub->afterSetFocusedWindowFocusChecked(request, displayId, result, previousResult,
                                                       currentFocus);
    }
}
} // namespace android