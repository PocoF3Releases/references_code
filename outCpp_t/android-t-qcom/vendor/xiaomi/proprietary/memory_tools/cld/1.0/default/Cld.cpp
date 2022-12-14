#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <android-base/properties.h>
#include <optional>
#include <cutils/uevent.h>
#include <sys/epoll.h>
#include <utils/Log.h>
#include <errno.h>
#include <utils/Errors.h>
#include "Cld.h"
#include <hidl/HidlTransportSupport.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "mem_cld"

#define QCOM_UFS_DRIVER_PATH "/sys/bus/platform/drivers/ufshcd-qcom/"
#define MTK_UFS_DRIVER_PATH "/sys/bus/platform/drivers/ufshcd-mtk/"
#define UEVENT_MSG_LEN 2048

using android::hardware::interfacesEqual;

static std::optional<std::string> getCldDevicePath() {
    static bool path_walked = false;
    static std::optional<std::string> result = std::nullopt;
    std::string platform = android::base::GetProperty("ro.hardware", "qcom");
    std::string driver_path;

    // Do not need to look for CLD device path every time
    if (path_walked)
        return result;

    path_walked = true;
    driver_path = (platform == "qcom") ? QCOM_UFS_DRIVER_PATH : MTK_UFS_DRIVER_PATH;

    // Walk in UFS platform driver
    struct dirent *fn;
    DIR *dir = opendir(driver_path.c_str());
    if (dir == NULL) {
        ALOGE("Cannot open %s, not an UFS device?", driver_path.c_str());
        return result;
    }

    bool found = false;
    while ((fn = readdir(dir)) != NULL) {
        std::string f = fn->d_name;
        if (f.find("ufs") != std::string::npos) {
            driver_path += f;
            driver_path += "/ufscld/";
            found = true;
            break;
        }
    }

    closedir(dir);

    if (!found) {
        ALOGE("Cannot find UFS platform device, not an UFS device?");
        return result;
    }

    result = driver_path;
    return result;
}

