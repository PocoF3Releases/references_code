//#define LOG_NDEBUG 0
#define LOG_TAG "MiMediaCodecStub"
#include <utils/Log.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/ColorUtils.h>
#include <media/stagefright/foundation/AUtils.h>
#include "MiMediaCodecStub.h"

namespace android{
namespace mimediacodec{

std::mutex MiMediaCodecStub::MiMediaCodecStubLock;

MiMediaCodecStub::MiMediaCodecStub() {
    ALOGI("MiMediaCodecStub::MiMediaCodecStub");
    mMiMediaCodecHandle = NULL;
    mLibMiMediaCodec = NULL;
}

IMiMediaCodecStub* MiMediaCodecStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(MiMediaCodecStubLock);
    if (!mMiMediaCodecHandle) {
        mLibMiMediaCodec = ::dlopen(MEDIACODECLIB_NAME, RTLD_LAZY);
        if (mLibMiMediaCodec) {
            Create* create = (Create *)dlsym(mLibMiMediaCodec, "create");
            mMiMediaCodecHandle = create();
        }
    }
    return mMiMediaCodecHandle;
}

void MiMediaCodecStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(MiMediaCodecStubLock);
    if (mLibMiMediaCodec) {
        Destroy* destroy = (Destroy *)dlsym(mLibMiMediaCodec, "destroy");
        destroy(mMiMediaCodecHandle);
        mMiMediaCodecHandle = NULL;
        dlclose(mLibMiMediaCodec);
        mLibMiMediaCodec = NULL;
    }
}

void MiMediaCodecStub::ResetSendFpsToDisplay(bool needSendFps, uint32_t id) {
    IMiMediaCodecStub* Istub = GetImplInstance();
    if(Istub){
         Istub->ResetSendFpsToDisplay(needSendFps, id);
    }
}

void MiMediaCodecStub::VideoToDisplayRealtimeFramerate(const sp<AMessage> &msg, uint32_t id) {
    IMiMediaCodecStub* Istub = GetImplInstance();
    if(Istub) {
         Istub->VideoToDisplayRealtimeFramerate(msg, id);
    }
}

void MiMediaCodecStub::SetFpsFromPlayer(float fpsfromplayer) {
    IMiMediaCodecStub* Istub = GetImplInstance();
    if(Istub) {
         Istub->SetFpsFromPlayer(fpsfromplayer);
    }
}

void MiMediaCodecStub::SendDoviPlaybackToDisplay(int status) {
    IMiMediaCodecStub* Istub = GetImplInstance();
    if(Istub){
         Istub->SendDoviPlaybackToDisplay(status);
    }
}

}
}
