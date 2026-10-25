#define LOG_TAG "CameraStub"
#define LOG_NDEBUG 0

#include <iostream>
#include <dlfcn.h>
#include <log/log.h>
#include <mutex>
#include <cutils/properties.h>
#include <utils/Trace.h>
#include <ui/GraphicBuffer.h>
#include "device3/Camera3OutputStreamInterface.h"

#include "CameraStub.h"

#define TRACE_SYNC_BEGIN_F(...)                                                                   \
    {                                                                                             \
        char traceNameRandomString[256];                                                          \
        snprintf(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__);            \
        ATRACE_BEGIN(traceNameRandomString);                                                      \
    }

#define TRACE_SYNC_END()                                                                          \
    {                                                                                             \
        ATRACE_END();                                                                             \
    }

#define TRACE_ASYNC_BEGIN_F(id, ...)                                                              \
    {                                                                                             \
        char traceNameRandomString[256];                                                          \
        snprintf(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__);            \
        ATRACE_ASYNC_BEGIN(traceNameRandomString, static_cast<uint32_t>(id));                     \
    }

#define TRACE_ASYNC_END_F(id, ...)                                                                \
    {                                                                                             \
        char traceNameRandomString[256];                                                          \
        snprintf(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__);            \
        ATRACE_ASYNC_END(traceNameRandomString, static_cast<uint32_t>(id));                       \
    }

#define SET_ERR(fmt, ...) setErrorState(   \
    "%s: " fmt, __FUNCTION__,              \
    ##__VA_ARGS__)

static sp<CameraStub::XJank> mXJank    = NULL;
static String16 mJankClientPackageName = String16 ("");
static bool bJankAvailable             = false;
static bool bJankFeatureEnable         = false;
static int  mSessionMaxFps             = 0;
static nsecs_t mLastPreviewTimeCache   = 0;
static int logProp                     = 0;

#define TIME_GROUP 5000
#define THRESHOLD_VARIANCE 15

struct PreviewFrame {
public:
    int mFrameNumber = 0;
    nsecs_t mTimestamp = 0;
};

int count = 0;
std::vector<PreviewFrame> vectorPreview = {};
void* CameraSingleton::LibCameraImpl = nullptr;
ICameraStub* CameraSingleton::ImplInstance = nullptr;
static std::mutex StubLock;
int mCurMaxMissedFrameNumber = 0;
int mCurTotalMissedFrameNumber = 0;
int mCurTotalFrameNumber = 0;
int mMaxMissedFrameNumber = 0;
int mTotalMissedFrameNumber = 0;
int mTotalFrameNumber = 0;

ICameraStub* CameraSingleton::GetImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (ImplInstance == nullptr) {
        LibCameraImpl = dlopen(LIBPATH, RTLD_LAZY);
        if (LibCameraImpl != nullptr) {
            PFN_CameraImpl_Create pCamera_Create =
                 (PFN_CameraImpl_Create)dlsym(LibCameraImpl, "create");
            ImplInstance = pCamera_Create();
            char value[PROPERTY_VALUE_MAX] = {0};
            property_get("persist.service.camera.frame_variance", value, "0");
            logProp = atoi(value);
        } else {
            ALOGE("%s:%d dlopen failed %s", __FUNCTION__, __LINE__, dlerror());
        }
    }
    return ImplInstance;
}

void CameraSingleton::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibCameraImpl) {
        PFN_CameraImpl_Destroy pCamera_Destroy =
            (PFN_CameraImpl_Destroy)dlsym(LibCameraImpl, "destroy");
        pCamera_Destroy(ImplInstance);
        dlclose(LibCameraImpl);
        LibCameraImpl = NULL;
        ImplInstance = NULL;
    }
}

bool isPreviewStream(int usage) {
    bool isPreviewStream = false;
        if ((GRALLOC_USAGE_HW_COMPOSER == (GRALLOC_USAGE_HW_COMPOSER & usage)) ||
            (GRALLOC_USAGE_HW_TEXTURE == (GRALLOC_USAGE_HW_TEXTURE & usage)))
    {
        isPreviewStream = true;
    }

    return isPreviewStream;
}

bool isVideoStream(int usage) {
    return (usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) != 0;
}

