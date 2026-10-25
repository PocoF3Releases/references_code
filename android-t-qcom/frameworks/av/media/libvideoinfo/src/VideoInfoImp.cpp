#define LOG_TAG "VideoInfo"
#define LOG_NDEBUG 0
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <media/stagefright/VideoInfoImp.h>
#include <media/stagefright/foundation/ALooper.h>

static const int MAX_FILE_PATH_LENGTH = 256;
static const char *VIDEO_DEBUG_PERF_CTRL = "debug.media.video.perf";
static const char *VIDEO_DEBUG_DUMP_CTRL = "debug.media.video.dump";
static const char *DEFAULT_VIDEO_DUMP_DIR = "/data/local/traces/video_";

namespace android {

VideoInfoImp::VideoInfoImp(String16 &clientName, AString &codecName)
    : mClientName(clientName),
      mCodecName(codecName),
      mLastBufferQueueTime(0),
      mLastBufferDequeueTime(0),
      mLastBufferRenderTime(0),
      mMaxQueueInputInterver(0),
      mInputBufferNum(0),
      mMaxDequeueOutputInterver(0),
      mOutputBufferNum(0),
      mMaxRenderOutputInterver(0),
      mRenderBufferNum(0),
      mVideoDebugPerf(false),
      mVideoDebugDataDump(false) {
    char value[PROPERTY_VALUE_MAX];

    if ((property_get(VIDEO_DEBUG_PERF_CTRL, value, "false")) && (!strcmp("1", value) || !strcmp("true", value))) {
        ALOGD("video debug perf enabled");
        mVideoDebugPerf = true;
    }

    if ((property_get(VIDEO_DEBUG_DUMP_CTRL, value, "false")) && (!strcmp("1", value) || !strcmp("true", value))) {
        ALOGD("video debug dump enabled");
        mVideoDebugDataDump = true;
    }

    ALOGD("%s create %s in MediaCodec", String8(mClientName).string() , mCodecName.c_str());
}

VideoInfoImp::~VideoInfoImp()
{
    ALOGD("%s destroy %s in MediaCodec, input num %" PRIu64 " output num %" PRIu64 " render num %" PRIu64
          "\nmax input intervel %" PRIu64 "ms max output intervel %" PRIu64 "ms max render interval %" PRIu64 "ms",
                String8(mClientName).string(), mCodecName.c_str(), mInputBufferNum, mOutputBufferNum, mRenderBufferNum,
                mMaxQueueInputInterver / 1000, mMaxDequeueOutputInterver / 1000, mMaxRenderOutputInterver / 1000);
}

void VideoInfoImp::dumpDataToFile(char *filePath, void *buffer, uint64_t size)
{
    umask(0000);
    FILE* fp = fopen(filePath, "ab+");
    if (!fp) {
        ALOGE("%s open failed , %s", filePath, strerror(errno));
        return;
    }

    fwrite (buffer , sizeof(char), size, fp);
    fclose(fp);
}

void VideoInfoImp::dumpVideoData(bool input,
                                  DataType type,
                                  void *buffer,
                                  uint64_t size,
                                  uint32_t width,
                                  uint32_t height)
{
    if (!mVideoDebugDataDump) {
        return;
    }

    char filePath[MAX_FILE_PATH_LENGTH];
    snprintf(filePath, sizeof(filePath), "%s%s_%s_%s_%dx%d.",
                                         DEFAULT_VIDEO_DUMP_DIR,
                                         String8(mClientName).string(),
                                         mCodecName.c_str(),
                                         input ? "input" : "output",
                                         width,
                                         height);
    switch (type)
    {
        case DATA_TYPE_RAW:
            dumpDataToFile(strcat(filePath, "raw"), buffer, size);
            break;
        case DATA_TYPE_YUV:
            dumpDataToFile(strcat(filePath, "yuv"), buffer, size);
            break;
        default:
            ALOGE("unsuport data type %d", type);
            return;
    }

}

void VideoInfoImp::printInputBufferInterver(int64_t presentationTimeUs)
{
    mInputBufferNum++;

    if (mLastBufferQueueTime == 0) {
        mLastBufferQueueTime = ALooper::GetNowUs();
        return;
    }

    uint64_t queueBufferTime = ALooper::GetNowUs();
    uint64_t queueInputBufferInterver = queueBufferTime - mLastBufferQueueTime;
    mLastBufferQueueTime = queueBufferTime;
    mMaxQueueInputInterver = std::max(mMaxQueueInputInterver, queueInputBufferInterver);

    ALOGD_IF(mVideoDebugPerf, "%s queue input buffer PTS %" PRId64 "ms, interverl is %" PRIu64 "ms",
                String8(mClientName).string(), presentationTimeUs / 1000, queueInputBufferInterver / 1000);
}

void VideoInfoImp::printOutputBufferInterver(int64_t presentationTimeUs)
{
    mOutputBufferNum++;

    if (mLastBufferDequeueTime == 0) {
        mLastBufferDequeueTime = ALooper::GetNowUs();
        return;
    }

    uint64_t dequeueBufferTime = ALooper::GetNowUs();
    uint64_t dequeueOutputBufferInterver = dequeueBufferTime - mLastBufferDequeueTime;
    mLastBufferDequeueTime = dequeueBufferTime;
    mMaxDequeueOutputInterver = std::max(mMaxDequeueOutputInterver, dequeueOutputBufferInterver);

    ALOGD_IF(mVideoDebugPerf, "%s dequeue output buffer PTS %" PRId64 "ms, interverl is %" PRIu64 "ms",
                String8(mClientName).string(), presentationTimeUs / 1000, dequeueOutputBufferInterver / 1000);
}

void VideoInfoImp::printRenderBufferInterver(int64_t presentationTimeUs)
{
    mRenderBufferNum++;

    if (mLastBufferRenderTime == 0) {
        mLastBufferRenderTime = ALooper::GetNowUs();
        return;
    }

    uint64_t renderBufferTime = ALooper::GetNowUs();
    uint64_t renderOutputBufferInterver = renderBufferTime - mLastBufferRenderTime;
    mLastBufferRenderTime = renderBufferTime;
    mMaxRenderOutputInterver = std::max(mMaxRenderOutputInterver, renderOutputBufferInterver);

    ALOGD_IF(mVideoDebugPerf || renderOutputBufferInterver > 100000,
                "%s render buffer PTS %" PRId64 "ms, interverl is %" PRIu64 "ms",
                String8(mClientName).string(), presentationTimeUs / 1000, renderOutputBufferInterver / 1000);
}

} //namespace android