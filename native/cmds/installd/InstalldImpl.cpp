//
// Created by mi on 21-7-6.
//
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <binder/IPCThreadState.h>
#include <android-base/scopeguard.h>
#include <android-base/stringprintf.h>
#include <private/android_filesystem_config.h>
#include <private/android_projectid_config.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <selinux/android.h>
#include <log/log.h>               // TODO: Move everything to base/logging.
#include "InstalldImpl.h"
#include "miui_datamanager.h"
#include <cutils/sockets.h>
#include <thread>
#include <sys/wait.h>
#include <sys/types.h>
#include <iostream>

using android::base::StringPrintf;
using android::IPCThreadState;

namespace android {
namespace installd {

namespace {
static android::binder::Status ok() {
    return android::binder::Status::ok();
}

static android::binder::Status exception(uint32_t code, const std::string& msg) {
    LOG(ERROR) << msg << " (" << code << ")";
    return android::binder::Status::fromExceptionCode(code, String8(msg.c_str()));
}

android::binder::Status checkUid(uid_t expectedUid) {
    uid_t uid = IPCThreadState::self()->getCallingUid();
    if (uid == expectedUid || uid == AID_ROOT) {
        return ok();
    } else {
        return exception(android::binder::Status::EX_SECURITY,
                StringPrintf("UID %d is not expected UID %d", uid, expectedUid));
    }
}

#define ENFORCE_UID(uid) {                                  \
    android::binder::Status status = checkUid((uid));                \
    if (!status.isOk()) {                                   \
        return status;                                      \
    }                                                       \
}
} //namespace end

bool InstalldImpl::moveData(const std::string& src, const std::string& dst,
        int32_t uid, int32_t gid, const std::string& seInfo, int32_t flag){
    return move_files(src.c_str(), dst.c_str(), uid, gid, seInfo.c_str(), flag);
}

int InstalldImpl::wait_child(pid_t pid) {
    int status;
    pid_t got_pid;

    while (1) {
        got_pid = waitpid(pid, &status, 0);
        if (got_pid == -1 && errno == EINTR) {
            printf("waitpid interrupted, retrying\n");
        } else {
            break;
        }
    }
    if (got_pid != pid) {
        ALOGW("waitpid failed: wanted %d, got %d: %s\n",
            (int) pid, (int) got_pid, strerror(errno));
        return 1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    } else {
        return status;  /* always nonzero */
    }
}

int mi_dexopt_fd = -1;
#define DEXOPT_SOCKET_NAME "mi_dexopt"

int InstalldImpl::get_dexopt_fd() {
    int fd = socket_local_client(DEXOPT_SOCKET_NAME,
                                ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    LOG(WARNING) << "Dexopt client socket connectting.";
    if (fd < 0) {
        ALOGE("Fail to connect to socket pkms. return code: %d", fd);
        return -1;
    }
    return fd;
}

void InstalldImpl::send_mi_dexopt_result(const char* pkg_name, int secondaryId, int result) {
    write_mi_dexopt_result(pkg_name, secondaryId, result);
}

void InstalldImpl::write_mi_dexopt_result(const char* pkg_name, int secondaryId, int result) {
    if (mi_dexopt_fd < 0) {
        mi_dexopt_fd = get_dexopt_fd();
        if (mi_dexopt_fd < 0) {
            return;
        }
    }
    char buf[256];
    int ret = snprintf(buf, sizeof(buf), "%s:%d:%d\r\n", pkg_name, secondaryId, result);
    if (ret >= (ssize_t)sizeof(buf) || ret < 0) {
        ALOGE("mi_dexopt write buffer error!");
        return;
    }

    ssize_t written;
    written = write(mi_dexopt_fd, buf, strlen(buf) + 1);
    if (written < 0) {
        ALOGE("mi_dexopt written:%zu", written);
        close(mi_dexopt_fd);
        mi_dexopt_fd = -1;
        return;
    }
}

void InstalldImpl::dex2oat_thread(const std::string package_name, pid_t pid, int secondaryId) {
    int result = wait_child(pid);
    if (result == 0) {
        LOG(VERBOSE) << "DexInv: --- END '" << package_name.c_str() << "' secondaryId is "
                     << secondaryId << " (success) ---";
        result = 1;
    } else {
        LOG(VERBOSE) << "DexInv: --- END '" << package_name.c_str() << "' secondaryId is "
                     << secondaryId << " --- status=0x" << std::hex << result << ", process failed";
        result = -1;
    }
    write_mi_dexopt_result(package_name.c_str(), secondaryId, result);
}

void InstalldImpl::dexopt_async(const std::string package_name, pid_t pid, int secondaryId) {
    std::thread th(dex2oat_thread, package_name, pid, secondaryId);
    th.detach();
}

extern "C" IInstalldStub* create() {
    return new InstalldImpl;
}

extern "C" void destroy(IInstalldStub* impl) {
    delete impl;
}

} //namespace android
} //namespace installd
