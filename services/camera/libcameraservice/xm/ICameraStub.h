#ifndef ANDROID_CAMERA_ICAMERASTUB_H
#define ANDROID_CAMERA_ICAMERASTUB_H

#include <camera/CameraMetadata.h>
#include <system/camera_metadata.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include "../device3/Camera3Device.h"
#include "../device3/hidl/HidlCamera3Device.h"
#include "../device3/aidl/AidlCamera3Device.h"
#include "xm/IPrivacyCamera.h"

#include "../device3/Camera3StreamInterface.h"
#include "../device3/InFlightRequest.h"
#include "../device3/Camera3OutputUtils.h"
#include "../device3/hidl/HidlCamera3OutputUtils.h"
#include "../device3/aidl/AidlCamera3OutputUtils.h"

using android::CameraMetadata;
using android::String16;
using android::String8;
using android::sp;
using android::wp;
using android::status_t;
using android::Camera3Device;
using namespace android::camera3;
enum JankStates{
    STATES_JANKEXIT,
    STATES_JANKJOIN,
    STATES_JANKRUN,
    STATES_JANKRESET,
};

class ICameraStub {

public:
    virtual ~ICameraStub() {}
    virtual void setClientPackageName(const String16& clientName, int32_t api_num) = 0;
    virtual void updateSessionParams(CameraMetadata& sessionParams) = 0;
    virtual void adjSysCameraPriority(const String8& packageName,
        std::vector<int> &priorityScores, std::vector<int> &states) = 0;
    virtual void adjCameraPriority(const String8& packageName,
        std::vector<int> &priorityScores, std::vector<int> &states,
        std::vector<std::pair<std::string, int>> &clientList) = 0;
    virtual void adjNativeCameraPriority(const String8& key, int32_t& score_adj, int32_t& state_adj) = 0;
    virtual bool isFaceUnlockPackage(const String8& cameraId, bool systemNativeClient) = 0;
    virtual void setNfcPolling(const char *value) = 0;
    virtual bool checkSensorFloder(CameraMetadata *CameraCharacteristics,
        CameraMetadata *gCameraCharacteristics, pthread_rwlock_t *rwLock) = 0;
    virtual status_t getTagFromName(const CameraMetadata& info,
            char* tagName, uint32_t &tag) = 0;
    virtual void getCustomBestSize(const CameraMetadata& info, int32_t width /*in*/,
        int32_t height /*in*/, int32_t format /*in*/, int32_t &bestWidth /*out*/,
        int32_t &bestHeight /*out*/) = 0;
    virtual void createCustomDefaultRequest(CameraMetadata& metadata) = 0;
    virtual status_t repeatAddFakeTriggerIds(status_t res, CameraMetadata &metadata) = 0;
    virtual void checkAllClientsStoppedAndReclaimMem(int32_t *cameraProviderPid,
        bool *memDirty) = 0;
    virtual void notifyCameraStatus(const int32_t cameraId, const int32_t status, const String16& clientPackageName);
    virtual String16 getplatformversion();
    virtual void setTargetSdkVersion(int targetSdkVersion) = 0;
    virtual int getTargetSdkVersion();
    virtual int getPluginRequestCount(const CameraMetadata &metadata, const CameraMetadata &deviceInfo);
    virtual void reportSdkConfigureStreams(const String8 &cameraId, camera_stream_configuration_t *config, const CameraMetadata &sessionParams);
    virtual void reportSdkRequest(camera_capture_request_t *request);
    virtual void dumpDropInfo(bool isVideoStream, int format, nsecs_t timestamp,  nsecs_t lastTimeStamp);
    virtual void resetFrameInfo();
    virtual bool isSupportPrivacyCamera(const CameraMetadata& cameraMetadata) = 0;
    virtual android::IPrivacyCamera* createPrivacyCamera(const CameraMetadata & cameraMetadata) = 0;
    virtual bool checkInvisibleMode(int32_t mode);
};


typedef ICameraStub* (*PFN_CameraImpl_Create)();
typedef void (*PFN_CameraImpl_Destroy)(ICameraStub *pStub);
#endif
