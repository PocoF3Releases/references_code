#undef LOG_TAG
#define LOG_TAG "SurfaceFlingerInspector"

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define LOG_TURBO_VERSION_NATIVE 3

#include "SurfaceFlingerInspector.h"

#include <android-base/stringprintf.h>
#include <cutils/properties.h>
#include <inttypes.h>
#include <log/log.h>
#include <string.h>
#include <json/json.h>
#include <time.h>
#include <regex>
#include "FrameTimelineInspector.h"

using android::base::StringAppendF;

static const nsecs_t DUMP_STATTE_THRESHOLD = s2ns(15);
static const nsecs_t NONE_OPERATION_THRESHOLD = 10;
static const int SMALLVIEW_HEIGHT_THRESHOLD = 150;
static const int SMALLVIEW_WIDTH_THRESHOLD = 100;
static const int FRAME_RECORD_MAX_CAPACITY = 30;
static const bool DEBUG_ENABLE = property_get_bool("persist.fpstool.debug.enable", false);
static list<string> mBoostAllowList = {"com.sina.weibo", "com.ss.android.article.news",
	"com.tencent.mm", "com.smile.gifmaker", "com.ss.android.ugc.aweme", "com.taobao.taobao"};

static int const thresholds[3][4] = {
    {2, 3, 2, 3},
    {2, 3, 2, 3},
    {3, 4, 3, 4}
};

