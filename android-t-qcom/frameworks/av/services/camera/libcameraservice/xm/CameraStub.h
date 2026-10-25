#ifndef ANDROID_CAMERA_CAMERASTUB_H
#define ANDROID_CAMERA_CAMERASTUB_H

#include "ICameraStub.h"
#include "../CameraService.h"
#include <media/AudioSystem.h>
#include "MiCondition.h"

//using android::CameraService;
//using namespace android::camera3;
//using android::CaptureRequest;
using namespace android;
using namespace android::camera3;
using android::MiCondition;
using android::Thread;
using android::camera3::InFlightRequestMap;
using android::Camera3Device;
using android::HidlCamera3Device;
using android::AidlCamera3Device;

struct CachedResult {
    int64_t frameNumber;
    nsecs_t exposureTime;
    nsecs_t availableTime;
    nsecs_t shutterTimestamp;
};

#if defined(__LP64__)
#define LIBPATH "/system_ext/lib64/libcameraimpl.so"
#else
#define LIBPATH "/system_ext/lib/libcameraimpl.so"
#endif

const std::vector<std::string> lowerPriorityPkgNameList = {
   "com.android.keyguard",
   "com.android.systemui",
   "com.xiaomi.biometric",
   "com.miui.face",
   "org.codeaurora.ims"
};

class CameraSingleton {
public:
    virtual ~CameraSingleton() {}
    static ICameraStub *GetImplInstance();
    void DestroyImplInstance();
private:
    CameraSingleton() {}
    static void *LibCameraImpl;
    static ICameraStub *ImplInstance;
};

namespace CameraStub {
    void calVariance();
    void resetVariance();
    void updateSessionParams(CameraMetadata& sessionParams);
    void setClientPackageName(const String16& clientName, int32_t api_num);
    void adjSysCameraPriority(const String8& packageName,
        std::vector<int> &priorityScores, std::vector<int> &states);
    void waitFaceAuthenticated(const String8& packageName,
        CameraService::CameraClientManager &clientManager);
    void adjNativeCameraPriority(const String8& key, int32_t& score_adj, int32_t& state_adj);
    void adjCameraPriority(const String8& packageName,
        std::vector<int> &priorityScores, std::vector<int> &states,
        CameraService::CameraClientManager &clientManager,
        std::vector<int> ownerPids);
    int32_t getClientIndex(CameraService::CameraClientManager &clientManager,
           const String8& clientPackageName, std::vector<int> ownerPids);
    void setNfcPolling(const char *value);
    bool checkSensorFloder(CameraMetadata *CameraCharacteristics,
        CameraMetadata *gCameraCharacteristics, pthread_rwlock_t *rwLock);
    audio_stream_type_t getAudioStreamType();
    int getShrinkMode(const CameraMetadata& info);
    void detachBuffer(sp<camera3::Camera3OutputStreamInterface> outputStream,
        int shrinkMode, bool isActiveStream);
    bool checkWhiteNameList(const String8& clientName8);
    void getCustomBestSize(const CameraMetadata& info, int32_t width,
         int32_t height, int32_t format, int32_t &bestWidth, int32_t &bestHeight);
    void createCustomDefaultRequest(CameraMetadata& metadata);
    int getPluginRequestCount(const CameraMetadata &metadata, const CameraMetadata &deviceInfo);
    void dumpDropInfo(bool isVideoStream, int format, nsecs_t timestamp,  nsecs_t lastTimeStamp);
    void resetFrameInfo();
    status_t repeatAddFakeTriggerIds(status_t res, CameraMetadata &metadata);
    void checkAllClientsStoppedAndReclaimMem(int32_t *cameraProviderPid, bool *memDirty);
    void notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName);
    String16 getplatformversion();
    void setTargetSdkVersion(int targetSdkVersion);
    int getTargetSdkVersion();

    bool isSupportPrivacyCamera(const CameraMetadata& cameraMetadata);
    android::IPrivacyCamera* createPrivacyCamera(const CameraMetadata & cameraMetadata);
    void reportSdkConfigureStreams(const String8 &cameraId, camera_stream_configuration *config, const CameraMetadata &sessionParams);
    void reportSdkRequest(camera_capture_request_t *request);
    bool checkInvisibleMode(int32_t mode);
    bool isAllowPreviewRespace();
    bool postInFlightRequestIfReadyLocked(CaptureOutputStates& states, int idx);
    bool initJank(wp<android::Camera3Device> parent, wp<android::HidlCamera3Device> hidlParent, wp<android::AidlCamera3Device> aidlParent, const CameraMetadata& sessionParams);
    void setJankStates(int states);
    void configMaxBuffer(camera_stream_configuration config);
    void cloneRequest(sp<HidlCamera3Device::CaptureRequest> dst, sp<HidlCamera3Device::CaptureRequest> src);
    void cloneRequest(sp<AidlCamera3Device::CaptureRequest> dst, sp<AidlCamera3Device::CaptureRequest> src, bool aidlDevice);
    void setErrorState(const char *fmt, ...);
    void postProcessCaptureResult(CaptureOutputStates& states, const uint32_t frameNumber);
    void checkIsUnReleasedBuild();
    class XJank : public Thread {
      public:
        XJank(wp<Camera3Device> parent, wp<HidlCamera3Device> hidlParent, wp<AidlCamera3Device> aidlParent);
        ~XJank();

        wp<Camera3Device>     mParent;
        wp<HidlCamera3Device> mHidlParent;
        wp<AidlCamera3Device> mAidlParent;

        virtual void requestExit();

        int receiver();
        void JankRun();
        void JankReset();

        MiCondition              mSignal;
        int                      mFrameOffset;
        bool                     mEnableJank;
        bool                     mDebug;
        bool                     first_request_returned;
        CachedResult             mLastResult;
        std::deque<CachedResult> mCachedResults;
        std::mutex               mLock;
        private:
        virtual bool threadLoop();
    };
};
#endif
