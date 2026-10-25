#ifndef MIMEDIACODEC_STUB_H
#define MIMEDIACODEC_STUB_H

#include <memory>
#include <vector>

#include <media/stagefright/foundation/AHandler.h>
#include <utils/Vector.h>
#include <media/stagefright/CodecBase.h>
#include <utils/String16.h>
#include <media/stagefright/foundation/AString.h>
#include <media/MediaCodecBuffer.h>

static const char * MEDIACODECLIB_NAME = "libmimediacodec.so";

namespace android{
namespace mimediacodec{

class IMiMediaCodecStub {

public:
    virtual ~IMiMediaCodecStub() {}
    virtual void ResetSendFpsToDisplay(bool needSendFps, uint32_t id);
    virtual void VideoToDisplayRealtimeFramerate(const sp<AMessage> &msg, uint32_t id);
    virtual void SetFpsFromPlayer(float fpsfromplayer);
    virtual void SendDoviPlaybackToDisplay(int status);
};

typedef IMiMediaCodecStub* Create();

typedef void Destroy(IMiMediaCodecStub *);

class MiMediaCodecStub {

public:
    MiMediaCodecStub();
    virtual ~MiMediaCodecStub(){}
    void DestroyImplInstance();
    void ResetSendFpsToDisplay(bool needSendFpsm, uint32_t id);
    void VideoToDisplayRealtimeFramerate(const sp<AMessage> &msg, uint32_t id);
    void SetFpsFromPlayer(float fpsfromplayer);
    void SendDoviPlaybackToDisplay(int status);

private:
    void *mLibMiMediaCodec;
    IMiMediaCodecStub *mMiMediaCodecHandle;
    static std::mutex MiMediaCodecStubLock;
    IMiMediaCodecStub *GetImplInstance();

};

}
}
#endif