namespace CameraStub {

void dumpMissedFrameInfo() {
        if(mCurTotalMissedFrameNumber < 0) mCurTotalMissedFrameNumber = 0;
        if(mCurMaxMissedFrameNumber > mMaxMissedFrameNumber) {
            mMaxMissedFrameNumber = mCurMaxMissedFrameNumber;
        }
        mTotalMissedFrameNumber += mCurTotalMissedFrameNumber;
        mTotalFrameNumber += mCurTotalFrameNumber;
        ALOGW("%s: cameraServer current max missed framenumber = %d"
            ", current total missed framenumber = %d, current total framenumber %d"
            ", all max missed framenumber %d, all total missed framenumber %d, total framenumber %d"
            ", mSessionMaxFps %d",
            __FUNCTION__, mCurMaxMissedFrameNumber, mCurTotalMissedFrameNumber, mCurTotalFrameNumber,
            mMaxMissedFrameNumber, mTotalMissedFrameNumber, mTotalFrameNumber, mSessionMaxFps);
        mCurMaxMissedFrameNumber = 0;
        mCurTotalMissedFrameNumber = 0;
        mCurTotalFrameNumber = 0;
}

void triggerCalVariance(int status) {
    uint64_t averageTime;
    uint64_t usageTime;
    int size = vectorPreview.size();
    mCurTotalFrameNumber += size;
    if(size < 2 ) {
        if(status == 0 && size > 0) dumpMissedFrameInfo();
        return;
    }
    float frameduration = mSessionMaxFps > 0 ? 1e3f/mSessionMaxFps : 1e3f/30;

    usageTime = vectorPreview.back().mTimestamp - vectorPreview.front().mTimestamp;
    averageTime = usageTime / (size - 1);
    mCurTotalMissedFrameNumber += (usageTime/frameduration - size +1);

    uint64_t maxInterval = 0;
    uint64_t minInterval = UINT_MAX;
    uint64_t variance = 0;
    PreviewFrame pre = vectorPreview.front();
    for (auto iter = vectorPreview.begin()+1;iter != vectorPreview.end();iter++) {
        PreviewFrame next = *iter;
        uint64_t time = next.mTimestamp - pre.mTimestamp;
        if (time > maxInterval) {
            maxInterval = time;
        }
        if (time < minInterval) {
            minInterval = time;
        }
        variance += (time - averageTime) * (time - averageTime);
        pre = next;
    }
    variance /= (size - 1);
    if (variance > THRESHOLD_VARIANCE || logProp) {
        ALOGE("%s: cameraServer send preview frame : variace = %d,maxInterval = %d,minInterval = %d,first frame = %d,last frame = %d",
               __FUNCTION__, variance,maxInterval,minInterval,vectorPreview.front().mFrameNumber,vectorPreview.back().mFrameNumber);
    }

    int max_missedFrameNumber = (maxInterval/frameduration);
    if(max_missedFrameNumber > mCurMaxMissedFrameNumber) {
        mCurMaxMissedFrameNumber = max_missedFrameNumber;
    }

    if(status == 0) {
        dumpMissedFrameInfo();
    }
    vectorPreview.clear();
}

void calVariance() {
    std::lock_guard<std::mutex> lock(StubLock);
    PreviewFrame previewFrame;
    previewFrame.mFrameNumber = ++count;
    previewFrame.mTimestamp = systemTime(SYSTEM_TIME_MONOTONIC) / 1e6;
    vectorPreview.push_back(previewFrame);
    if(previewFrame.mTimestamp - vectorPreview.front().mTimestamp >= TIME_GROUP) {
        triggerCalVariance(-1);
    }
}

void updateSessionParams(CameraMetadata& sessionParams) {
    ALOGV("%s:%d ", __FUNCTION__, __LINE__);
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        stubInstance->updateSessionParams(sessionParams);
}

void setClientPackageName(const String16& clientName, int32_t api_num) {
    ALOGV("%s:%d ", __FUNCTION__, __LINE__);
    mJankClientPackageName=clientName;
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        stubInstance->setClientPackageName(clientName, api_num);
}

int32_t getClientIndex(CameraService::CameraClientManager &clientManager,
       const String8& clientPackageName, std::vector<int> ownerPids) {

    int32_t ownerPid = -1;
    String8 packageName;
    auto mClients = clientManager.getAll();

    for (auto client : mClients) {
        auto clientSp = client->getValue();
        if (clientSp.get() != nullptr) {
            packageName = String8{clientSp->getPackageName()};
            ALOGD("%s:%d %s", __FUNCTION__, __LINE__, packageName.string());
            if (!clientPackageName.compare(packageName)) {
                ownerPid = client->getOwnerId();
            }
        }
    }
    if (ownerPid == -1) {
        return -1;
    }
    for (size_t i = 0; i < ownerPids.size(); i++) {
        if (ownerPid == ownerPids[i]) {
            return i;
        }
    }
    return -1;
}

void adjSysCameraPriority(const String8& packageName,
     std::vector<int> &priorityScores, std::vector<int> &states) {
    ALOGV("%s:%d ", __FUNCTION__, __LINE__);
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        stubInstance->adjSysCameraPriority(packageName, priorityScores,
            states);
}

void waitFaceAuthenticated(const String8& packageName,
    CameraService::CameraClientManager &clientManager) {

    ALOGV("%s:%d ", __FUNCTION__, __LINE__);

    String8 systemCameraPackageName("com.android.camera");
    String8 faceUnlockPackageName("com.miui.face");
    std::vector<CameraService::DescriptorPtr> activeClientsDescriptor;
    activeClientsDescriptor = clientManager.getAll();
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    for (const auto& i : activeClientsDescriptor) {
        bool faceUnlockPackage = false;
        if (nullptr != stubInstance)
          faceUnlockPackage = stubInstance->isFaceUnlockPackage(i->getKey(), static_cast<bool>(i->getPriority().isVendorClient()));
        if (systemCameraPackageName.compare(packageName) && (faceUnlockPackage ||
            !faceUnlockPackageName.compare(String8(i->getValue()->getPackageName().string())))) {
            // Wait 400ms for faceunlock FaceAuthenticated
            clientManager.waitUntilRemoved(i, 400000000);
        }
    }
}

void adjNativeCameraPriority(const String8& key,int32_t& score_adj,int32_t& state_adj){
   ALOGV("%s:%d ", __FUNCTION__, __LINE__);
   ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
   if (nullptr != stubInstance)
       stubInstance->adjNativeCameraPriority(key, score_adj, state_adj);
}

void adjCameraPriority(const String8& packageName,
    std::vector<int> &priorityScores, std::vector<int> &states,
    CameraService::CameraClientManager &clientManager,
    std::vector<int> ownerPids) {

    ALOGV("%s:%d ", __FUNCTION__, __LINE__);

    int32_t clientIdx = -1;
    std::vector<std::pair<std::string, int>> clientList;
    for (auto lowerPkgName : lowerPriorityPkgNameList) {
        clientIdx = getClientIndex(clientManager,
                           String8(lowerPkgName.c_str()), ownerPids);
        clientList.push_back(std::pair<std::string, int>(lowerPkgName, clientIdx));
    }

    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        stubInstance->adjCameraPriority(packageName, priorityScores,
            states, clientList);
}

void setNfcPolling(const char *value) {
    ALOGV("%s:%d", __FUNCTION__, __LINE__);
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        stubInstance->setNfcPolling(value);
}

bool checkSensorFloder(CameraMetadata *CameraCharacteristics, CameraMetadata *gCameraCharacteristics, pthread_rwlock_t *rwLock) {
    bool ret = false;
    ALOGV("%s:%d", __FUNCTION__, __LINE__);
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance) {
        ret = stubInstance->checkSensorFloder(CameraCharacteristics, gCameraCharacteristics, rwLock);
    }
    return ret;
}

