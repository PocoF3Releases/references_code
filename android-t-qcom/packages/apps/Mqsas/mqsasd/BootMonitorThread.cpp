/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#define LOG_TAG "BootMonitor"

#include <algorithm>
#include <regex>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <utils/String8.h>
#include <utils/SystemClock.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <android/log.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android-base/strings.h>
#include <android-base/stringprintf.h>
#include <android-base/properties.h>
#include <private/android_filesystem_config.h>
#include "BootMonitorThread.h"
#include "dump.h"
#include "utils.h"

using android::base::ReadFileToString;
using android::base::StringPrintf;
using android::base::GetProperty;
using android::base::unique_fd;

static const std::string UNCRYPT_SOCKET = "/dev/socket/uncrypt";
static const std::string INIT_SVC_SETUP_BCB = "init.svc.setup-bcb";
static const std::string INIT_SVC_CLEAR_BCB = "init.svc.clear-bcb";
static const std::string INIT_SVC_UNCRYPT = "init.svc.uncrypt";
static const std::string RO_SAFEMODE_PROPERTY = "ro.sys.safemode";
static const std::string MAINTENANCE_MODE_PROP = "sys.maintenance_mode";

// 尝试开机成功次数
#define BOOT_RERTY_LIMIT            5
#define TRAP_MODE                   80
// 每次等待boot complete轮询时间
#define ONCE_WAIT_SECONDS           10
#define BOOTCOMPLETE_PROP           "sys.boot_completed"
#define BOOTDISPLAYCOMPLETE_PROP    "sys.boot_display_completed"
#define BOOTMONITOR_STEP_TIME       20
#define BOOTMONITOR_LEVEL_TIME      300
#define BOOTMONITOR_TIME_OUT        1200
#define SOCKET_CONNECT_MAX_RETRY    30

