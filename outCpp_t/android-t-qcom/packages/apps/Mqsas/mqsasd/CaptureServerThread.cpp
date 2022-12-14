//
// Created by tanxiaoyan on 19-4-8.
//
#define LOG_TAG "CaptureServerThread"

#include <string>
#include <vector>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <cutils/log.h>
#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>
#include "android-base/file.h"
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>

#include "CaptureServerThread.h"
#include "BootMonitorThread.h"
#include "utils.h"

using namespace android;
using android::base::GetProperty;
using android::base::GetBoolProperty;
using android::base::StringPrintf;
using android::base::unique_fd;
using android::base::ReadFully;
using android::base::WriteFully;

#define SOCKET_NAME "mqsasd"
#define MAX_BUFFER_SIZE (4096-1)

CaptureServerThread::CaptureServerThread() {
    const char *where = GetBoolProperty("ro.build.ab_update", false)
                    ? RESCUE_BOOT_FAILED_LOG_PATH
                    : BOOT_FAILED_LOG_PATH;
    // rwx-rwx-rx
    mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    create_dir_if_needed(where, mode);
    chown(where, AID_ROOT, AID_SYSTEM);
    chmod(where, mode);

    mTopDir = where;
}

bool CaptureServerThread::threadLoop() {
    const char *response = "finish";
    unique_fd sockfd;

    sockfd.reset(android_get_control_socket(SOCKET_NAME));
    if (sockfd == -1) {
        ALOGE("Failed to get socket from environment: %s\n", strerror(errno));
        return false;
    }
    fcntl(sockfd.get(), F_SETFD, FD_CLOEXEC);

    if (listen(sockfd.get(), 5) == -1) {
        ALOGE("Listen on socket failed: %s\n", strerror(errno));
        return false;
    }

    for (;;) {
        struct sockaddr addr;
        socklen_t addrlen = sizeof(addr);

        unique_fd connfd(accept(sockfd.get(), &addr, &addrlen));
        if (connfd == -1) {
            ALOGE("Accept failed: %s\n", strerror(errno));
            continue;
        }
        fcntl(connfd, F_SETFD, FD_CLOEXEC);

        char buffLen[4] = {0};
        TEMP_FAILURE_RETRY(read(connfd, buffLen, sizeof(buffLen)));

        int size = charArr2Int(buffLen);
        ALOGD("content size :%d", size);

        char buf[size + 1];
        if (!ReadFully(connfd, buf, size)) {
            ALOGE("read json failed: %s\n", strerror(errno));
            continue;
        }
        buf[size] = '\0';
        ALOGD("mqsasd dump begin for dump request: %s\n", buf);

        runCaptureLog(buf);

        if (!WriteFully(connfd, response, strlen(response))) {
            ALOGE("Write response failed: %s\n", strerror(errno));
            continue;
        }
    }

    return false;
}

int CaptureServerThread::charArr2Int(char b[]) {
    return (b[3] & 0xff) |
           (b[2] & 0xff <<  8) |
           (b[1] & 0xff <<  16) |
           (b[0] & 0xff <<  24);
}

std::string CaptureServerThread::createLogPath(const char* type) {
    // "{mTopDir}/BootFailure" or "{mTopDir}/RescuePartyLog"
    std::string type_dir = StringPrintf("%s/%s", mTopDir.c_str(), type);
    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(type_dir.c_str()), closedir);
    if (!dir) {
        mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
        create_dir_if_needed(type_dir.c_str(), mode);
        chown(type_dir.c_str(), AID_ROOT, AID_SYSTEM);
        chmod(type_dir.c_str(), mode);
    }

    time_t now = time(NULL);
    char date[64] = "";
    // use '-' replace ':' because widnwos does not support ':' naming
    strftime(date, sizeof(date), "%Y-%m-%d_%H-%M-%S", localtime(&now));

    std::string device = GetProperty("ro.product.device", "unknown");
    std::string miui_version = GetProperty("ro.build.version.incremental", "unknown");
    // - "{mTopDir}/BootFailure/BootFailure_venus_21.2.3_2021-03-09_12-35-59.zip"
    // - "{mTopDir}/RescuePartyLog/RescuePartyLog_venus_21.2.3_21-03-09_12-35-59.zip"
    return StringPrintf("%s/%s/%s_%s_%s_%s",
                mTopDir.c_str(), type, type, device.c_str(), miui_version.c_str(), date);
}