audio_stream_type_t getAudioStreamType() {
    audio_stream_type_t type = AUDIO_STREAM_SYSTEM;
    char value[PROPERTY_VALUE_MAX];
    // check global version
    property_get("ro.product.mod_device", value, "");
    if (strstr(value, "_global") != NULL) {
    // check enfoce sound country code
        property_get("ro.miui.region", value, "");
        if (strstr(value, "KR") != NULL || strstr(value, "JP") != NULL) {
            type = AUDIO_STREAM_ENFORCED_AUDIBLE;
        }
    }
    return type;
}

int getShrinkMode(const CameraMetadata& info) {
    ATRACE_CALL();

    uint32_t tag = 0;
    int shrinkMode = -1;
    //0x0: not shrink, 0x1: shrink inactive streams, 0x2: shrink all streams.
    char tagName[]="xiaomi.memory.shrinkMode";
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance) {
        status_t res = stubInstance->getTagFromName(info, tagName, tag);
        if (res == OK) {
            camera_metadata_ro_entry shrinkModeTag = info.find(tag);
            if (shrinkModeTag.count > 0) {
                shrinkMode = shrinkModeTag.data.i32[0];
                ALOGD("%s:%d shrinkMode %d", __FUNCTION__, __LINE__, shrinkMode);
            }
        }
    }

    return shrinkMode;
}