namespace android {

/**
 * Enable safemode when set safemode flag file.
 * 
 * @safemodePath: safemode flag path
 */
static void checkSafeMode(std::string& safemodePath) {
    std::fstream safeModeFile;
    safeModeFile.open(safemodePath, std::ios::in);
    if (safeModeFile) {
        ALOGD("Safe Mode may be set.");
        std::string context;
        safeModeFile >> context;
        if (context == "true") {
            int result = property_set(RO_SAFEMODE_PROPERTY.c_str(), "1");
            if (result) {
                ALOGE("set RO_SAFEMODE_PROPERTY failed! error code is %d\n", result);
            }
        }
        safeModeFile.close();
        if(remove(safemodePath.c_str())) {
            ALOGE("remove path %s failed! info: %s\n", safemodePath.c_str(), strerror(errno));
        }
    } else {
        ALOGD("Safe Mode not set.");
    }
}

/**
 * Enable maintenance mode when set maintenance mode flag file.
 * 
 * @maintenancemodePath: maintenance mode flag path
 */
static void checkMaintenanceMode(std::string& maintenancemodePath) {
    std::fstream maintenanceModeFile;
    maintenanceModeFile.open(maintenancemodePath, std::ios::in);
    if (maintenanceModeFile) {
        ALOGD("Maintenance Mode may be set.");
        std::string context;
        maintenanceModeFile >> context;
        if (context == "true") {
            int result = property_set(MAINTENANCE_MODE_PROP.c_str(), "1");
            if (result) {
                ALOGE("set MAINTENANCE_MODE_PROP failed! error code is %d\n", result);
            }
        }
        maintenanceModeFile.close();
        if(remove(maintenancemodePath.c_str())) {
            ALOGE("remove path %s failed! info: %s\n", maintenancemodePath.c_str(), strerror(errno));
        }
    } else {
        ALOGD("Maintenance Mode not set.");
    }
}

/**
 * Enable trap mode when set trap mode flag file.
 * trap mode can auto capture log when mqsasd started (5s).
 *
 * @trapmodePath: trap mode flag path
 */
static bool checkTrapMode(std::string& trapmodePath, int &trap_time) {
    bool result = false;
    std::fstream trapModeFile;
    trapModeFile.open(trapmodePath, std::ios::in);
    if (trapModeFile.is_open()) {
        ALOGD("Trap Mode may be set.");
        std::string context1;
        std::string context2;
        trapModeFile >> context1;
        trapModeFile >> context2;
        if (context1 == "true") {
            result = true;
        }
        trap_time = atoi(context2.c_str());
        trapModeFile.close();
        if(remove(trapmodePath.c_str())) {
            ALOGE("remove path %s failed!\n", trapmodePath.c_str());
        }
    } else {
        ALOGD("Trap Mode not set.");
        return false;
    }
    return result;
}

/**
 * Determine the log type according to the waiting time and the count of unsuccessful startup.
 * timeout: wait_time ≈ 300 s || wait_time ≈ 600 s || wait_time ≈ 900 s || wait_time ≈ 1200 s
 * inc: wait_time < 300 s
 * rcy: boot_count >= 5
 *
 * @boot_count: the count of unsuccessful startup
 * @wait_time: bootmonitor runtime
 */
static std::string buildLogName(int boot_count, long long wait_time) {
    time_t t = time(NULL);
    long int slot = time(&t);
    std::string device = GetProperty("ro.product.device", "unknown");
    std::string miui_version = GetProperty(
            "ro.build.version.incremental", "unknown");

    if (boot_count == TRAP_MODE) {
        return StringPrintf("bm_trap_N_%s_%s_%ld.zip",
                device.c_str(), miui_version.c_str(), slot);
    }

    if (boot_count >= BOOT_RERTY_LIMIT) {
        return StringPrintf("bm_rcy_%d_%s_%s_%ld.zip",
                boot_count, device.c_str(), miui_version.c_str(), slot);
    }

    if (wait_time < BOOTMONITOR_LEVEL_TIME) {
        return StringPrintf("bm_inc_%d_%lld_%s_%s_%ld.zip",
            boot_count, wait_time, device.c_str(), miui_version.c_str(), slot);
    }

    return StringPrintf("bm_timeout_%d_%lld_%s_%s_%ld.zip",
            boot_count, wait_time, device.c_str(), miui_version.c_str(), slot);
}

static bool isDigits(std::string& context) {
    return context.size() != 0 && std::all_of(context.begin(), context.end(), ::isdigit);
}

/**
 * @abs_path: current log storage path
 *
 * Find the latest log in each type.
 */
static int checkAndSaveLog(std::string& abs_path) {

    android::base::unique_fd fd(TEMP_FAILURE_RETRY(open(abs_path.c_str(),
                            O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)));
    if (fd == -1) {
        ALOGW("Faile to open file: %s, %s", abs_path.c_str(), strerror(errno));
        return false;
    }

    struct stat st = {};
    if (fstat(fd, &st) == -1) {
        ALOGW("Faile to read file stat: %s, %s", abs_path.c_str(), strerror(errno));
        return false;
    }

    if (st.st_size < 1024) {
        ALOGW("log too small, ready to delete.");
        return false;
    }

    return true;
}

static void removeDeprecatedLog(std::string& logdir, std::string& currLog) {
    std::string bootMonitorCurrPrefix = "bm_";
    if (currLog.size() > 6) {
        bootMonitorCurrPrefix = currLog.substr(0, 6);
    }
    std::string bugreportTempFilePrefix = "bugreport";

    auto exclusion_predicate = [&](const char* name, bool /*is_dir*/) {
        const std::string base_name(name);

        // not 'bm_' and 'bugreport' prefix files need to be retained
        if (base_name.find(bootMonitorCurrPrefix) != 0 && base_name.find(bugreportTempFilePrefix) != 0) {
            return true;
        }

        // save the latest log in each type
        if (base_name.find(currLog) != std::string::npos) {
            return true;
        }

        return false;
    };

    auto abs_path = logdir + currLog;
    // The old log is cleared only when the currently fetched log is valid
    if (checkAndSaveLog(abs_path)) {
        int rc = delete_dir_contents(logdir.c_str(), /* delete_dir */ false, exclusion_predicate);
        if (rc != 0) {
            LOG(ERROR) << "failed delete dir content " << logdir << " :" << strerror(errno);
        }
    } else if (remove(abs_path.c_str())) {
        ALOGE("remove path %s failed! info: %s\n", abs_path.c_str(), strerror(errno));
    }
}

static int readBootCountAndCheck(std::string& bootcntPath) {
    std::string value;
    std::fstream bootCountFile;
    bootCountFile.open(bootcntPath, std::ios::in);
    if (bootCountFile) {
        bootCountFile >> value;
        bootCountFile.close();
    } else {
        ALOGW("Faile to read file: %s, %s", bootcntPath.c_str(), strerror(errno));
        value = "0";
    }

    if (value == "disable") {
        ALOGE("explicit disable BootMonitor.");
        return -1;
    }

    if (android::base::GetBoolProperty("persist.sys.miui_spo_test", false)) {
        ALOGE("disable BootMonitor for spo test.");
        return -1;
    }

    if (!isDigits(value)) {
        ALOGW("An invalid string was read: %s", value.c_str());
        value = "0";
    }

    return std::stoi(value);
}

static void resetBootCount(std::string& bootcntPath, int retryCount) {
    CaptureLogUtil dump;
    char buffer[32] = { 0 };
    snprintf(buffer, sizeof(buffer), "%d", retryCount);
    if (remove(bootcntPath.c_str())) {
        ALOGE("remove path %s failed! info: %s (resetBootCount)\n", bootcntPath.c_str(), strerror(errno));
    }
    dump.create_file_write(bootcntPath.c_str(), buffer);
}

static void clearBootCount(std::string& bootcntPath) {
    if (remove(bootcntPath.c_str())) {
        ALOGE("remove path %s failed! info: %s (clearBootCount)\n", bootcntPath.c_str(), strerror(errno));
    }
}

static void createLogDirIfNeeded(const char* logdir) {
    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(logdir), closedir);
    if (!dir) {
        mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
        if (create_dir_if_needed(logdir, mode)) {
            ALOGE("create dir %s failed!\n", logdir);
            return;
        }
        if (chown(logdir, AID_ROOT, AID_SYSTEM)) {
            ALOGE("Failed to chown to user system for path %s, errinfo=%s.",
                logdir, strerror(errno));
            return;
        }
        if (chmod(logdir, mode)) {
            ALOGE("Failed to chmod for path %s, errinfo=%s.", logdir, strerror(errno));
            return;
        }
    }
}

