#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <string>

#include <cutils/sockets.h>
#include <cutils/properties.h>

#include "ModemEventHandler.h"

const char CHECKNV_DEV_PATH[] = "/checknv_errimei/errimei";

using namespace std;

#if 0
#include <arpa/inet.h>
#include <sys/un.h>
#include <android-base/file.h>

const char UNCRYPT_SOCKET[] = "/dev/socket/uncrypt";
static constexpr int SOCKET_CONNECTION_MAX_RETRY = 30;


static bool reboot_into_recovery() {
    const char options[] = "--show_bad_nv\n";

    property_set("ctl.start", "setup-bcb");
    sleep(1);

    sockaddr_un un = {};
    un.sun_family = AF_UNIX;
    strlcpy(un.sun_path, UNCRYPT_SOCKET, sizeof(un.sun_path));

    int sockfd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sockfd == -1) {
        ALOGE("sockfd invalid");
        return false;
    }

    // Connect to the uncrypt socket.
    bool success = false;
    for (int retry = 0; retry < SOCKET_CONNECTION_MAX_RETRY; retry++) {
      if (connect(sockfd, reinterpret_cast<sockaddr*>(&un), sizeof(sockaddr_un)) != 0) {
        success = true;
        break;
      }
      sleep(1);
    }
    if (!success) {
        ALOGE("connect socket failed");
        if (close(sockfd)) {
           ALOGE("close socket failed");
        }
        return false;
    }

    // Send out the BCB message.
    int length = htonl(strlen(options));
    if (!(android::base::WriteFully(sockfd, &length, sizeof(int)))) {
        ALOGE("Failed to write length, errno %s", strerror(errno));
        if (close(sockfd)) {
           ALOGE("close socket failed");
        }
        return false;
    }
    if (!(android::base::WriteFully(sockfd, options, strlen(options)))) {
        ALOGE("Failed to write options, errno %s", strerror(errno));
        if (close(sockfd)) {
           ALOGE("close socket failed");
        }
        return false;
    }

    // Check the status code from uncrypt.
    int status;
    if (!(android::base::ReadFully(sockfd, &status, sizeof(int)))) {
        ALOGE("Failed to read status, errno %s", strerror(errno));
        if (close(sockfd)) {
           ALOGE("close socket failed");
        }
        return false;
    }
    if (ntohl(status) != 100U) {
        ALOGE("check status failed");
        if (close(sockfd)) {
           ALOGE("close socket failed");
        }
        return false;
    }

    // Ack having received the status code.
    int code = 0;
    if (!(android::base::WriteFully(sockfd, &code, sizeof(int)))) {
        ALOGE("Failed to write ack, errno %s", strerror(errno));
        if (close(sockfd)) {
           ALOGE("close socket failed");
        }
        return false;
    }
    if (close(sockfd)) {
        ALOGE("close socket failed");
        return false;
    }

    ALOGD("reboot to recovery to remind user that the NV is destroyed!");
    property_set("ctl.stop", "setup-bcb");
    property_set("sys.powerctl", "reboot,recovery");
    return true;
}
#else
#include <binder/IBinder.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <utils/String16.h>

using namespace android;

static const uint32_t TRANSACTION_uncrypt = IBinder::FIRST_CALL_TRANSACTION + 1;
static const uint32_t TRANSACTION_setupBcb = IBinder::FIRST_CALL_TRANSACTION + 2;
static const uint32_t TRANSACTION_clearBcb = IBinder::FIRST_CALL_TRANSACTION + 3;
static const uint32_t TRANSACTION_rebootRecoveryWithCommand = IBinder::FIRST_CALL_TRANSACTION + 4;

static bool reboot_into_recovery() {
    if (!property_get_bool("sys.boot_completed", false)) {
        ALOGD("Waiting for boot complete");
        return false;
    }

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> recovery = sm->getService(String16("recovery"));
    if (recovery == NULL) {
        ALOGE("recovery is null");
        return false;
    }
    ALOGD("get recovery service success");

    Parcel data, reply;
    data.writeInterfaceToken(String16("android.os.IRecoverySystem"));
    data.writeString16(String16("--show_bad_nv"));

    status_t ret = recovery->transact(TRANSACTION_rebootRecoveryWithCommand, data, &reply);
    if (ret == NO_ERROR) {
        ALOGD("reboot to recovery success");
        property_set("sys.powerctl", "reboot,recovery");
        return true;
    }

    ALOGE("send recovery command fail, ret: %d", ret);

    return false;
}
#endif

ModemEventHandler::ModemEventHandler(int sock) : NetlinkListener(sock) {
}

ModemEventHandler::~ModemEventHandler() {}

void ModemEventHandler::onEvent(NetlinkEvent *evt)
{
    NetlinkEvent::Action action = evt->getAction();
    string subsys(evt->getSubsystem());
    string evtPath(evt->findParam("DEVPATH")?evt->findParam("DEVPATH"):"");
    if (action == NetlinkEvent::Action::kAdd && evtPath == string(CHECKNV_DEV_PATH)) {
        ALOGE("NV check fail, reboot to recovery");
        if (reboot_into_recovery()) {
            ALOGD("goes into recovery, stop listener");
            this->stopListener();
        }
    }
#if 0
    // Test only!!!
    if (action == NetlinkEvent::Action::kAdd && subsys == string("ssc") && evtPath == string("/devices/virtual/ssc/sensors")) {
        if (reboot_into_recovery()) {
            ALOGD("goes into recovery, stop listener");
            this->stopListener();
        }
    }
#endif
}