void detachBuffer(sp<camera3::Camera3OutputStreamInterface> outputStream, int shrinkMode,
                          bool isActiveStream) {
    if (shrinkMode == 0x2 || !isActiveStream) {
        sp<GraphicBuffer> buffer;
        do {
            outputStream->detachBuffer(&buffer, nullptr);
        } while(buffer.get() != nullptr);
    }
}

bool checkWhiteNameList(const String8& clientName8) {
    const std::vector<std::string> whiteNameList = {
        "com.android.camera",
    };
    bool inWhiteList = false;
    for (auto pgkname : whiteNameList) {
       if (!clientName8.compare(String8(pgkname.c_str()))) {
           ALOGD("clientName8:%s in white list", clientName8.string());
           inWhiteList = true;
       }
    }
    return inWhiteList;
}

void getCustomBestSize(const CameraMetadata& info, int32_t width, int32_t height,
     int32_t format, int32_t &bestWidth, int32_t &bestHeight)
{
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        stubInstance->getCustomBestSize(info, width, height, format,
                            bestWidth, bestHeight);
}

void createCustomDefaultRequest(CameraMetadata& metadata) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        stubInstance->createCustomDefaultRequest(metadata);
}

status_t repeatAddFakeTriggerIds(status_t res, CameraMetadata &metadata) {
    status_t ret = OK;
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        ret = stubInstance->repeatAddFakeTriggerIds(res, metadata);

    return ret;
}

void checkAllClientsStoppedAndReclaimMem(int32_t *cameraProviderPid, bool *memDirty) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance)
        stubInstance->checkAllClientsStoppedAndReclaimMem(cameraProviderPid, memDirty);
}

void notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance) {
        ALOGV("%s:%d", __FUNCTION__, __LINE__);
        stubInstance->notifyCameraStatus(cameraId, status, clientPackageName);
    }
    if(status == 0) { //camera close
        std::lock_guard<std::mutex> lock(StubLock);
        triggerCalVariance(0);
    }
}

String16 getplatformversion() {
    String16 platformversion = String16("");
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if (nullptr != stubInstance) {
        platformversion = stubInstance->getplatformversion();
    }
    return platformversion;
}

void setTargetSdkVersion(int targetSdkVersion) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance) {
        stubInstance->setTargetSdkVersion(targetSdkVersion);
    }
}

int getTargetSdkVersion() {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance) {
        return stubInstance->getTargetSdkVersion();
    }
    return -1;
}

