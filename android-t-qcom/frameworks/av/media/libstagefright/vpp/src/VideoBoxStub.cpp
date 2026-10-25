//#define LOG_NDEBUG 0
#define LOG_TAG "VideoBoxStub"
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

#include "VideoBoxStub.h"
namespace android{
namespace videobox{

void* VideoBoxStub::mLibVideoBox = NULL;
std::mutex VideoBoxStub::VideoBoxStubLock;
static int32_t mGenerationVideoBox = 0;

IVideoBoxStub* VideoBoxStub::GetImplInstance() {
    std::lock_guard<std::mutex> lock(VideoBoxStubLock);
    if (!mLibVideoBox) {
        mVideoBoxHandle = NULL;
        mLibVideoBox = ::dlopen(VIDEOBOXLIB_NAME, RTLD_LAZY);
    }
    if (mLibVideoBox && !mVideoBoxHandle) {
        Create* create = (Create *)dlsym(mLibVideoBox, "create");
        mVideoBoxHandle = create();
        mGenerationVideoBox ++;
    }
    return mVideoBoxHandle;
}

void VideoBoxStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(VideoBoxStubLock);
    if (mLibVideoBox) {
        Destroy* destroy = (Destroy *)dlsym(mLibVideoBox, "destroy");
        if (mVideoBoxHandle) {
            destroy(mVideoBoxHandle);
            mVideoBoxHandle = NULL;
        }
    }
    if(mGenerationVideoBox <= 0 && mLibVideoBox) {
        dlclose(mLibVideoBox);
        mLibVideoBox = NULL;
        mGenerationVideoBox = 0;
        mVideoBoxHandle = NULL;
    }
}

VideoBoxStub::VideoBoxStub() {
    ALOGI("VideoBoxStub::VideoBoxStub");
    mVideoBoxHandle = NULL;
}

bool VideoBoxStub::initVideobox(const AString &name,bool isvideo,sp<CodecBase> codec)
{
    ALOGI("VideoBoxStub::initVideobox");
    bool ret = false;
    ALOGI("VideoBoxStub::initVideobox mGenerationVideoBox= %d",mGenerationVideoBox);
    IVideoBoxStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->initVideobox(name,isvideo,codec);
    if (ret)
        return true;
    else
        return false;
}

bool VideoBoxStub::configVideobox(const sp<AMessage> format)
{
    ALOGI("VideoBoxStub::configVideobox");
    bool ret = false;
    IVideoBoxStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->configVideobox(format);
    return ret;
}

bool VideoBoxStub::readVideoBox(int32_t portIndex,const sp<MediaCodecBuffer> &buffer)
{
    ALOGV("VideoBoxStub::readVideoBox");
    bool ret = false;
    IVideoBoxStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->readVideoBox(portIndex,buffer);
    return ret;
}

bool VideoBoxStub::getRealtimeFramerate(const sp<AMessage> &msg)
{
    ALOGV("VideoBoxStub::getRealtimeFramerate");
    bool ret = false;
    IVideoBoxStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->getRealtimeFramerate(msg);
    return ret;
}

void VideoBoxStub::releaseVideoBox()
{
    ALOGI("VideoBoxStub::releaseVideoBox");
    mGenerationVideoBox --;
    DestroyImplInstance();
}

bool VideoBoxStub::isFrcEnable()
{
    ALOGV("VideoBoxStub::isFrcEnable");
    bool ret = false;
    IVideoBoxStub* Istub = GetImplInstance();
    if(Istub)
        ret = Istub->isFrcEnable();
    return ret;
}

}
}
