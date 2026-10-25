#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <utils/Errors.h>
#include <cutils/log.h>
#include <sys/types.h>
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <set>
#include "ClassFound.h"
#include "MiGLSettings.h"

using namespace android;
using namespace com::xiaomi::joyose;

#define REMOTE_SERVICE_NAME         "xiaomi.joyose"
#define REMOTE_SERVICE_INTERFACE    "com.xiaomi.joyose.IJoyoseInterface"
#define GET_MIGL_PREFS_CODE         IBinder::FIRST_CALL_TRANSACTION + 12

#define MIN_THIRD_APP_UID           10000

#define NON_ERROR                   0
#define CALL_FROM_SYSTEM            1
#define SERVICE_NOT_EXIST           2
#define CALL_REMOTE_FAILED          3
#define RECEIVE_EXCEPTION           4
#define RECEIVE_NULL_OBJECT         5

namespace android {
namespace migl {

MiGLSettings::MiGLSettings() {
    remoteService = nullptr;
    miglPrefs = nullptr;
    myPid = (unsigned int)getpid();
    std::string cmdline;
    if (!android::base::ReadFileToString(android::base::StringPrintf("/proc/%d/cmdline", myPid), &cmdline)) {
        ALOGE("Failed to read the process name");
    }

    static std::set<std::string> whitelist = {
        "com.miHoYo.Yuanshen",
        "com.miHoYo.GenshinImpact",
        "com.miHoYo.ys.mi",
        "com.miHoYo.ys.bilibili",
        "com.tencent.tmgp.pubgmhd"};
    if (whitelist.find(cmdline.c_str()) != whitelist.end()) {
        ALOGI("pid: %d, cmdline: %s", myPid, cmdline.c_str());
        initMiGLPrefs();
    }
}

void MiGLSettings::onRemoteServiceDied() {
    remoteService = nullptr;
    ALOGE("remote service died!");
}

int MiGLSettings::initMiGLPrefs() {
    status_t ret;
    Parcel data, reply;
    remoteService = defaultServiceManager()->checkService(String16(REMOTE_SERVICE_NAME));
    if (remoteService == nullptr) {
        ALOGW("remote service not exist.");
        return SERVICE_NOT_EXIST;
    }

    class RemoteDeath : public IBinder::DeathRecipient {
        public:
            RemoteDeath(MiGLSettings* settings)
                : miglSettings(settings) { }
            ~RemoteDeath() override = default;
            void binderDied(const ::android::wp<::android::IBinder>& /* who */) override {
                miglSettings->onRemoteServiceDied();
            }
        private:
            MiGLSettings* miglSettings;
    };
    sp<IBinder::DeathRecipient> deathRecipient = new RemoteDeath(this);
    remoteService->linkToDeath(deathRecipient);

    data.writeInterfaceToken(String16(REMOTE_SERVICE_INTERFACE));
    if (initialized == false && initActivityThreadClass()) {
       initialized =  true;
       mAppName = getCurrentPackageName();
    }
    data.writeString16(String16(mAppName.c_str()));
    ret = remoteService->transact(GET_MIGL_PREFS_CODE, data, &reply);
    if (ret != NO_ERROR) {
        ALOGE("migl:call remote service failed.");
        return CALL_REMOTE_FAILED;
    }
    // read Exception Code
    if (!reply.readExceptionCode()) {
        // receive an non-null object
        std::string cmds = String8(reply.readString16()).string();
        if (!cmds.empty()) {
            miglPrefs = new MiGLPrefs();
            miglPrefs->readFromJoyose(cmds);
            miglPrefs->miglEnabled = true;
            return NON_ERROR;
        } else {
            miglPrefs = new MiGLPrefs();
            miglPrefs->miglEnabled = false;
        }
        return RECEIVE_NULL_OBJECT;
    }
    return RECEIVE_EXCEPTION;
}

}
}