bool postInFlightRequestIfReadyLocked(CaptureOutputStates& states, int idx) {
    InFlightRequestMap& inflightMap = states.inflightMap;
    android::camera3::InFlightRequest &request = inflightMap.editValueAt(idx);
    if (mXJank == NULL || !states.enableJank) {
        return false;
    }

    bool has_preview_buffer = false;
    for (size_t i = 0;i < request.pendingOutputBuffers.size(); i++) {
        Camera3Stream *stream =
                Camera3Stream::cast(request.pendingOutputBuffers[i].stream);
        int usage = stream->getUsage();
        if (isPreviewStream(usage)) {
            has_preview_buffer = true;
            break;
        }
    }

    // When we get the preview buffer,post to jank ASAP.
    if (has_preview_buffer &&
        request.bufferReturned == STATUS_INITED) {

        assert(request.requestStatus != OK ||
               request.pendingOutputBuffers.size() == 0);

        const uint32_t frameNumber = inflightMap.keyAt(idx);

        if (mXJank->mDebug) {
            int64_t diff;
            if (mLastPreviewTimeCache == 0) {
                mLastPreviewTimeCache = systemTime();
            } else {
                diff = systemTime() - mLastPreviewTimeCache;
                mLastPreviewTimeCache = systemTime();
                ATRACE_INT64("preview_before_jank", (int64_t)diff);
            }
            ALOGI("%s post frameNum:%d", __FUNCTION__, frameNumber);
        }

        CachedResult cr     = {0,0,0,0};
        cr.frameNumber      = frameNumber;
        cr.shutterTimestamp = request.shutterTimestamp;
        cr.exposureTime     = cr.shutterTimestamp -mXJank->mLastResult.shutterTimestamp;
        cr.availableTime    = mXJank->mLastResult.availableTime + cr.exposureTime;

        if (!mXJank->first_request_returned) {
            cr.exposureTime  = 1e9f/mSessionMaxFps;
            cr.availableTime = systemTime() + cr.exposureTime;
        } else if (cr.exposureTime > 2 * mXJank->mLastResult.exposureTime) {
            // frame drop
            cr.exposureTime  = mXJank->mLastResult.exposureTime;
            cr.availableTime = systemTime() +mXJank->mLastResult.exposureTime;
        }
        if (cr.exposureTime < 0) {
            //In this case request.shutterTimestamp is smaller than mLastResult.shutterTimestamp,
            //it might be wrong and we have to avoid it
            cr.exposureTime     = mXJank->mLastResult.exposureTime;
            cr.shutterTimestamp = mXJank->mLastResult.shutterTimestamp + cr.exposureTime;
        }
        mXJank->mLastResult.frameNumber      = cr.frameNumber;
        mXJank->mLastResult.exposureTime     = cr.exposureTime;
        mXJank->mLastResult.shutterTimestamp = cr.shutterTimestamp;
        mXJank->mLastResult.availableTime    = cr.availableTime;

        TRACE_SYNC_BEGIN_F("post frame: %d | available %" PRId64, frameNumber, cr.availableTime);

        mXJank->mCachedResults.push_back(cr);
        request.bufferReturned = STATUS_POSTED;

        mXJank->mSignal.signal();

        TRACE_SYNC_END();
    }

    if (request.bufferReturned == STATUS_RETURNED || request.bufferReturned == STATUS_INITED) {
        //In this case the preview buffer is returned and we no longer cache other buffers
        //or pendingOutputBuffers without preview buffers(snapshot without preview)
        return false;
    }

    return true;
}

bool initJank(wp<android::Camera3Device> parent, wp<android::HidlCamera3Device> hidlParent, wp<android::AidlCamera3Device> aidlParent, const CameraMetadata& sessionParams) {
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.service.camera.jank_enable", value, "0");
    bJankFeatureEnable = atoi(value);

    const camera_metadata_t* metadata = sessionParams.getAndLock();
    camera_metadata_ro_entry_t entry;
    memset(&entry,0,sizeof(camera_metadata_ro_entry_t));
    if (metadata != NULL)
    {
        int ret = find_camera_metadata_ro_entry(metadata, ANDROID_CONTROL_AE_TARGET_FPS_RANGE, &entry);
        if(ret == 0)
        {
            mSessionMaxFps = entry.data.i32[1];
        }
        property_get("persist.service.camera.fps", value, "60");
        if (atoi(value) > mSessionMaxFps) {
            bJankFeatureEnable = false;
        }
    }

    if (bJankFeatureEnable) {
        int isJank = !strcmp(String8(mJankClientPackageName).string(), "com.android.camera");
        if (isJank && !bJankAvailable ) {
            mXJank = new XJank(parent, hidlParent, aidlParent);
        }

        if (mXJank != NULL) {
            mXJank->mEnableJank = isJank;
            bJankAvailable = true;
            char value[PROPERTY_VALUE_MAX] = {0};
            property_get("persist.service.camera.frame_offset", value, "0");
            mXJank->mFrameOffset = atoi(value);
            property_get("persist.service.camera.debug", value, "0");
            mXJank->mDebug = atoi(value);
            ALOGI("%s PackageName:%s ,mEnableJank: %d mFrameOffset: %d",
                __FUNCTION__, String8(mJankClientPackageName).string(),  mXJank->mEnableJank, mXJank->mFrameOffset);
            return mXJank->mEnableJank;
        }
    }

    if (mXJank != NULL && bJankFeatureEnable == false ) {
        setJankStates(STATES_JANKEXIT);
        setJankStates(STATES_JANKJOIN);
    }
    return false;
}

