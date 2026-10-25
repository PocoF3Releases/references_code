#include <fcntl.h>
#include <stdint.h>
#include <android/log.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <cutils/android_filesystem_config.h>
#include <mi_stc_service.h>
#include "../../common/v4.0/DisplayFeatureHal.h"
#define LOG_TAG "MiStcImpl"

using namespace android;
using namespace qClient;

namespace snapdragoncolor {

IMPLEMENT_META_INTERFACE(MiStcService, "android.display.IMiStcService");

status_t BnMiStcService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    ALOGV("code: %d", code);
    // IPC should be from certain processes only
    IPCThreadState* ipc = IPCThreadState::self();
    const int callerPid = ipc->getCallingPid();
    const int callerUid = ipc->getCallingUid();

    const bool permission = (callerUid == AID_MEDIA ||
            callerUid == AID_GRAPHICS ||
            callerUid == AID_ROOT ||
            callerUid == AID_CAMERASERVER ||
            callerUid == AID_AUDIO ||
            callerUid == AID_SYSTEM ||
            callerUid == AID_MEDIA_CODEC);
    if (code == CONNECT_Mi_STC_CLIENT) {
        CHECK_INTERFACE(IMiStcService, data, reply);
        if(callerUid != AID_GRAPHICS) {
            ALOGE("Mi STC service CONNECT_Mi_STC_CLIENT access denied: pid=%d uid=%d",
                   callerPid, callerUid);
            return PERMISSION_DENIED;
        }
        sp<IQClient> client =
                interface_cast<IQClient>(data.readStrongBinder());
        connect(client);
        return NO_ERROR;
    } else if (code > COMMAND_LIST_START && code < COMMAND_LIST_END) {
        if(!permission) {
            ALOGE("Mi STC service access denied: command=%d pid=%d uid=%d",
                   code, callerPid, callerUid);
            return PERMISSION_DENIED;
        }
        CHECK_INTERFACE(IMiStcService, data, reply);
        dispatch(code, &data, reply);
        return NO_ERROR;
    } else {
        return BBinder::onTransact(code, data, reply, flags);
    }
}

MiStcService* MiStcService::sMiStcService = NULL;

void MiStcService::connect(const sp<qClient::IQClient>& client) {
    ALOGI("client connected");
    mClient = client;
}

static inline const char* getFeatureIdName(int FeatureId) {
    switch (FeatureId) {
        case BnMiStcService::SET_PCC : return "PCC";
        case BnMiStcService::SET_PA : return "PA";
        case BnMiStcService::SET_IGC : return "IGC";
        case BnMiStcService::SET_GC : return "GC";
        case BnMiStcService::SET_LUT : return "LUT";
        case BnMiStcService::SET_SRE_IGC : return "SRE_IGC";
        case BnMiStcService::SET_DPPS_PROP : return "SET_DPPS_PROP";
        case BnMiStcService::ENABLE_STC : return "ENABLE STC";
        case BnMiStcService::SET_LOG : return "SET_LOG";
        case BnMiStcService::SET_DISPLAY_STATE :  return "SET_DISPLAY_STATE";
        case BnMiStcService::SET_PARAM : return "SET_PARAM";
        case BnMiStcService::SET_IGC_DITHER : return "SET_IGC_DITHER";
        case BnMiStcService::SET_SRE : return "SET_SRE";
        case BnMiStcService::SET_EYECARE : return "SET_EYECARE";
        case BnMiStcService::RESTORE_BL_PCC : return "RESTORE BL PCC";
        case BnMiStcService::SET_SRE_GC : return "SET SRE GC";
        case BnMiStcService::STC_DUMP : return "STC_DUMP";
        case BnMiStcService::SET_EYECARE_PCC : return "SET_EYECARE_PCC";
        case BnMiStcService::SET_EXIF : return "SET_EXIF";
        default : return "Unknown";
    }
}

status_t MiStcService::dispatch(uint32_t command, const Parcel* inParcel,
        Parcel* outParcel) {
    status_t err = (status_t) FAILED_TRANSACTION;
    IPCThreadState* ipc = IPCThreadState::self();
    //Rewind parcel in case we're calling from the same process
    bool sameProcess = (ipc->getCallingPid() == getpid());
    if(sameProcess)
        inParcel->setDataPosition(0);
    if (mClient.get()) {
        ALOGI("Dispatching command: %d %s", command, getFeatureIdName(command));
        err = mClient->notifyCallback(command, inParcel, outParcel);
        //Rewind parcel in case we're calling from the same process
        if (sameProcess)
            outParcel->setDataPosition(0);
    }
    return err;
}

void MiStcService::init()
{
    if(!sMiStcService) {
        sMiStcService = new MiStcService();

        ALOGI("Creating defaultServiceManager");
        sp<IServiceManager> sm = defaultServiceManager();
        ALOGI("Creating defaultServiceManager...done!");

        ALOGI("Adding display.mistcservice to defaultServiceManager");
        sm->addService(String16("display.mistcservice"), sMiStcService);
        ALOGI("Adding display.mistcservice to defaultServiceManager...done!");

        if(sm->checkService(String16("display.mistcservice")) != NULL)
            ALOGI("Adding display.mistcservice succeeded");
        else
            ALOGI("Adding display.mistcservice failed");
    }
}
}
