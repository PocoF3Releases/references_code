#include <fcntl.h>
#include <stdint.h>
#include <android/log.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <cutils/android_filesystem_config.h>
#include <mi_clstc_service.h>

#define LOG_TAG "MiClstcImpl"

using namespace android;
using namespace qClient;

namespace clstc {

IMPLEMENT_META_INTERFACE(MiClstcService, "android.display.IMiClstcService");

status_t BnMiClstcService::onTransact(
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
    if (code == CONNECT_Mi_CLSTC_CLIENT) {
        CHECK_INTERFACE(IMiClstcService, data, reply);
        if(callerUid != AID_GRAPHICS) {
            ALOGE("Mi CLSTC service CONNECT_Mi_CLSTC_CLIENT access denied: pid=%d uid=%d",
                   callerPid, callerUid);
            return PERMISSION_DENIED;
        }
        sp<IQClient> client =
                interface_cast<IQClient>(data.readStrongBinder());
        connect(client);
        return NO_ERROR;
    } else if (code > COMMAND_LIST_START && code < COMMAND_LIST_END) {
        if(!permission) {
            ALOGE("Mi CLSTC service access denied: command=%d pid=%d uid=%d",
                   code, callerPid, callerUid);
            return PERMISSION_DENIED;
        }
        CHECK_INTERFACE(IMiClstcService, data, reply);
        dispatch(code, &data, reply);
        return NO_ERROR;
    } else {
        return BBinder::onTransact(code, data, reply, flags);
    }
}

MiClstcService* MiClstcService::sMiClstcService = NULL;

void MiClstcService::connect(const sp<qClient::IQClient>& client) {
    ALOGI("client connected");
    mClient = client;
}

static inline const char* getFeatureIdName(int FeatureId) {
    switch (FeatureId) {
        case BnMiClstcService::SET_PCC : return "PCC";
        case BnMiClstcService::SET_IGC : return "IGC";
        case BnMiClstcService::SET_GC  : return "GC";
        case BnMiClstcService::SET_LUT : return "LUT";
        case BnMiClstcService::SET_LOG : return "LOG";
        default : return "Unknown";
    }
}

status_t MiClstcService::dispatch(uint32_t command, const Parcel* inParcel,
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
bool MiClstcService::init_done = false;
void MiClstcService::init()
{
    ALOGE("%s init done %d, %p", __func__, init_done, sMiClstcService);
    if (init_done)
        return;

    if(!sMiClstcService) {
        sMiClstcService = new MiClstcService();
        ALOGI("Creating defaultServiceManager");
        sp<IServiceManager> sm = defaultServiceManager();
        ALOGI("Creating defaultServiceManager...done!");
        ALOGI("Adding miclstcservice to defaultServiceManager");
        sm->addService(String16("miclstcservice"), sMiClstcService);
        ALOGI("Adding display.mistcservice to defaultServiceManager...done!");

        if(sm->checkService(String16("miclstcservice")) != NULL) {
            ALOGI("Adding miclstcservice succeeded");
            init_done = true;
        } else
            ALOGI("Adding miclstcservice failed");
    }
}
}