void setJankStates(int states) {
    if (mXJank != NULL) {
      switch(states) {
          case STATES_JANKEXIT:
              mXJank->requestExit();
              break;
          case STATES_JANKJOIN:
              mXJank->join();
              bJankAvailable  = false;
              mXJank          = NULL;
              break;
          case STATES_JANKRUN:
              mXJank->JankRun();
              break;
          case STATES_JANKRESET:
              mXJank->JankReset();
              break;
        }
    }
}
void configMaxBuffer(camera_stream_configuration config) {
    for (size_t i = 0; i < config.num_streams; i++) {
        android::camera3::camera_stream *dst = config.streams[i];
        dst->max_buffers += 1;
    }
}

void XJank::JankRun() {
    run(String8::format("%s", "X-Jank"), PRIORITY_HIGHEST);
}

void XJank::JankReset() {
    first_request_returned = false;
    mLastResult            = {0,0,0,0};
    mCachedResults.clear();
}

int XJank::receiver()
{
    ATRACE_CALL();
    std::lock_guard<std::mutex> l(mLock);

    if (mCachedResults.size() == 0) {
        mSignal.waitRelative(mLock, 5e7);
        if (exitPending()) {
            return false;
        } else {
            return true;
        }
    }

    sp<Camera3Device> parent  = mParent.promote();
    std::mutex* mInFlightLock = (std::mutex*)parent->getmInFlightLock();
    std::lock_guard<std::mutex> InFlightLock(*mInFlightLock);

    for (; mCachedResults.size() != 0 && !exitPending(); ) {
        int     frameNum  = -1;

        frameNum = mCachedResults.front().frameNumber;

        int idx = parent->getmInFlightMap().indexOfKey(frameNum);
        if (idx == NAME_NOT_FOUND) {
            ALOGI("Unknown frame number for capture result: %d, Maybe return by flush!", frameNum);
            mCachedResults.pop_front();

            return 1;
        }

        const android::camera3::InFlightRequest &request = parent->getmInFlightMap().valueAt(idx);

        if (request.shutterTimestamp != request.sensorTimestamp) {
            ALOGW("The timestamp is not matched frame %d status: %d shutter: %" PRId64 ", sensor %" PRId64,
                  frameNum, request.requestStatus, request.shutterTimestamp, request.sensorTimestamp);
        }

        TRACE_SYNC_BEGIN_F("Trace Frame %d \n", frameNum);

        // mFrameOffset           enable feature
        // skipResultMetadata     should be in case of flush
        if (frameNum < mFrameOffset || request.skipResultMetadata) {
            goto release;
        }

        if (parent->getmInFlightMap().keyAt(idx) != 0) {
            nsecs_t availableTime = 0;
            availableTime = mCachedResults.front().availableTime;
            nsecs_t timeWait = availableTime - systemTime()-1e6;

            if(mDebug) {
                ALOGI("%s cacheNum:%d pop frameNum:%d wait:%" PRId64,
                        __FUNCTION__, (int)mCachedResults.size(), frameNum, timeWait);
            }

            // use 1ms to reduce effect of schdule and lock
            while (timeWait > 1e6 && !exitPending()) {
                mSignal.waitRelative(*mInFlightLock, timeWait);
                timeWait = availableTime - systemTime()-1e6;
            }
        }
release:
        // refetch the idx when call wait (release mLock), it may change.
        idx = parent->getmInFlightMap().indexOfKey(frameNum);
        if (idx == NAME_NOT_FOUND) {
            ALOGI("Unknown frame number for capture result: %d, Maybe return by flush!", frameNum);
            mCachedResults.pop_front();
            TRACE_SYNC_END();
            return 1;
        }

        if (mHidlParent != NULL) {
            sp<HidlCamera3Device> hidlParent = mHidlParent.promote();
            HidlCaptureOutputStates states   = hidlParent->getCaptureOutputStates(mEnableJank, hidlParent);
            postProcessCaptureResult(states, frameNum);
        } else if (mAidlParent != NULL) {
            sp<AidlCamera3Device> aidlParent = mAidlParent.promote();
            AidlCaptureOutputStates states   = aidlParent->getCaptureOutputStates(mEnableJank, aidlParent);
            postProcessCaptureResult(states, frameNum);
        }
        mCachedResults.pop_front();

        TRACE_SYNC_END();
    }
    return 0;
}