static void stopService() {
    android::base::SetProperty("ctl.stop", "setup-bcb");
    android::base::SetProperty("ctl.stop", "clear-bcb");
    android::base::SetProperty("ctl.stop", "uncrypt");

    bool success = false;
    for (int retry = 0; retry < SOCKET_CONNECT_MAX_RETRY; retry++) {
        std::string setup_bcb = android::base::GetProperty(INIT_SVC_SETUP_BCB, "");
        std::string clear_bcb = android::base::GetProperty(INIT_SVC_CLEAR_BCB, "");
        std::string uncrypt = android::base::GetProperty(INIT_SVC_UNCRYPT, "");
        if (setup_bcb != "running" && clear_bcb != "running" && uncrypt != "running") {
            success = true;
            break;
        }
        sleep(1);
    }
}

static bool setupBcb(const std::string& message) {
    stopService();
    android::base::SetProperty("ctl.start", "setup-bcb");
    sleep(1);
    sockaddr_un un = {};
    un.sun_family = AF_UNIX;
    strlcpy(un.sun_path, UNCRYPT_SOCKET.c_str(), sizeof(un.sun_path));
    unique_fd sockfd;
    sockfd.reset(socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0));
    if(sockfd.get() < 0) {
        ALOGD("Failed to open %s socket: %s", UNCRYPT_SOCKET.c_str(), strerror(errno));
        android::base::SetProperty("ctl.stop", "setup-bcb");
        return false;
    }
    bool success = false;
    for (int retry = 0; retry < SOCKET_CONNECT_MAX_RETRY; retry++) {
        if (connect(sockfd.get(), reinterpret_cast<sockaddr*>(&un), sizeof(sockaddr_un)) != 0) {
            success = true;
            break;
        }
        sleep(1);
    }
    int length = static_cast<int>(message.size());
    int length_out = htonl(length);
    if (!android::base::WriteFully(sockfd.get(), &length_out, sizeof(int))) {
        ALOGE("Failed send message size to socket: [%s], error info: %s", UNCRYPT_SOCKET.c_str(), strerror(errno));
        return false;
    }
    if (!android::base::WriteFully(sockfd.get(), message.data(), length)) {
        ALOGE("Failed send message to socket: [%s], error info: %s", UNCRYPT_SOCKET.c_str(), strerror(errno));
        return false;
    }
    int status;
    if (!android::base::ReadFully(sockfd.get(), &status, sizeof(int))) {
        ALOGE("Failed read status from socket: [%s], error info: %s", UNCRYPT_SOCKET.c_str(), strerror(errno));
        return false;
    }
    if(status == 0) {
        ALOGD("setup-bcb response %d", status);
        success = false;
    }
    android::base::SetProperty("ctl.stop", "setup-bcb");
    return success;
}

static void rebootRecovery() {
    std::string message = "BootFailure\n--reason=BootMonitor";
    if(!setupBcb(message)) {
        ALOGD("Failed to setupBcb");
    }
    property_set("sys.powerctl", "reboot,recovery");
}

