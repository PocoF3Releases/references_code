#ifndef VIDEO_BOX_VPP_STUB_H
#define VIDEO_BOX_VPP_STUB_H

#include <memory>
#include <vector>

#include <media/stagefright/foundation/AHandler.h>
#include <utils/Vector.h>
#include <media/stagefright/CodecBase.h>
#include <utils/String16.h>
#include <media/stagefright/foundation/AString.h>
#include <media/MediaCodecBuffer.h>

static const char * VIDEOBOXLIB_NAME = "libvppvideobox.so";

namespace android{
namespace videobox{

class IVideoBoxStub {
public:
	virtual ~IVideoBoxStub() {}
	virtual bool initVideobox(const AString &name,bool isvideo,sp<CodecBase> codec);
	virtual bool configVideobox(const sp<AMessage> format);
	virtual bool readVideoBox(int32_t portIndex,const sp<MediaCodecBuffer> &buffer);
	virtual bool getRealtimeFramerate(const sp<AMessage> &msg);
	virtual bool isFrcEnable() { return false; };
};

typedef IVideoBoxStub* Create();
typedef void Destroy(IVideoBoxStub *);

class VideoBoxStub {
private:
	static void *mLibVideoBox;
	IVideoBoxStub *mVideoBoxHandle;
	IVideoBoxStub *GetImplInstance();
	void DestroyImplInstance();
	static std::mutex VideoBoxStubLock;

public:
	VideoBoxStub();
	virtual ~VideoBoxStub() {}
	bool initVideobox(const AString &name,bool isvideo,sp<CodecBase> codec);
	bool configVideobox(const sp<AMessage> format);
	bool readVideoBox(int32_t portIndex,const sp<MediaCodecBuffer> &buffer);
	bool getRealtimeFramerate(const sp<AMessage> &msg);
	void releaseVideoBox();
	bool isFrcEnable();
};
}
}
#endif