namespace vendor::xiaomi::hardware::cld::implementation {

// Methods from ::vendor::xiaomi::hardware::cld::V1_0::ICld follow.
Return<bool> Cld::isCldSupported() {
    if (auto ufs_path = getCldDevicePath(); ufs_path.has_value()) {
        std::string cld_trigger = ufs_path.value() + "trigger";
        if (access(cld_trigger.c_str(), F_OK) == 0) {
            // Assume CLD is supported if trigger file is accessible
            return true;
        }
    }

    return false;
}

Return<::vendor::xiaomi::hardware::cld::V1_0::FragmentLevel> Cld::getFragmentLevel() {
    using FragLevel = vendor::xiaomi::hardware::cld::V1_0::FragmentLevel;

    if (auto ufs_path = getCldDevicePath(); ufs_path.has_value()) {
        std::string ufs_frag_level = ufs_path.value() + "frag_level";

        int fd = open(ufs_frag_level.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            ALOGE("Failed to open frag level file.");
            return FragLevel::FRAG_ANALYSIS;
        }

        char lvl[4] = { 0 };
        int ret = read(fd, lvl, 1);
        if (ret < 0) {
            ALOGE("Read sysfs node failed.");
            close(fd);
            return FragLevel::FRAG_UNKNOWN;
        }

        close(fd);
        return static_cast<FragLevel>(lvl[0] - '0');
    }

    return FragLevel::FRAG_UNKNOWN;
}

Return<void> Cld::triggerCld(int32_t val) {
    if (auto ufs_path = getCldDevicePath(); ufs_path.has_value()) {
        std::string cld_trigger = ufs_path.value() + "trigger";

        int fd = open(cld_trigger.c_str(), O_RDWR | O_NONBLOCK);
        if (fd < 0) {
            ALOGE("Failed to open trigger file.");
            return Void();
        }

        int ret = write(fd, (val == 0) ? "0" : "1", 1);
        if (ret < 0)
            ALOGE("Failed to write trigger node.");
        close(fd);
    }

    return Void();
}

Return<::vendor::xiaomi::hardware::cld::V1_0::CldOperationStatus> Cld::getCldOperationStatus() {
    using CldOperStats = vendor::xiaomi::hardware::cld::V1_0::CldOperationStatus;

    if (auto ufs_path = getCldDevicePath(); ufs_path.has_value()) {
        std::string cld_stat = ufs_path.value() + "cld_operation_status";

        int fd = open(cld_stat.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            ALOGE("Failed to open cld status file.");
            return CldOperStats::CLD_OPER_NOT_SUPPORTED;
        }

        char lvl[4] = {0};
        int ret = read(fd, lvl, 1);
        if (ret < 0) {
            ALOGE("Read sysfs node failed.");
            close(fd);
            return CldOperStats::CLD_OPER_NOT_SUPPORTED;
        }

        close(fd);
        return static_cast<CldOperStats>(lvl[0] - '0');
    }

    return CldOperStats::CLD_OPER_NOT_SUPPORTED;
}

Return<void> Cld::registerCallback(const sp<::vendor::xiaomi::hardware::cld::V1_0::ICldCallback>& callback) {
    if (callback == nullptr) {
        ALOGE("Failed to register callback, no function specified!");
    } else {
        std::lock_guard<decltype(callback_lock)> lock(callback_lock);
        mCallbacks.push_back(callback);
    }

    return Void();
}

Return<bool> Cld::unregisterCallback(const sp<::vendor::xiaomi::hardware::cld::V1_0::ICldCallback>& callback) {
    if (callback == nullptr) {
        ALOGE("Failed to unregister callback, no function specified!");
        return false;
    }

    bool removed = false;
    std::lock_guard<decltype(callback_lock)> lock(callback_lock);
    for (auto it = mCallbacks.begin(); it != mCallbacks.end(); ) {
        if (interfacesEqual(*it, callback)) {
            it = mCallbacks.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }

    return removed;
}

Cld::Cld() {
    th = std::thread([this](){ UEventWorker(); });
}

void Cld::UEventEvent() {
    char msg[UEVENT_MSG_LEN + 2];
    char *cp;
    int n;

    n = uevent_kernel_multicast_recv(uevent_fd, msg, UEVENT_MSG_LEN);
    if (n <= 0)
        return;
    if (n >= UEVENT_MSG_LEN)   /* overflow -- discard */
        return;

    msg[n] = '\0';
    msg[n + 1] = '\0';
    cp = msg;

    while (*cp) {
        if (!strcmp(cp, "CLD_TRIGGER=OFF")) {
            ALOGI("CLD uevent received: %s", cp);
            {
                std::lock_guard<decltype(callback_lock)> lock(callback_lock);
                if (!mCallbacks.empty()) {
                    for (auto& cb : mCallbacks) {
                        Return<void> ret = cb->notifyStatusChange(getFragmentLevel());
                        if (!ret.isOk())
                            ALOGE("Notify failed: %s", ret.description().c_str());
                    }
                }
            }
        }

        while (*cp++)
            ;
    }
}

void Cld::UEventWorker() {
    int epoll_fd;
    struct epoll_event ev;
    int nevents = 0;

    ALOGI("Creating CLD uevent worker!");

    uevent_fd = uevent_open_socket(64 * 1024, true);
    if (uevent_fd < 0) {
        ALOGE("uevent_open_socket failed");
        return;
    }

    fcntl(uevent_fd, F_SETFL, O_NONBLOCK);

    ev.events = EPOLLIN;
    ev.data.fd = uevent_fd;

    epoll_fd = epoll_create(64);
    if (epoll_fd == -1) {
        ALOGE("epoll_create failed");
        goto error;
    }

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, uevent_fd, &ev) == -1) {
        ALOGE("epoll_ctl failed, errno = %d", errno);
        goto error;
    }

    while (1) {
        struct epoll_event events[64];

        nevents = epoll_wait(epoll_fd, events, 64, -1);
        if (nevents == -1) {
            if (errno == EINTR)
                continue;
            ALOGE("epoll_wait failed, errno = %d", errno);
            break;
        }

        for (int n = 0; n < nevents; n++) {
            if (events[n].data.fd == uevent_fd)
                UEventEvent();
        }
    }

error:
    close(uevent_fd);
    if (epoll_fd >= 0)
        close(epoll_fd);
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//ICld* HIDL_FETCH_ICld(const char* /* name */) {
    //return new Cld();
//}
//
}  // namespace vendor::xiaomi::hardware::cld::implementation
