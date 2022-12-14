#ifndef RenderInspector_H_
#define RenderInspector_H_

#ifdef RENDER_INSPECTOR_LEGACY
#include <string>
#endif

#include <FrameInfo.h>
#include <utils/SortedVector.h>
#include <utils/RingBuffer.h>
#include <gui/TraceUtils.h>

#include <cutils/properties.h>
#include <binder/IInterface.h>
#include <IMiuiRenderInspectorStub.h>

namespace android {
namespace uirenderer {

#define FRAME_THRESHOLD  3

#define FRAME_BUFFER_SIZE 120

// Since a terrible frame can take far more than 2 vsync,
// TERRIBLE_FRAME_WEIGHT stands for weight per vsync.
#define TERRIBLE_FRAME_WEIGHT 3

#define BAD_FRAME_WEIGHT -2

#define OK_FRAME_WEIGHT -3

#define PROPERTY_DEBUG_RENDER_INSPECTOR "debug.miui.perf.inspector"
#define PERFSHIELDER_SERVICE_NAME "perfshielder"
#define TURBOSCHEDMANAGER_NAME "turbosched"

enum SelfCause {
    SlowMainThread,
    SlowHandleInput,
    SlowHandleAnimation,
    SlowHandleTraversal,

    SlowSync, //Bitmap Uploads,
    SlowRT,   //Issue Draw Commands,
    SlowSwapBuffers, //SwapBuffer

    // Must be the last value
    NUM_SELF_CAUSES,
};

namespace MetisCmd {
enum {
    UI_GET_VTASK_TOKEN,
    REN_GET_VTASK_TOKEN,
    MI_DYN_VTASK_TOKEN,
    UI_RELEASE_VTASK_TOKEN,
    REN_RELEASE_VTASK_TOKEN,
    MI_DYN_RELEASE_VTASK_TOKEN,
    FRAME_BOOST,
    TASK_BOOST_REQ,
    METIS_CMD_TYPES,
};
}

class BufferDurationChecker {
public:
    BufferDurationChecker(nsecs_t threshold, std::string name);
    ~BufferDurationChecker();
    void checkBufferDuration(nsecs_t duration, const std::string& name);

private:
    nsecs_t mThreshold;
    std::string mName;
    nsecs_t mMaxDuration;
    nsecs_t mTotalDuration;
    nsecs_t mNormalDuration;
    int mTotalBufferCount;
    int mNormalBufferCount;

    void reportDelayedBuffers(const std::string& name);
    void reset();
};

class RenderInspector : public IMiuiRenderInspectorStub {
private:
    void initSelfCauseThresholds();
    void initFrameThresholds();

    void checkBoostPermission();
    bool isCoreApp();

    sp<IBinder> mPerfShielder;
    sp<IBinder> mTurboManager;

    // Ring buffer large enough for FRAME_BUFFER_SIZE / 60 seconds worth of frames
    RingBuffer<FrameInfo, FRAME_BUFFER_SIZE> mTrackerBuffer;
    // Maintaining window index with FrameInfo structure intact
    RingBuffer<ssize_t, FRAME_BUFFER_SIZE> mWindowIndexs;
    SortedVector<std::string> mWindowNames;

    BufferDurationChecker mQueueBufferChecker;
    BufferDurationChecker mDequeueBufferChecker;

    int64_t mThresholds [NUM_SELF_CAUSES];
    int64_t mFrameInterval;
    bool mDebugEnabled;
    bool mInitial;
    //Application framerate
    int mFps;
    //严重卡顿的阈值因子
    float mTerribleRefreshFrameFactor;
    //一般卡顿的阈值因子
    float mBadRefreshFrameFactor;

    ssize_t mCurrentWindowIndex;

    nsecs_t mDropFrameDuration = 0;
    nsecs_t mLastFrameEndTime = 0;
    nsecs_t mLastUpdateTime = 0;
    bool mIgnoreFirstFrame;
    int mIgnoreFrameCount;

    bool mPermissionChecked = false;
    bool mBoostEnabled = false;

public:
    RenderInspector(nsecs_t frameIntervalNanos, int fps = 60);
    ~RenderInspector();

    void addFrame(const FrameInfo& frame, const std::string& name);
    ssize_t indexOfWindow(const std::string& name);
    void raiseAlarm(size_t start, size_t end, int64_t maxFrameDuration, int64_t weight);
    void requestAnalysis(size_t start, size_t end);
    void scanIsolatedScenario(size_t start, size_t end);
    void clearBuffer();

    int32_t evaluateSelfCause(size_t start, size_t end, int64_t maxFrameDuration);

    void ignoreNextFrame();
    int checkFrameDropped(FrameInfo& frame, int64_t vsyncPeriodNanos,
                          bool hardwareLayerUpdated, int height, int width);
    void getRenderToken();
    void releaseRenderToken();
}; /* namespace renderthread */
} /* namespace uirenderer */
} /* namespace android */

#endif /*RenderInspector_H_ */
