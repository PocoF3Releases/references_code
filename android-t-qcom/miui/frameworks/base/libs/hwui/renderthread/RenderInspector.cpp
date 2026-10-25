#undef LOG_TAG
#define LOG_TAG "RenderInspector"

#include "RenderInspector.h"

#include <binder/BinderService.h>
#include <binder/Parcel.h>

#include <stdio.h>
#include <fcntl.h>
#include <linux/ioctl.h>

#define INPUT_INTERVAL 50 * 1000000
#define INDEX_FPS 0
#define INDEX_TERRIBLE_THRESHOLD 1
#define INDEX_BAD_THRESHOLD 2
#define FRAME_THRESHOLDS_SIZE 3

#define SMALLVIEW_HEIGHT_THRESHOLD 150
#define SMALLVIEW_WIDTH_THRESHOLD 100
#define FRAME_UPDATE_THRESHOLD 10

namespace android {
namespace uirenderer {

#ifdef RENDER_INSPECTOR_LEGACY
using namespace FrameInfoIndex;
#endif

struct Interval {
#ifdef RENDER_INSPECTOR_LEGACY
    FrameInfoIndexEnum start;
    FrameInfoIndexEnum end;
#else
    FrameInfoIndex start;
    FrameInfoIndex end;
#endif
    float weight;
};

static const float frameThresholds[][FRAME_THRESHOLDS_SIZE] = {
        {60.0f, 2.0f, 1.0f},
        {90.0f, 3.0f, 1.5f},
        {120.0f, 4.0f, 2.0f}
};

static const Interval INTERVALS[] = {
        {FrameInfoIndex::IntendedVsync, FrameInfoIndex::Vsync, 1},                     //main thread delay time
        {FrameInfoIndex::HandleInputStart, FrameInfoIndex::AnimationStart, 1},         //handle_input_time interval
        {FrameInfoIndex::AnimationStart, FrameInfoIndex::PerformTraversalsStart, 1},   //handle_animation_time
        {FrameInfoIndex::PerformTraversalsStart, FrameInfoIndex::DrawStart, 1},        //handle_traversal_time
        {FrameInfoIndex::SyncStart, FrameInfoIndex::IssueDrawCommandsStart, 1.5},      //bitmap_uploads_time
        {FrameInfoIndex::IssueDrawCommandsStart, FrameInfoIndex::SwapBuffers, 0.75},   //issue_draw_commands_time
        {FrameInfoIndex::SwapBuffers, FrameInfoIndex::SwapBuffersCompleted, 0.75},           //swap_buffer_time
};

enum {
    REPORT_PERCEPTIBLE_JANK_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION
};

enum {
    CHECK_BOOST_PERMISSION_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION,
    IS_CORE_APP_TRANSACTION = IBinder::FIRST_CALL_TRANSACTION + 2
};

static const bool DEBUG_ENABLE = property_get_bool("persist.fpstool.debug.enable", false);
static const bool TURBO_SCHED_ENABLE = property_get_bool("persist.sys.turbosched.enable.coreApp.optimizer", false);
static const char* METIS_DEV_PATH = "/dev/metis";

RenderInspector::RenderInspector(nsecs_t frameIntervalNanos, int fps)
            : mPerfShielder(NULL)
            , mTurboManager(NULL)
            , mQueueBufferChecker(frameIntervalNanos, "QueueBuffer")
            , mDequeueBufferChecker(frameIntervalNanos, "DequeueBuffer")
            , mFrameInterval(frameIntervalNanos)
            , mDebugEnabled(false)
            , mInitial(true)
            , mFps(fps) {
    const sp<IServiceManager> sm(defaultServiceManager());
    if (sm != NULL) {
        // Get PerfShielder binder.
        mPerfShielder = sm->getService(String16(PERFSHIELDER_SERVICE_NAME));
    } else
        ALOGE("Get default service manager failed.");

    initSelfCauseThresholds();
    initFrameThresholds();

    char property[PROPERTY_VALUE_MAX];
    if (property_get(PROPERTY_DEBUG_RENDER_INSPECTOR, property, NULL) > 0) {
        if (property[0] == '1') {
            ALOGI("RenderInspector debugging enabled.");
            mDebugEnabled = true;
        }
    }

    mIgnoreFirstFrame = property_get_bool("persist.sys.ignore.first.frame", true);
    mIgnoreFrameCount = property_get_int32("persist.sys.ignore.first.frame.count", 3);
}

RenderInspector::~RenderInspector(){}

void RenderInspector::addFrame(const FrameInfo& frame, const std::string& name) {
    mQueueBufferChecker.checkBufferDuration(frame[FrameInfoIndex::QueueBufferDuration], name);
    mDequeueBufferChecker.checkBufferDuration(frame[FrameInfoIndex::DequeueBufferDuration], name);
    if (mTrackerBuffer.size() >= FRAME_THRESHOLD) {
        if (CC_UNLIKELY((frame[FrameInfoIndex::IntendedVsync]
                - mTrackerBuffer[mTrackerBuffer.size() - 1][FrameInfoIndex::SwapBuffersCompleted])
                > INPUT_INTERVAL)) {
            if (mDebugEnabled)
                ALOGI("requestAnalysis in advance.");
            requestAnalysis(0, mTrackerBuffer.size());
            mTrackerBuffer.next() = frame;
            mWindowIndexs.next() = indexOfWindow(name);
            return;
        }
    }

    mTrackerBuffer.next() = frame;
    mWindowIndexs.next() = indexOfWindow(name);

    if (CC_UNLIKELY(mTrackerBuffer.size() == mTrackerBuffer.capacity())) {
        if (mDebugEnabled)
            ALOGI("requestAnalysis with buffer full.");
        requestAnalysis(0, mTrackerBuffer.size());
    }
}

void RenderInspector::ignoreNextFrame() {
    mIgnoreFrameCount ++;
}

static inline bool isSmallWindow(int32_t height, int32_t width) {
    if (height < SMALLVIEW_HEIGHT_THRESHOLD ||
            width < SMALLVIEW_WIDTH_THRESHOLD) {
        return true;
    }
    return false;
}

int RenderInspector::checkFrameDropped(FrameInfo& frame, int64_t vsyncPeriodNanos,
        bool hardwareLayerUpdated, int32_t height, int32_t width) {
    if (isSmallWindow(height, width)) {
        return 0;
    }

    int dropFrameCount = 0;
    nsecs_t totalDuration =
            frame[FrameInfoIndex::SwapBuffersCompleted] - frame[FrameInfoIndex::IntendedVsync];
    if (CC_UNLIKELY(totalDuration <= 0)) {
        ALOGE("CheckFrameDropped failed, wrong FrameInfo.");
        return 0;
    }

    if (totalDuration > vsyncPeriodNanos) {
        nsecs_t drawDuration = 0;
        if (frame[FrameInfoIndex::IntendedVsync] < mLastFrameEndTime) {
            drawDuration = frame[FrameInfoIndex::SwapBuffersCompleted] - mLastFrameEndTime;
        } else {
            drawDuration = totalDuration;
            mDropFrameDuration = 0;
        }

        dropFrameCount = (mDropFrameDuration + drawDuration) / vsyncPeriodNanos;
        mDropFrameDuration =
                (mDropFrameDuration + drawDuration) %vsyncPeriodNanos - vsyncPeriodNanos;
    } else {
        mDropFrameDuration = 0;
    }

    if (mIgnoreFirstFrame) {
        if (mIgnoreFrameCount > 0) {
            mIgnoreFrameCount --;
            dropFrameCount = - dropFrameCount;
        } else if (hardwareLayerUpdated) {
            if (frame[FrameInfoIndex::IntendedVsync] - mLastUpdateTime >
                    FRAME_UPDATE_THRESHOLD * vsyncPeriodNanos) {
                dropFrameCount = - dropFrameCount;
            }
            mLastUpdateTime = frame[FrameInfoIndex::SwapBuffersCompleted];
        }
    }

    mLastFrameEndTime = frame[FrameInfoIndex::SwapBuffersCompleted];
    if (DEBUG_ENABLE) {
        ATRACE_INT("dropFrameCount", dropFrameCount);
    }
    return dropFrameCount;
}

ssize_t RenderInspector::indexOfWindow(const std::string& name) {
    if (CC_UNLIKELY(mWindowNames.size() == 0)) {
        if (mDebugEnabled)
            ALOGI("A new Window added: %s", name.c_str());
        mCurrentWindowIndex = mWindowNames.add(name);
    }

    if (CC_UNLIKELY(mWindowNames.itemAt(mCurrentWindowIndex) != name)) {
        ssize_t index = mWindowNames.indexOf(name);
        if (index < 0) {
            if (mDebugEnabled)
                ALOGI("A new Window added: %s", name.c_str());
            mCurrentWindowIndex = mWindowNames.add(std::string(name));
        } else
            mCurrentWindowIndex = index;
    }

    return mCurrentWindowIndex;
}

void RenderInspector::clearBuffer() {
    mTrackerBuffer.clear();
    mWindowIndexs.clear();
}

void RenderInspector::requestAnalysis(size_t start, size_t end) {
    ATRACE_CALL();
    if (CC_UNLIKELY(start > end || end > mTrackerBuffer.size())) {
        return;
    }

    // Indicates when the final user operation begins.
    int64_t final_scenario_start_ts = -1;
    // Ingore operations have been performed for quite a while.
    int64_t aging_threshold = 3 * FRAME_BUFFER_SIZE * mFrameInterval;

    for(size_t i = end - 1; i > start;) {
        i--;
        if (CC_UNLIKELY((mTrackerBuffer[i + 1][FrameInfoIndex::IntendedVsync]
                - mTrackerBuffer[i][FrameInfoIndex::SwapBuffersCompleted]) > INPUT_INTERVAL)) {
            final_scenario_start_ts = mTrackerBuffer[i + 1][FrameInfoIndex::IntendedVsync];
            break;
        }
    }

    if (final_scenario_start_ts < 0) {
        // There is only one operation, in effect, at least mTrackerBuffer.size() frames.
        scanIsolatedScenario(start, end);
    } else {
        size_t last_index = start;
        for(size_t i = start + 1; i < end; i++) {
            if (CC_UNLIKELY((mTrackerBuffer[i][FrameInfoIndex::IntendedVsync]
                    - mTrackerBuffer[i - 1][FrameInfoIndex::SwapBuffersCompleted]) > INPUT_INTERVAL)) {
                if (final_scenario_start_ts - mTrackerBuffer[i - 1][FrameInfoIndex::SwapBuffersCompleted] < aging_threshold)
                    scanIsolatedScenario(last_index, i);
                last_index = i;
            }
        }
        scanIsolatedScenario(last_index, end);
    }

    if (CC_UNLIKELY(mInitial)) {
        mInitial = false;
    }

    clearBuffer();
}

void RenderInspector::initSelfCauseThresholds() {
    mThresholds[SlowMainThread] = static_cast<int64_t>(mFrameInterval);
    /*
     * The thresholds comes from these defined in JankTracker.
     * kSlowUI (0.5 * mFrameInterval) consists of SlowHandleInput, SlowHandleAnimation, SlowHandleTraversal,
     * Let's assume that each one of them takes 0.15 * mFrameInterval.
     */
    mThresholds[SlowHandleInput] = static_cast<int64_t>(.15 * mFrameInterval);
    mThresholds[SlowHandleAnimation] = static_cast<int64_t>(.15 * mFrameInterval);
    mThresholds[SlowHandleTraversal] = static_cast<int64_t>(.15 * mFrameInterval);
    mThresholds[SlowSync] = static_cast<int64_t>(.2 * mFrameInterval);
    // kSlowRT (0.75 * mFrameInterval) is divided into SlowRT (0.55 * mFrameInterval) and SlowSwapBuffers (0.2 * mFrameInterval)
    mThresholds[SlowRT] = static_cast<int64_t>(.55 * mFrameInterval);
    mThresholds[SlowSwapBuffers] = static_cast<int64_t>(.2 * mFrameInterval);
}

//初始化时即确定判断严重卡顿、一般卡顿的阈值
void RenderInspector::initFrameThresholds() {
    size_t length  = sizeof(frameThresholds) / sizeof(frameThresholds[0]);

    for(size_t i = 0; i < length; i++) {
        if(mFps <= frameThresholds[i][INDEX_FPS]) {
            mTerribleRefreshFrameFactor = frameThresholds[i][INDEX_TERRIBLE_THRESHOLD];
            mBadRefreshFrameFactor = frameThresholds[i][INDEX_BAD_THRESHOLD];
            return;
        }
    }

    mTerribleRefreshFrameFactor = frameThresholds[length - 1][INDEX_TERRIBLE_THRESHOLD];
    mBadRefreshFrameFactor = frameThresholds[length - 1][INDEX_BAD_THRESHOLD];
}

// Try to find the jank cause by application itself
int32_t RenderInspector::evaluateSelfCause(size_t start, size_t end,
        int64_t maxFrameDuration __attribute__ ((unused))) {
    ATRACE_CALL();
    /*  How to evaluate cause:
     * 1. In every terrible frame :
     *    cause_weight = (interval_time - interval_threshold_time) * interval_weight
     * 2. Sum cause_weight for all terrible frames.
     * 3. return the cause which counts most.
     */

    int64_t cause_weights[NUM_SELF_CAUSES] = {0};

    for(size_t index = start; index <= end; index++) {
        int64_t realIntendedVsync = mTrackerBuffer[index][FrameInfoIndex::IntendedVsync];
        if (index != start && (mTrackerBuffer[index][FrameInfoIndex::IntendedVsync] < mTrackerBuffer[index - 1][FrameInfoIndex::IssueDrawCommandsStart])) {
            realIntendedVsync = mTrackerBuffer[index - 1][FrameInfoIndex::IssueDrawCommandsStart];
        }
        int64_t duration =
                mTrackerBuffer[index][FrameInfoIndex::SwapBuffersCompleted] - realIntendedVsync;

        float factor =  static_cast<float>(duration) / mFrameInterval;
        // We only care about the terrible frame
        if (CC_LIKELY(factor > 2.0f)) {
            for (int i = 0; i < NUM_SELF_CAUSES; i++) {
                int64_t interval = mTrackerBuffer[index].duration(INTERVALS[i].start, INTERVALS[i].end);
                if (i == SlowMainThread) {
                    interval = mTrackerBuffer[index][FrameInfoIndex::Vsync] - realIntendedVsync;
                }
                int64_t cause_weight = 0;
                if (interval > mThresholds[i])
                    cause_weight = static_cast<int64_t>((interval - mThresholds[i]) * INTERVALS[i].weight);
                cause_weights[i] += cause_weight;
            }
        }
    }

    int cause_index = 0;
    for (int i = 0; i < NUM_SELF_CAUSES; i++) {
        if (cause_weights[i] > cause_weights[cause_index])
            cause_index = i;
    }

    // Unknown self cause
    if (cause_weights[cause_index] == 0)
      cause_index = -1;

    return cause_index;
}

void RenderInspector::raiseAlarm(size_t start, size_t end, int64_t maxFrameDuration, int64_t weight) {
    if (CC_UNLIKELY(mInitial)) {
        if (mDebugEnabled)
            ALOGI("Window just created, ignore the alram.");
        else return;
    }

    if (CC_LIKELY(weight < 0 ||
                  (end - start <= 2 * FRAME_THRESHOLD && weight < 2 * FRAME_THRESHOLD * TERRIBLE_FRAME_WEIGHT) ||
                  maxFrameDuration < FRAME_THRESHOLD * mFrameInterval)) {
        // trival jank, ingore it.
        return;
    }

    // CallingPid is actually MainThread tid.
    int32_t callingPid = getpid();
    // We are running in RenderThread here.
    int32_t renderThreadTid = gettid();

    int64_t totalDuration = mTrackerBuffer[end][FrameInfoIndex::SwapBuffersCompleted] - mTrackerBuffer[start][FrameInfoIndex::IntendedVsync];
    int64_t endTs = mTrackerBuffer[end][FrameInfoIndex::SwapBuffersCompleted];

    uint32_t selfCause = evaluateSelfCause(start, end, maxFrameDuration);

    ssize_t windowIndex = mWindowIndexs[start];

    size_t num_frames = end - start + 1;
    if (windowIndex >= 0 && windowIndex < (ssize_t)mWindowNames.size()) {
        // Report to system service by reportPerceptibleJank transaction.
        if (mPerfShielder != NULL) {
            Parcel data, reply;
            data.writeInterfaceToken(String16("com.android.internal.app.IPerfShielder"));
            // MainThread tid.
            data.writeInt32(callingPid);
            // RenderThread tid.
            data.writeInt32(renderThreadTid);
            // Window attached to this jank.
            data.writeString16(String16(mWindowNames.itemAt(windowIndex).c_str()));
            // Total Duration of the janky secenario.
            data.writeInt64(totalDuration);
            // Maximum time a frame took within the sencenario.
            data.writeInt64(maxFrameDuration);
            // Final timestamp of this secenario.
            data.writeInt64(endTs);
            // Application cause may lead to the jank.
            data.writeInt32(selfCause);
            data.writeInt64(num_frames);
            mPerfShielder->transact(REPORT_PERCEPTIBLE_JANK_TRANSACTION, data, &reply, IBinder::FLAG_ONEWAY);
        }
    }
}

void RenderInspector::scanIsolatedScenario(size_t start, size_t end) {
    if (CC_UNLIKELY(start > end || end > mTrackerBuffer.size())) {
        return;
    }

    int64_t startIndex = -1;
    int64_t endIndex = -1;
    int64_t weight = 0;
    int64_t maxDuration = 0;

    for(size_t i = start; i < end; i++) {
        int64_t duration = 0;
        if (i > start && (mTrackerBuffer[i][FrameInfoIndex::IntendedVsync] < mTrackerBuffer[i - 1][FrameInfoIndex::IssueDrawCommandsStart])) {
            duration = mTrackerBuffer[i][FrameInfoIndex::SwapBuffersCompleted] - mTrackerBuffer[i - 1][FrameInfoIndex::IssueDrawCommandsStart];
        } else {
            duration = mTrackerBuffer[i][FrameInfoIndex::SwapBuffersCompleted] - mTrackerBuffer[i][FrameInfoIndex::IntendedVsync];
        }

        // A terrible frame makes an inevitable jank, and it's much
        // possible that there are more janky frames subsequently.
        float factor =  static_cast<float>(duration) / mFrameInterval;
        if (CC_UNLIKELY(factor > mTerribleRefreshFrameFactor)) {
            if (startIndex == -1) {
                endIndex = startIndex = i;
                maxDuration = duration;
                // weight continuously at point 2 * mFrameInterval
                weight += (int64_t)((factor - mTerribleRefreshFrameFactor) * TERRIBLE_FRAME_WEIGHT
                        + BAD_FRAME_WEIGHT);
            } else {
                if ((i - endIndex) >= FRAME_THRESHOLD) {
                    // make up extra weights
                    weight -= FRAME_THRESHOLD * OK_FRAME_WEIGHT;

                    raiseAlarm(startIndex, endIndex, maxDuration, weight);
                    endIndex = startIndex = i;
                    maxDuration = duration;
                    weight = (int64_t)(factor * TERRIBLE_FRAME_WEIGHT);
                } else {
                    endIndex = i;
                    maxDuration = maxDuration > duration ? maxDuration : duration;
                    weight += (int64_t)(factor * TERRIBLE_FRAME_WEIGHT);
                }
            }
        // A bad frame is not likely be responsible for a perceptible jank,
        // but makes itself a part of it if the lastest frames already went wrong.
        } else if (CC_UNLIKELY(factor > mBadRefreshFrameFactor)) {
            if (startIndex >= 0) {
                if ((i - endIndex) > FRAME_THRESHOLD) {
                    // make up extra weights
                    weight -= FRAME_THRESHOLD * OK_FRAME_WEIGHT;

                    raiseAlarm(startIndex, endIndex, maxDuration, weight);
                    endIndex = startIndex = -1;
                    maxDuration = 0;
                    weight = 0;
                } else {
                    endIndex = i;
                    // Longer a bad frame takes, less it weights.
                    weight += (int64_t)(mTerribleRefreshFrameFactor / factor * BAD_FRAME_WEIGHT);
                }
            }
        } else {
            if (startIndex >= 0) {
                if (i - endIndex > FRAME_THRESHOLD) {
                    // make up extra weights
                    weight -= FRAME_THRESHOLD * OK_FRAME_WEIGHT;

                    raiseAlarm(startIndex, endIndex, maxDuration, weight);
                    endIndex = startIndex = -1;
                    maxDuration = 0;
                    weight = 0;
                } else {
                    weight += OK_FRAME_WEIGHT;
                }
            }
        }
    }

    if (CC_UNLIKELY(startIndex >= 0)) {
        // make up extra weights
        weight -= (int64_t)(end - 1 - endIndex) * OK_FRAME_WEIGHT;

        raiseAlarm(startIndex, endIndex, maxDuration, weight);
        endIndex = startIndex = 0;
        maxDuration = 0;
        weight = 0;
    }
}

void RenderInspector::getRenderToken() {
    if(!TURBO_SCHED_ENABLE) {
        return;
    }
    if (!mPermissionChecked) {
        checkBoostPermission();
    }

    if (mBoostEnabled) {
        int handle = open(METIS_DEV_PATH, O_RDWR);
        if (handle >= 0) {
            ioctl(handle, MetisCmd::REN_GET_VTASK_TOKEN, 0);
            close(handle);
        }
    }
}

void RenderInspector::releaseRenderToken() {
    if(!TURBO_SCHED_ENABLE) {
        return;
    }
    if (!mPermissionChecked) {
        checkBoostPermission();
    }

    if (mBoostEnabled) {
        int handle = open(METIS_DEV_PATH, O_RDWR);
        if (handle >= 0) {
            ioctl(handle, MetisCmd::REN_RELEASE_VTASK_TOKEN, 0);
            close(handle);
        }
    }
}

void RenderInspector::checkBoostPermission(){
    int handle = open(METIS_DEV_PATH, O_RDWR);
    if (handle < 0) {
        mBoostEnabled = false;
        mPermissionChecked = true;
        return;
    }

    close(handle);

    if(!isCoreApp()) {
        mBoostEnabled = false;
        mPermissionChecked = true;
        return;
    }

    if (NULL == mTurboManager) {
        const sp<IServiceManager> sm(defaultServiceManager());
        if (sm != NULL)
            mTurboManager = sm->getService(String16(TURBOSCHEDMANAGER_NAME));
    }

    if (NULL != mTurboManager) {
        Parcel data, reply;
        int ret;
        data.writeInterfaceToken(String16("miui.turbosched.ITurboSchedManager"));
        mTurboManager->transact(CHECK_BOOST_PERMISSION_TRANSACTION, data, &reply);
        reply.readExceptionCode();
        ret = reply.readInt32();
        if (ret == -1) {
            mBoostEnabled = false;
            mPermissionChecked = false;
        } else if(ret == 0) {
            mBoostEnabled = false;
            mPermissionChecked = true;
        } else if (ret == 1) {
            mBoostEnabled = true;
            mPermissionChecked = true;
        }
    } else {
        mBoostEnabled = false;
        mPermissionChecked = false;
    }
}

bool RenderInspector::isCoreApp() {
    if (NULL == mTurboManager) {
        const sp<IServiceManager> sm(defaultServiceManager());
        if (sm != NULL)
            mTurboManager = sm->getService(String16(TURBOSCHEDMANAGER_NAME));
    }

    if (NULL != mTurboManager) {
        Parcel data, reply;
        data.writeInterfaceToken(String16("miui.turbosched.ITurboSchedManager"));
        mTurboManager->transact(IS_CORE_APP_TRANSACTION, data, &reply);
        reply.readExceptionCode();
        return reply.readBool();
    }

    return false;
}

static const int MAX_BUFFER_COUNT = 60;
static const int MAX_NORMAL_BUFFER_COUNT = 3;

BufferDurationChecker::BufferDurationChecker(nsecs_t threshold, std::string name)
        : mThreshold(threshold),
          mName(name) {
    reset();
}

BufferDurationChecker::~BufferDurationChecker() {}

void BufferDurationChecker::checkBufferDuration(nsecs_t duration, const std::string& surfaceName) {
    if (duration > mThreshold) {
        if (mTotalBufferCount < MAX_BUFFER_COUNT) {
            mNormalBufferCount = 0;
            mNormalDuration = 0;
        } else {
            reportDelayedBuffers(surfaceName);
        }
        mTotalDuration += duration;
        mTotalBufferCount++;
        mMaxDuration = mMaxDuration > duration ? mMaxDuration : duration;
    } else if (mTotalDuration > 0) {
        if (mNormalBufferCount < MAX_NORMAL_BUFFER_COUNT) {
            mTotalDuration += duration;
            mTotalBufferCount++;
            mNormalBufferCount++;
            mNormalDuration += duration;
        } else {
            reportDelayedBuffers(surfaceName);
        }
    }
}

void BufferDurationChecker::reportDelayedBuffers(const std::string& surfaceName) {
    mTotalBufferCount -= mNormalBufferCount;
    ALOGW("%s time out on %s, count=%d, avg=%ld ms, max=%ld ms.",
            mName.c_str(), surfaceName.c_str(), mTotalBufferCount,
            long(ns2ms((mTotalDuration - mNormalDuration) / mTotalBufferCount)),
            long(ns2ms(mMaxDuration))
        );
    reset();
}

void BufferDurationChecker::reset() {
    mMaxDuration = 0;
    mTotalDuration = 0;
    mNormalDuration = 0;
    mTotalBufferCount = 0;
    mNormalBufferCount = 0;
}

}
}