namespace android {

int64_t SurfaceInspector::sVsyncPeriod = 16666666666;

SurfaceFlingerInspector::SurfaceFlingerInspector() {
    std::regex PREREGEX("\\d+(.\\d+){2,}(-internal)?");
    mLastDumpTime = systemTime(SYSTEM_TIME_MONOTONIC);
    char path[128] = {};
    snprintf(path, sizeof(path), "/dev/mi_exception_log");
    mExceptionLogFd = open(path, O_WRONLY|O_CLOEXEC|O_NONBLOCK);
    char value[PROPERTY_VALUE_MAX];
    property_get("ro.build.version.incremental", value, "pre");
    string mRomVersion = value;
    mLogTurboVersion = std::regex_match(mRomVersion, PREREGEX) ? 
           property_get_int32("persist.sys.perfdebug.monitor.perfversion", 3) : 
           property_get_int32("persist.sys.perfdebug.monitor.perfversion", 2);
    property_get("persist.sys.perfdebug.monitor.sfduration", value, "50");
    mLongFrameRefreshThreshold = atoi(value);
    mFrameTimelineInjector = FrameTimelineInjector::createFrameTimelineInjector();
}

SurfaceFlingerInspector::~SurfaceFlingerInspector() {
    close(mExceptionLogFd);
}

void SurfaceFlingerInspector::markStartMonitorFrame() {
    mLastStageEndTime =  systemTime(SYSTEM_TIME_MONOTONIC);
}

void SurfaceFlingerInspector::monitorFrameComposition(std::string where, std::string displayName) {
    nsecs_t now =  systemTime(SYSTEM_TIME_MONOTONIC);
    nsecs_t duration = ns2ms(now - mLastStageEndTime);
    mLastStageEndTime = now;

    if (duration > mLongFrameRefreshThreshold) {
        if (mLogTurboVersion == LOG_TURBO_VERSION_NATIVE) {
            if (mExceptionLogFd > 0) {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                Json::StreamWriterBuilder factory;
                Json::Value root;
                root["EventID"] = 902001050;
                root["EventTime"] = static_cast<uint64_t>(tv.tv_sec) * UINT64_C(1000) + tv.tv_usec / UINT64_C(1000);
                root["Latency"] = duration;
                root["SFDisplayName"] = displayName.c_str();
                root["SFDisplayWhere"] = where.c_str();
                std::string exception_log_str = Json::writeString(factory, root);
                int errid = write(mExceptionLogFd, exception_log_str.c_str(), exception_log_str.size());
                if (errid == -1) {
                    ALOGW("err write to mi_exception_log");
                }
                //close exception_log fd when ~Surfaceflingerinspector
            }
        } else {
            SLOGW("Slow frame composition: %s %s took %ld ms",
                    displayName.c_str(), where.c_str(), (long) duration);
        }
    }
}

void SurfaceFlingerInspector::markBufferLatched(String8 layerName) {
    if (mDumpState == Sleep) return;

    if (CC_UNLIKELY(systemTime(SYSTEM_TIME_MONOTONIC) - mLastDumpTime >
            DUMP_STATTE_THRESHOLD)) {
        mDumpState = Sleep;
        std::lock_guard lock(mLock);
        mSurfaceInspectorMap.clear();
        mTransactionInspectorMap.clear();
        return;
    }

    std::lock_guard lock(mLock);
    if (find(mLastTopLayerList.begin(), mLastTopLayerList.end(), layerName) !=
            mLastTopLayerList.end()) {
        mSurfaceInspectorMap[layerName].markBufferLatched();
    }
}

static inline bool isSmallWindow(Rect rect) {
    if (rect.getHeight() < SMALLVIEW_HEIGHT_THRESHOLD ||
            rect.getWidth() < SMALLVIEW_WIDTH_THRESHOLD) {
        return true;
    }
    return false;
}

void SurfaceFlingerInspector::monitorSurfaceIfNeeded(String8 layerName, Rect rect) {
    // Ignore layers that are too small or have no name like #0 #1
    if (!isSmallWindow(rect) && layerName.size() > 2) {
        std::lock_guard lock(mLock);
        mTopLayersList.push_back(layerName);
        if (mSurfaceInspectorMap.find(layerName) == mSurfaceInspectorMap.end()) {
            mSurfaceInspectorMap[layerName].init(layerName);
        }
    }
}

void SurfaceFlingerInspector::producerFrameDropped(int32_t droppedFrameCount,
        int64_t intendedVsyncTime, String8 windowName) {
    if (mDumpState == Sleep || windowName.size() == 0) return;

    std::lock_guard lock(mLock);
    map<String8, SurfaceInspector>::iterator iter;
    for (iter = mSurfaceInspectorMap.begin(); iter != mSurfaceInspectorMap.end(); iter ++) {
        if (iter->first.contains(windowName)) {
            iter->second.producerFrameDropped(droppedFrameCount, intendedVsyncTime);
        } else if (droppedFrameCount < 0) {
            iter->second.removeLastDropIfNeeded(intendedVsyncTime);
        }
    }
}

void SurfaceFlingerInspector::checkFrameDropped(nsecs_t vsyncTimeStamp, nsecs_t vsyncPeriod) {
    std::lock_guard lock(mLock);
    if (mTopLayersList.size() > 0) {
        mLastTopLayerList.clear();
        mLastTopLayerList.assign(mTopLayersList.begin(), mTopLayersList.end());
        mTopLayersList.clear();
    }

    if (mDumpState == Dumping) {
        SurfaceInspector::sVsyncPeriod = vsyncPeriod;
        list<String8>::iterator it;
        for (it = mLastTopLayerList.begin(); it != mLastTopLayerList.end(); it++) {
            mSurfaceInspectorMap[*it].checkFrameDropped(vsyncTimeStamp);
        }

        map<String8, SurfaceInspector>::iterator iter;
        for (iter = mTransactionInspectorMap.begin();
                iter != mTransactionInspectorMap.end(); iter ++) {
            iter->second.checkFrameDropped(vsyncTimeStamp);
        }
    }
}

void SurfaceFlingerInspector::dumpFrameInfo(Parcel* reply) {
    mDumpState = Dumping;
    mLastDumpTime = systemTime(SYSTEM_TIME_MONOTONIC);

    std::lock_guard lock(mLock);
    reply->writeInt32(mLastTopLayerList.size());
    reply->writeInt32(mSurfaceInspectorMap.size() + mTransactionInspectorMap.size());

    map<String8, SurfaceInspector>::iterator iter;
    for (iter = mSurfaceInspectorMap.begin(); iter != mSurfaceInspectorMap.end(); iter ++) {
        iter->second.dump(reply);
    }
    for (iter = mTransactionInspectorMap.begin(); iter != mTransactionInspectorMap.end(); iter ++) {
        iter->second.dump(reply);
    }

    list<String8>::iterator it;
    for (it = mLastTopLayerList.begin(); it != mLastTopLayerList.end(); it++) {
        reply->writeString8(String8("[") + *it + String8("]"));
    }

    for (iter = mSurfaceInspectorMap.begin(); iter != mSurfaceInspectorMap.end(); iter ++) {
        reply->writeString8(String8("[") + iter->first + String8("]"));
        reply->writeString8(iter->second.dropRecordToString());
    }
    for (iter = mTransactionInspectorMap.begin(); iter != mTransactionInspectorMap.end(); iter ++) {
        reply->writeString8(String8("[") + iter->second.getLayerName() + String8("]"));
        reply->writeString8(iter->second.dropRecordToString());
    }

    resetData();
}

void SurfaceFlingerInspector::resetData() {
    map<String8, SurfaceInspector>::iterator iter;
    for (iter = mSurfaceInspectorMap.begin(); iter != mSurfaceInspectorMap.end();) {
        if (find(mLastTopLayerList.begin(), mLastTopLayerList.end(), iter->first) ==
                mLastTopLayerList.end() &&
                find(mTopLayersList.begin(), mTopLayersList.end(), iter->first) ==
                mTopLayersList.end()) {
            mSurfaceInspectorMap.erase(iter ++);
        } else {
            iter->second.reset();
            iter ++;
        }
    }

    nsecs_t now =  systemTime(SYSTEM_TIME_MONOTONIC);
    for (iter = mTransactionInspectorMap.begin(); iter != mTransactionInspectorMap.end();) {
        if (iter->second.noRefreshState(now)) {
            mTransactionInspectorMap.erase(iter ++);
        } else {
            iter->second.reset();
            iter ++;
        }
    }
}

void SurfaceFlingerInspector::markTransactionHandled(String8 layerName) {
    if (mDumpState == Sleep) return;

    std::lock_guard lock(mLock);
    map<String8, SurfaceInspector>::iterator iter = mTransactionInspectorMap.find(layerName);
    if (iter != mTransactionInspectorMap.end()) {
        if (DEBUG_ENABLE) {
            ATRACE_NAME(iter->second.getLayerName().c_str());
            ATRACE_INT(iter->second.getLayerName().c_str(), 1);
            ATRACE_INT(iter->second.getLayerName().c_str(), 0);
        }
        iter->second.markBufferLatched();
    }
}

void SurfaceFlingerInspector::markTransactionStateChanged(String8 layerName, uint64_t what) {
    if (mDumpState == Sleep) return;

    if (layerName.contains(" - animation-leash")) {
        String8 suffix = String8(" ");
        if (what & layer_state_t::ePositionChanged) {
            suffix += "ePositionChanged";
        }
        if (what & layer_state_t::eSizeChanged) {
            suffix += "eSizeChanged";
        }
        if (what & layer_state_t::eBlurRegionsChanged) {
            suffix += "eBlurRegionsChanged";
        }
        if (what & layer_state_t::eMatrixChanged) {
            suffix += "eMatrixChanged";
        }
        if (what & layer_state_t::eTransparentRegionChanged) {
            suffix += "eTransparentRegionChanged";
        }

        if (suffix.size() > 1) {
            std::lock_guard lock(mLock);
            if (mTransactionInspectorMap.find(layerName) == mTransactionInspectorMap.end()) {
                mTransactionInspectorMap[layerName].init(layerName);
                mTransactionInspectorMap[layerName].setPreffix(String8("windowAnimator/"));
            }
            mTransactionInspectorMap[layerName].setSuffix(suffix);
            mTransactionInspectorMap[layerName].producerFrameDropped(NONE_OPERATION_THRESHOLD, 0);
        }
    }
}

void SurfaceFlingerInspector::monitorAppFrame(std::string layerName, int64_t token,
                                              int64_t displayFrameToken, nsecs_t deadlineDelta,
                                              nsecs_t period, bool isAppDeadlineMissed) {
    FrameTimelineInspector::getInstance()->monitorAppFrame(layerName, token,
                                                           displayFrameToken, deadlineDelta,
                                                           period, isAppDeadlineMissed);
}

void SurfaceFlingerInspector::recordBufferTxCount(const char* name, int32_t count)
{
    if (name) {
        if (mBufferTxName.compare(name) !=0) {
            mBufferTxName = name;
        }

        list<string>::iterator it;
        for(it = mBoostAllowList.begin(); it != mBoostAllowList.end(); it++) {
            if (strstr(name, it->c_str())) {
                mBufferTxCount = count;
                // SLOGW("mBufferTxName:%s, mBufferTxCount:%d, FluencyOptimizer", mBufferTxName.c_str(), mBufferTxCount);
                break;
            }
        }

    }
}

int SurfaceFlingerInspector:: getBufferTxCount()
{
    return mBufferTxCount;
}

// FrameTimeline: support Jank detection for doframe -1
void SurfaceFlingerInspector::setVsyncPeriodNsecs(const nsecs_t period) {
    if (mFrameTimelineInjector) {
        mFrameTimelineInjector->setVsyncPeriodNsecs(period);
    }
}

void SurfaceFlingerInspector::setLastPredictions(const std::string layerName, nsecs_t startTime,
                                                 nsecs_t endTime, nsecs_t presentTime) {
    if (mFrameTimelineInjector) {
        mFrameTimelineInjector->setLastPredictions(layerName, startTime, endTime, presentTime);
    }
}

void SurfaceFlingerInspector::getCurrentSurfaceFrameInfo(const std::string layerName,
        int64_t frameStartTimeNanos, int64_t *outFrameVsyncId, nsecs_t *outStartTime,
        nsecs_t *outEndTime, nsecs_t *outPresentTime) {
    if (mFrameTimelineInjector) {
        mFrameTimelineInjector->getCurrentSurfaceFrameInfo(layerName, frameStartTimeNanos,
            outFrameVsyncId, outStartTime, outEndTime, outPresentTime);
    }
}

void SurfaceFlingerInspector::removePredictionsByLayer(const std::string layerName) {
    if (mFrameTimelineInjector) {
        mFrameTimelineInjector->removePredictionsByLayer(layerName);
    }
}
// END

SurfaceInspector::SurfaceInspector() {
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    mLastFrameEndTime = now;
    mLastLatchedFrameEndTime = now;
    mSerialDropRecord = new DropRecordItem;
}

SurfaceInspector::~SurfaceInspector() {
    delete mSerialDropRecord;
}

void SurfaceInspector::init(String8 layerName) {
    mLayerName = layerName;
}

void SurfaceInspector::setSuffix(String8 suffix) {
    if (mSuffix != suffix) {
        mSuffix = suffix;
        mLastLatchedFrameEndTime = systemTime(SYSTEM_TIME_MONOTONIC);
        mSleepedVsyncCount = 0;
        mConsumerDropDuration = 0;
        completeCurrentSerialDrop();
        mConsumerLastFrameDropped = false;
    }
    if (DEBUG_ENABLE) {
        ATRACE_NAME(getLayerName().c_str());
    }
}

void SurfaceInspector::markBufferLatched() {
    mBufferLatched = true;
}

void SurfaceInspector::producerFrameDropped(int32_t droppedFrameCount, int64_t intendedVsyncTime) {
    mIntendedVsyncTime = intendedVsyncTime;
    if (mIntendedVsyncTime != mRemoveIntendedVsyncTime || isTransactionInspector()) {
        mProducerReportCount = droppedFrameCount;
    }
}

// When two surfaces are drawn at the same msg, one of them is judged as the
// first frame, and the other should be the same.
void SurfaceInspector::removeLastDropIfNeeded(int64_t intendedVsyncTime) {
    if (mIntendedVsyncTime == intendedVsyncTime) {
        // The last reported drop form producer should be removed
        if (mProducerReportCount > 0) {
            mProducerReportCount = 0;
        } else if (mSerialDropRecord->counts.size() > 0) {
            // Has been counted into mSerialDropRecord, and needs to be subtracted
            int count = mSerialDropRecord->counts.back();
            mSerialDropRecord->times --;
            mSerialDropRecord->droppedFrameCount -= count;
            mSerialDropRecord->counts.pop_back();
            mSerialDropRecord->duration -= ns2ms((1 + count) * sVsyncPeriod);
        }
    } else {
        mRemoveIntendedVsyncTime = intendedVsyncTime;
    }
}

void SurfaceInspector::checkFrameDropped(nsecs_t vsyncTimeStamp) {
    nsecs_t now =  systemTime(SYSTEM_TIME_MONOTONIC);
    int drewFrameCount = 0;
    int droppedFrameCount = 0;

    //Judgment of frame dropped in the producer stage.
    if (mBufferLatched) {
        drewFrameCount ++;
        droppedFrameCount += checkProducerDropped(vsyncTimeStamp);
        mProducerReportCount = 0;
        mLastLatchedFrameEndTime = now;
        mSleepedVsyncCount = 0;
    } else {
        mSleepedVsyncCount ++;
    }

    //Judgment of frame dropped in the Consumer stage.
    if (mBufferLatched  || mConsumerLastFrameDropped) {
        droppedFrameCount += checkConsumerDropped(vsyncTimeStamp, now);
        mLastFrameEndTime = now;
    }
    mBufferLatched = false;

    updateFrameRecord(drewFrameCount, droppedFrameCount);
}

int SurfaceInspector::checkProducerDropped(nsecs_t vsyncTimeStamp) {
    int droppedFrameCount = 0;

    nsecs_t sleepVsyncCount =
            (vsyncTimeStamp - mLastLatchedFrameEndTime) / sVsyncPeriod;
    sleepVsyncCount = sleepVsyncCount > mSleepedVsyncCount ? sleepVsyncCount : mSleepedVsyncCount;
    if (sleepVsyncCount > 0 && mProducerReportCount > 0) {
        // frame dropping occurred
        if (sleepVsyncCount <= mProducerReportCount) {
            droppedFrameCount += sleepVsyncCount;
        } else {
            // after a period of no operation
            completeCurrentSerialDrop();
            droppedFrameCount += isTransactionInspector() ? 0 : mProducerReportCount;
        }
    }

    // reported from producer, but hasn't drop frame on the SF side
    // reset mIntendedVsyncTime to avoid mistakenly removing
    if (droppedFrameCount == 0 && mProducerReportCount > 0) {
        mIntendedVsyncTime = 0;
    }

    return droppedFrameCount;
}

int SurfaceInspector::checkConsumerDropped(nsecs_t vsyncTimeStamp, nsecs_t now) {
    int droppedFrameCount = 0;

    nsecs_t totalDuration = now - vsyncTimeStamp;

    if (totalDuration > sVsyncPeriod) {
        nsecs_t drawDuration = totalDuration;
        if (vsyncTimeStamp < mLastFrameEndTime) {
            //The last frame also dropped, update drawDuration.
            drawDuration = now - mLastFrameEndTime;
        } else {
            mConsumerDropDuration = 0;
        }

        if (mBufferLatched) {
            droppedFrameCount += (mConsumerDropDuration + drawDuration) / sVsyncPeriod;
        }
        mConsumerDropDuration =
                (mConsumerDropDuration + drawDuration) % sVsyncPeriod - sVsyncPeriod;

        mConsumerLastFrameDropped = true;
    } else {
        mConsumerDropDuration = 0;
        mConsumerLastFrameDropped = false;
    }

    return droppedFrameCount;
}

void SurfaceInspector::updateFrameRecord(int drewFrameCount, int droppedFrameCount) {
    mFrameRecordInfo[FrameRecordInfoIndex::DrewFrameCount] += drewFrameCount;
    if (DEBUG_ENABLE && drewFrameCount > 0) {
        ATRACE_INT((String8("V-dropCount") + getLayerName()).c_str(), droppedFrameCount);
    }

    if (droppedFrameCount > 0) {
        mFrameRecordInfo[FrameRecordInfoIndex::DroppedFrameCount] += droppedFrameCount;
        mSerialDropRecord->times ++;
        mSerialDropRecord->droppedFrameCount += droppedFrameCount;
        mSerialDropRecord->duration += (int) ns2ms(sVsyncPeriod * (droppedFrameCount + 1));
        mSerialDropRecord->counts.push_back(droppedFrameCount);
    } else if (drewFrameCount > 0) {
        // stop recording this serial frame drop
        completeCurrentSerialDrop();
    }
}

void SurfaceInspector::completeCurrentSerialDrop() {
    bool catonHappend = false;
    int frameCount = mSerialDropRecord->times;

    // Select the corresponding threshold according to the current screen refresh rate
    const int* threshold;
    if (sVsyncPeriod < ms2ns(9)) {
        // 120Hz and above
        threshold = thresholds[2];
    } else if (sVsyncPeriod < ms2ns(12)) {
        threshold = thresholds[1];
    } else {
        // 60Hz and below
        threshold = thresholds[0];
    }

    if (frameCount >= threshold[0]) {
        mFrameRecordInfo[FrameRecordInfoIndex::SerialDropTimes] ++;
    }

    list<int>::iterator it;
    for (it = mSerialDropRecord->counts.begin(); it != mSerialDropRecord->counts.end(); it++) {
        if (frameCount >= threshold[0] || *it >= threshold[2]) {
            mFrameRecordInfo[FrameRecordInfoIndex::PerceptibleDroppedFrameCount] += *it;
            catonHappend = true;
        }
        if (frameCount >= threshold[1] || *it >= threshold[3]) {
            mFrameRecordInfo[FrameRecordInfoIndex::SeriousDroppedFrameCount] += *it;
        }

        if (*it >= threshold[2]) {
            mFrameRecordInfo[FrameRecordInfoIndex::SingleDropTimes] ++;
        }
    }

    if (catonHappend) {
        mDropRecordList.push_back(*mSerialDropRecord);
        if (CC_UNLIKELY(DEBUG_ENABLE)) {
            ATRACE_INT("V-catonHappend", 1);
            string result;
            StringAppendF(&result, "%s:%d|%d|%d: ",
                    getLayerName().c_str(), mSerialDropRecord->times,
                    mSerialDropRecord->droppedFrameCount, mSerialDropRecord->duration);
            list<int>::iterator it;
            for (it = mSerialDropRecord->counts.begin(); it != mSerialDropRecord->counts.end();
                 it++) {
                StringAppendF(&result, "%d,", *it);
            }
            SLOGW("%s", result.c_str());
            ATRACE_NAME(result.c_str());
            ATRACE_INT("V-catonHappend", 0);
        }
    }
    delete mSerialDropRecord;
    mSerialDropRecord = new DropRecordItem;
}

void SurfaceInspector::dump(Parcel* reply) {
    for (int i = 0; i < FRAME_RECORD_INFO_SIZE; i++) {
      reply->writeInt32(mFrameRecordInfo[i]);
    }
}

String8 SurfaceInspector::dropRecordToString() {
    if (mSerialDropRecord->times > FRAME_RECORD_MAX_CAPACITY) {
        completeCurrentSerialDrop();
    }
    string result;
    list<DropRecordItem>::iterator it;
    for (it = mDropRecordList.begin(); it != mDropRecordList.end(); it++) {
        StringAppendF(&result, "[%s:%d|%d|%d]",
                getLayerName().c_str(), it->times, it->droppedFrameCount, it->duration);
    }
    mDropRecordList.clear();
    return String8(result.c_str());
}

void SurfaceInspector::reset() {
    for (int i = 0; i < FRAME_RECORD_INFO_SIZE; i++) {
        mFrameRecordInfo[i] = 0;
    }
}

// If it is more than 100 ms from the previous frame refresh,
// it is considered as a no refresh state
bool SurfaceInspector::noRefreshState(nsecs_t now) {
    return now - mLastLatchedFrameEndTime > NONE_OPERATION_THRESHOLD * sVsyncPeriod;
}

} // namespace android