bool BootMonitorThread::threadLoop() {
    std::string safemodePath;
    std::string maintenancemodePath;
    std::string trapmodePath;
    std::string bootcntPath;
    std::string logdirPath;
    int trap_time = 0;
    // When the startup fails, the next startup will use incLogCaptureFlag
    // to determine whether the incremental log here is captured
    bool incLogCaptureFlag = false;

    // It means four stages: 0 minutes, 5 minutes, 10 minutes and 15 minutes
    // Capture the log when it has not been started in the 5th、10th and 15th minutes.
    bool bootlevel[4] = { true, false, false, false };

    if (android::base::GetBoolProperty("ro.build.ab_update", false)) {
        logdirPath = RESCUE_UNHEALTHY_BOOT_LOG_PATH;
        bootcntPath = RESCUE_BOOT_COUNT_PATH;
        safemodePath = RESCUE_SAFE_MODE_PATH;
        maintenancemodePath = RESCUE_MAINTENANCE_MODE_PATH;
        trapmodePath = RESCUE_TRAP_MODE_PATH;
    } else {
        logdirPath = UNHEALTHY_BOOT_LOG_PATH;
        bootcntPath = BOOT_COUNT_PATH;
        safemodePath = CACHE_SAFE_MODE_PATH;
        maintenancemodePath = MAINTENANCE_MODE_PATH;
        trapmodePath = CACHE_TRAP_MODE_PATH;
    }

    createLogDirIfNeeded(logdirPath.c_str());

    checkSafeMode(safemodePath);

    checkMaintenanceMode(maintenancemodePath);

    if (checkTrapMode(trapmodePath, trap_time)) {
        long long startTrapTime = uptimeMillis();
        while (true) {
            // seconds
            long long waitTrapTime = (uptimeMillis() - startTrapTime) / 1000;

            ALOGI("(trap mode) boot time: %lld seconds\n", waitTrapTime);

            captureBugreport(logdirPath, TRAP_MODE, waitTrapTime);

            sleep(trap_time);
        }
    }

    int bootCount = readBootCountAndCheck(bootcntPath);
    if (bootCount < 0) {
        ALOGE("exit for invalid boot count.");
        return false;
    } else {
        ALOGI("note boot count: %d", bootCount);
        // 记录开机次数到bootcntPath文件中
        resetBootCount(bootcntPath, bootCount + 1);
    }

    if (bootCount >= BOOT_RERTY_LIMIT) {
        // 开机超过最大尝试次数，先抓bugreport，然后进recovery
        ALOGW("stuck in boot loop, capture bugreport...");
        clearBootCount(bootcntPath);
        captureBugreport(logdirPath, bootCount);

        ALOGE("reboot to recovery");
        rebootRecovery();
        return false;
    }

    long long startTime = uptimeMillis();

    while (true) {
        // seconds
        long long wait_time = (uptimeMillis() - startTime) / 1000;

        // Waiting for 20 minutes did not boot complete, 
        // so capture a bugreport and restart the system.
        if (wait_time >= BOOTMONITOR_TIME_OUT) {
            captureBugreport(logdirPath, bootCount, wait_time);
            ALOGE("Wait boot completed timeout, so reboot system!");
            property_set("sys.powerctl", "reboot");
            return false;
        }

        // Check boot completed
        if (android::base::WaitForProperty(BOOTCOMPLETE_PROP, "1",
                    std::chrono::seconds(ONCE_WAIT_SECONDS))) {
            // Check display boot completed when display monitor is enabled
            std::string bootDisplayCompletedProp =
                    android::base::GetProperty(BOOTDISPLAYCOMPLETE_PROP, "DISABLE");
            if (bootDisplayCompletedProp == "DISABLE" || 
                    android::base::WaitForProperty(BOOTDISPLAYCOMPLETE_PROP, "1", 
                    std::chrono::seconds(ONCE_WAIT_SECONDS))) {
                ALOGW("Boot successfully, so BootMonitor exiting.");
                break;
            }
        }

        ALOGI("boot time: %lld seconds\n", wait_time);

        // Don't capture the log at the first startup.
        // In the case of startup failure, the subsequent log
        // fetching is 20 seconds later than the previous one.
        if (bootCount != 0 && wait_time >= bootCount * BOOTMONITOR_STEP_TIME && !incLogCaptureFlag) {
            ALOGW("Failed to boot system for the %d time, capture bugreport...", bootCount);
            incLogCaptureFlag = true;
            captureBugreport(logdirPath, bootCount, wait_time);
        }

        // Capture a bugreport in 5 minutes, 10 minutes and 15 minutes respectively.
        if (bootlevel[wait_time / BOOTMONITOR_LEVEL_TIME] == false) {
            bootlevel[wait_time / BOOTMONITOR_LEVEL_TIME] = true;
            ALOGW("Failed to boot system for the %d time, capture bugreport...", bootCount);
            captureBugreport(logdirPath, bootCount, wait_time);
        }
    }

    // boot completed
    clearBootCount(bootcntPath);

    return false;
}

void BootMonitorThread::captureBugreport(std::string& topdir, int bootCount, long long wait_time) {
    auto logName = buildLogName(bootCount, wait_time);
    auto logPath = StringPrintf("%s/%s", topdir.c_str(), logName.c_str());

    CaptureLogUtil dump;
    dump.dump_lite_bugreport(topdir.c_str(), logPath.c_str());
    removeDeprecatedLog(topdir, logName);
    ALOGI("Capture lite bugreport complete: %s", logPath.c_str());
}

}