void postProcessCaptureResult(CaptureOutputStates& states, const uint32_t frameNumber) {
    ATRACE_CALL();
    int idx;
    idx = states.inflightMap.indexOfKey(frameNumber);
    InFlightRequest &r = states.inflightMap.editValueAt(idx);

    r.bufferReturned = STATUS_RETURNED;
    mXJank->first_request_returned = true;

    returnAndRemovePendingOutputBuffers(states.useHalBufManager, states.listener, r, states.sessionStatsBuilder, states.privacyCamera);
    removeInFlightRequestIfReadyLocked(states, idx);
}

void checkIsUnReleasedBuild() {
    bool isUnRelease = property_get_bool("ro.mi.development", false);
    ALOGI("check isUnReleasedBuild: %d", isUnRelease);
    if (isUnRelease) {
        property_set("vendor.camera.sensor.logsystem.unrelease", "1");
    } else {
        property_set("vendor.camera.sensor.logsystem.unrelease", "0");
    }
}

void XJank::requestExit() {
    // Call parent to set up shutdown
    first_request_returned = false;
    Thread::requestExit();
    // The exit from any possible waits
    mSignal.signal();
}

XJank::XJank(wp<Camera3Device> parent, wp<HidlCamera3Device> hidlParent, wp<AidlCamera3Device> aidlParent):
    mParent(parent) {
    mFrameOffset = 0;
    mDebug       = false;
    mEnableJank  = false;
    first_request_returned = false;
    mLastResult            = {0,0,0,0};
    if (hidlParent != NULL) {
        ALOGI("Enable jank feature, this is HidlDevice");
        mHidlParent = hidlParent;
    }
    if (aidlParent != NULL) {
        ALOGI("Enable jank feature, this is AidlDevice");
        mAidlParent = aidlParent;
    }
}

XJank::~XJank() {
    mFrameOffset  = 0;
    mDebug        = false;
    mParent       = NULL;
    mHidlParent   = NULL;
    mAidlParent   = NULL;
    mCachedResults.clear();
    Thread::requestExit();
    mSignal.signal();
}

bool XJank::threadLoop() {
    ATRACE_CALL();
    receiver();
    return true;
}

int getPluginRequestCount(const CameraMetadata &metadata, const CameraMetadata &deviceInfo) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr == stubInstance) {
        return 0;
    }
    return stubInstance->getPluginRequestCount(metadata, deviceInfo);
}

void cloneRequest(sp<HidlCamera3Device::CaptureRequest> dst, sp<HidlCamera3Device::CaptureRequest> src) {
        if(NULL == dst || NULL == src) {
        return;
    }
    // return stubInstance->cloneRequest(dst, src);
    dst->mSettingsList = src->mSettingsList;

    auto presettings = src->mSettingsList.begin();
    for (auto& settings : dst->mSettingsList) {
         if (presettings == src->mSettingsList.end()) {
             break;
         }
         memcpy(settings.mOriginalTestPatternData, presettings->mOriginalTestPatternData, 4*sizeof(int));
         presettings++;
    }

    dst->mOutputSurfaces = src->mOutputSurfaces;
    dst->mBatchSize = src->mBatchSize;
    dst->mRotateAndCropAuto = src->mRotateAndCropAuto;
    dst->mZoomRatioIs1x = src->mZoomRatioIs1x;
    dst->mRepeating = src->mRepeating;
    dst->mRequestTimeNs = src->mRequestTimeNs;
    dst->mResultExtras.burstId = src->mResultExtras.burstId;
    dst->mResultExtras.requestId = src->mResultExtras.requestId;
    uint8_t tmp = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
    dst->mSettingsList.begin()->metadata.update(ANDROID_CONTROL_CAPTURE_INTENT, &tmp, 1);
    dst->mSettingsList.begin()->metadata.erase(ANDROID_CONTROL_ENABLE_ZSL);
}