/**
 * @currLog: current log storage path
 *
 * Find the latest log in each type.
 */
bool CaptureServerThread::checkAndSaveLog(std::string& currLog) {
    android::base::unique_fd fd(TEMP_FAILURE_RETRY(open(currLog.c_str(),
                            O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)));
    if (fd == -1) {
        ALOGW("Faile to open file: %s, %s", currLog.c_str(), strerror(errno));
        return false;
    }

    struct stat st = {};
    if (fstat(fd, &st) == -1) {
        ALOGW("Faile to read file stat: %s, %s", currLog.c_str(), strerror(errno));
        return false;
    }

    if (st.st_size < 1024) {
        ALOGW("log too small, ready to delete.");
        return false;
    }

    return true;
}

void CaptureServerThread::runCaptureLog(const char* type) {
    std::string logPath = createLogPath(type);
    ALOGD("mqsasd dump to: %s\n", logPath.c_str());

    /*
     * 这个来源于init里面拦截进入Recovery的场景，一般有两种场景:
     * 1. RescueParty问题
     *  - 该调用是同步阻塞的，不能抓取完整Bugreport等长耗时操作，因为PowerMs
     * 执行reboot recovery后会等待20s，如果超时则直接shutdown.
     *  - 如果再次通过dumpstate抓取bugreport可能导致死锁，所以通过命令抓取
     *
     * 2. Init里面fbe相关异常
     *  - 这种情况下boot进入init后很快就会crash，还没有启动Zygote。在抓日志时
     *  间上要求更短。
     */
    std::vector<CaptureLogUtil::DumpAction> actions = {
        {
            .title = "UPTIME",
            .cmd = {"uptime", "-p"},
            .timeout = 1,
        },
        {
            .title = "MAIN,SYSTEM,EVENTS,CRASH LOGS",
            .cmd = {"logcat", "-b", "main,system,events,crash", "-v", "uid", "-d", "*:v"},
            .timeout = 10,
        },
        {
            .title = "DROPBOX SYSTEM SERVER CRASHES",
            .cmd = {"dumpsys", "dropbox", "-p", "system_server_crash"},
            .timeout = 1,
        },
        {
            .title = "DROPBOX SYSTEM SERVER WATCHDOG",
            .cmd = {"dumpsys", "dropbox", "-p", "system_server_watchdog"},
            .timeout = 1,
        },
        {
            .title = "DROPBOX SYSTEM APP CRASHES",
            .cmd = {"dumpsys", "dropbox", "-p", "system_app_crash"},
            .timeout = 1,
        },
        {
            .title = "SYSTEM PROPERTIES",
            .cmd = {"getprop"},
            .timeout = 1,
        },
    };

    // RescueParty for persistent app crash, just add 'dumpsys package xxx' info
    // Note: the prop name should sync with RescueParty.java#executeRescueLevelInternal
    std::string failed_package = GetProperty("sys.rescue_party_failed_package", "");
    if (failed_package != "") {
        actions.push_back({"DUMPSYS PACKAGE", {"dumpsys", "package", failed_package}, 1});
    }

    CaptureLogUtil dump;
    std::string zip_dir = StringPrintf("%s/%s/", mTopDir.c_str(), type);
    dump.dump_for_actions(zip_dir.c_str(), logPath.c_str(), actions);

    // only save lastest RescueParty or BootFailure log
    std::string rescuePartyPrefix = "RescuePartyLog";
    std::string bootFailurePrefix = "BootFailure";
    logPath.append(".zip");
    auto exclusion_predicate = [&](const char* name, bool /*is_dir*/) {
        const std::string base_name(name);

        // not 'RescueParty' and 'BootFailure' prefix files need to be retained
        if (base_name.find(rescuePartyPrefix) != 0 && base_name.find(bootFailurePrefix) != 0) {
            return true;
        }

        // save the latest log in each type
        if (logPath.find(base_name) != std::string::npos) {
            return true;
        }

        return false;
    };

    if (checkAndSaveLog(logPath)) {
        delete_dir_contents(zip_dir.c_str(), false, exclusion_predicate);
    } else if (remove(logPath.c_str())) {
        ALOGE("remove path %s failed! info: %s\n", logPath.c_str(), strerror(errno));
    }

    ALOGD("mqsasd dump end for dump request\n");
}
