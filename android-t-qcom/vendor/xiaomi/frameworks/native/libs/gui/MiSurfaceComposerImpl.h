#ifndef ANDROID_MI_SURFACE_IMPL_H
#define ANDROID_MI_SURFACE_IMPL_H

#include <log/log.h>
#include <utils/Errors.h>
#include <utils/String8.h>
#include <gui/MiSurfaceComposerStub.h>
#include <gui/IMiSurfaceComposerStub.h>

namespace android {

class MiSurfaceComposerImpl : public IMiSurfaceComposerStub {
public:
    virtual ~MiSurfaceComposerImpl() {}
    void setMiuiTransactionState(const Vector<MiuiDisplayState>& miuiDisplays,
             uint32_t flags, int64_t desiredPresentTime,const sp<IBinder>& remote,
             const String16& interfaceDescriptor,uint32_t code) override;
    status_t setFrameRateVideoToDisplay(uint32_t cmd, Parcel *data,
             const String16& interfaceDescriptor, const sp<IBinder>& remote, uint32_t code) override;
};

extern "C" IMiSurfaceComposerStub* createMiSurfaceComposerImpl();
extern "C" void destroyMiSurfaceComposerImpl(IMiSurfaceComposerStub* impl);

}//namespace android

#endif