void cloneRequest(sp<AidlCamera3Device::CaptureRequest> dst, sp<AidlCamera3Device::CaptureRequest> src, bool aidlDevice) {
       if(NULL == dst || NULL == src) {
        return;
    }
     (void) aidlDevice;
    dst->mSettingsList = src->mSettingsList;

    auto presettings = src->mSettingsList.begin();
    for (auto& settings : dst->mSettingsList) {
         if (presettings == src->mSettingsList.end()) {
             break;
         }
         memcpy(settings.mOriginalTestPatternData, presettings->mOriginalTestPatternData, 4*sizeof(int));
         presettings++;
    }

    dst->mOutputSurfaces = src->mOutputSurfaces;
    dst->mBatchSize = src->mBatchSize;
    dst->mRotateAndCropAuto = src->mRotateAndCropAuto;
    dst->mZoomRatioIs1x = src->mZoomRatioIs1x;
    dst->mRepeating = src->mRepeating;
    dst->mRequestTimeNs = src->mRequestTimeNs;
    dst->mResultExtras.burstId = src->mResultExtras.burstId;
    dst->mResultExtras.requestId = src->mResultExtras.requestId;
    uint8_t tmp = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
    dst->mSettingsList.begin()->metadata.update(ANDROID_CONTROL_CAPTURE_INTENT, &tmp, 1);
    dst->mSettingsList.begin()->metadata.erase(ANDROID_CONTROL_ENABLE_ZSL);
}

bool isSupportPrivacyCamera(const CameraMetadata& cameraMetadata) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr == stubInstance) {
        return false;
    }

    return stubInstance->isSupportPrivacyCamera(cameraMetadata);
}

android::IPrivacyCamera* createPrivacyCamera(const CameraMetadata& cameraMetadata) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr == stubInstance) {
        return nullptr;
    }

    return stubInstance->createPrivacyCamera(cameraMetadata);
}

void setErrorState(const char *fmt, ...) {
    sp<Camera3Device> parent = mXJank->mParent.promote();
    if (parent != NULL) {
        parent->executesetErrorState(fmt);
    }
}

void reportSdkConfigureStreams(const String8 &cameraId, camera_stream_configuration *config, const CameraMetadata &sessionParams) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr == stubInstance) {
        return;
    }
    stubInstance->reportSdkConfigureStreams(cameraId,config,sessionParams);
}

void reportSdkRequest(camera_capture_request_t *request) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr == stubInstance) {
        return;
    }
    stubInstance->reportSdkRequest(request);
}

void dumpDropInfo(bool isVideoStream, int format, nsecs_t timestamp,  nsecs_t lastTimeStamp) {
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr == stubInstance) {
        return;
    }
    stubInstance->dumpDropInfo(isVideoStream, format, timestamp, lastTimeStamp);
}

void resetFrameInfo() {
    resetVariance();
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr == stubInstance) {
        return;
    }
    stubInstance->resetFrameInfo();
}
void resetVariance() {
    std::lock_guard<std::mutex> lock(StubLock);
    triggerCalVariance(0);
    count = 0;
    vectorPreview.clear();
}

bool checkInvisibleMode(int32_t mode) {
    bool invisibleMode = false;
    ICameraStub* stubInstance = CameraSingleton::GetImplInstance();
    if(nullptr != stubInstance) {
        invisibleMode = stubInstance->checkInvisibleMode(mode);
    }
    return invisibleMode;
}

bool isAllowPreviewRespace()
{
    int ret = true;
    char value[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.service.camera.google_feature_enable", value, "1");
    if (!atoi(value)) {
        ret = false;
    }

    if (bJankFeatureEnable) {
        ret = false;
    }
    ALOGI("%s: google_feature:%d, jank_feature:%d, allow: %d", __FUNCTION__, atoi(value), bJankFeatureEnable, ret);
    return ret;
}

}
