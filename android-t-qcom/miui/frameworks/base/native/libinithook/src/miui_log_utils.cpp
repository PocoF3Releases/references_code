#include "miui_log_utils.h"

#include <stdio.h>
#include <sys/klog.h>
#include <stdlib.h>
#include <time.h>
#include <map>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <cutils/sockets.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/klog.h>
#include <sys/stat.h>
#include <private/android_filesystem_config.h>
#include <android-base/logging.h>
#include <android-base/file.h>
#include <android-base/unique_fd.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/properties.h>
#include <bootloader_message/bootloader_message.h>
#include <fscrypt/fscrypt.h>
#include "service.h"
#include "service_list.h"
#include "util.h"

using android::base::GetBoolProperty;
using android::base::WriteStringToFd;
using android::base::StringPrintf;
using android::base::unique_fd;
using android::base::WriteFully;
using android::base::ReadFully;

namespace android {
namespace init {

static std::string get_date_time() {
    time_t seconds;
    struct tm * t_tm;
    std::string time_str = "UNKNOWN TIME";

    if (time(&seconds) != -1 && (t_tm = localtime(&seconds)) != NULL) {
        time_str = StringPrintf("%04d-%02d-%02d-%02d:%02d:%02d", t_tm->tm_year + 1900,
                t_tm->tm_mon + 1, t_tm->tm_mday, t_tm->tm_hour,
                t_tm->tm_min, t_tm->tm_sec);
    }
    return time_str;
}

static bool write_str_to_file(const std::string& buf) {
    if (buf.size() <= 0) return false;

    const char* dirpath = GetBoolProperty("ro.build.ab_update", false)
                            ? RESUCE_LOG_DIR : CACHE_LOG_DIR;
    struct stat st;
    int rc = stat(dirpath, &st);
    if (rc != 0 && errno == ENOENT) {
        rc = mkdir(dirpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (rc != 0) {
            LOG(ERROR) << "mkdir " << dirpath << " failed.";
            return false;
        }
        chown(dirpath, AID_ROOT, AID_SYSTEM);
        chmod(dirpath, 0775);
    }

    std::string logpath = StringPrintf("%s/%s", dirpath, KMSG_FILE);
    int flags = O_WRONLY | O_CREAT | O_CLOEXEC | O_BINARY;
    android::base::unique_fd fd(open(logpath.c_str(), flags, 0666));
    if (fd.get() == -1) {
        LOG(ERROR) << "failed open:" << logpath.c_str() << strerror(errno);
        return false;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if ((file_size + buf.size()) >= MAX_KMSG_SIZE) {
        flags |= O_TRUNC;
        fd.reset(open(logpath.c_str(), flags, 0666));
        if (fd.get() == -1) {
            LOG(ERROR) << "failed trunc open:" << logpath.c_str() << strerror(errno);
            return false;
        }

        std::string info_str = "android-kmsg.txt file too large, trunc it.\n";
        WriteStringToFd(info_str, fd);
    }

    if (!WriteStringToFd(buf, fd)) {
        LOG(ERROR) << "write log failed to " << logpath.c_str();
        return false;
    }

    syncfs(fd);
    return true;
}

static void save_kmsg() {
    std::string log_str = "------ KERNEL LOG START at " + get_date_time() + " ------\n\n";

    int size = klogctl(KLOG_SIZE_BUFFER, NULL, 0);
    if (size <= 0) {
        log_str += StringPrintf("klogctl get size failed.\n");
    } else {
        std::string buf(size, 0);
        int retval = klogctl(KLOG_READ_ALL, &buf[0], size);

        if (retval <= 0) {
            log_str += "klogctl read buffer failed.\n";
        } else  {
            buf.resize(retval);
            log_str += buf;
        }
    }

    log_str += "\n------ KERNEL LOG END at " + get_date_time() + " ------\n\n\n";

    if(!write_str_to_file(log_str)) {
        LOG(INFO) << "save kernel log failed.";
    }
}

static bool save_logcat(const char *buf) {
    std::string errstr;

    Service* mqsasd = ServiceList::GetInstance().FindService("mqsasd");
    if (mqsasd == nullptr || !mqsasd->IsRunning())  {
        LOG(ERROR) << "service mqsasd is unavailable.";
        return false;
    }

    unique_fd sockfd(socket_local_client(
                MQSASD_SOCKET, ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM));
    if (sockfd == -1) {
        errstr = StringPrintf(
                "%s, failed to connect to socket mqsasd : %s", TAG, strerror(errno));
        write_str_to_file(errstr);
        return false;
    }

    if (buf) {
        char len[4] = {0};
        int buf_size = strlen(buf);

        for(int i = 0; i<4; i++) {
            len[i] += ((buf_size >> 8 * (4 - i - 1)) & 0xFF);
        }

        struct timeval tv = {
            .tv_sec = 1,
            .tv_usec = 0,
        };
        if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
            errstr = StringPrintf("%s failed to set send timeout: %s\n",
                                  TAG, strerror(errno));
            return false;
        }

        tv.tv_sec = 20;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
            errstr = StringPrintf("%s failed to set rcv timeout: %s\n",
                                  TAG, strerror(errno));
            return false;
        }

        if (!WriteFully(sockfd, len, sizeof(len))) {
            errstr = StringPrintf("%s, failed to send the size of request body: %s\n",
                                  TAG, strerror(errno));
            write_str_to_file(errstr);
            return false;
        }

        if (!WriteFully(sockfd, buf, buf_size)) {
            errstr = StringPrintf("%s, failed to send dump request to mqsasd: %s\n",
                                  TAG, strerror(errno));
            write_str_to_file(errstr);
            return false;
        }

        char response[7] = {'\0'};
        if (!ReadFully(sockfd, response, sizeof(response)-1)
                || strncmp("finish", response, sizeof(response)-1) != 0) {
            errstr = StringPrintf("%s, failed to receive response from mqsasd: %s\n",
                                  TAG, strerror(errno));
            write_str_to_file(errstr);
            return false;
        }
    }

    return true;
}

void save_system_time_file() {
    std::string time_string = get_date_time();
    const char* filepath = GetBoolProperty("ro.build.ab_update", false)
                            ? RESCUE_TIME_FILE : CACHE_TIME_FILE;
    // 1. Create recovery last_extension_parameters file and if file exit so
    int flags = O_RDWR | O_CREAT;
    android::base::unique_fd fd(open(filepath, flags, 0666));
    if (fd.get() == -1) {
        LOG(ERROR) << "failed open:" << filepath << strerror(errno);
        return;
    }
    std::string line;
    std::ifstream recoveryfile(filepath);
    std::map<std::string, std::string> recoveryExtensionParameters;
    while (getline(recoveryfile, line)) {
        std::vector<std::string> parameter = android::base::Split(line, "=");
        if (parameter.size() != 2) continue;
        recoveryExtensionParameters[parameter[0]]= parameter[1];
    }

    // 2. Set system_time for recoveryExtensionParameters
    recoveryExtensionParameters[EXTENSION_PARAMETERS_SYSTEM_TIME] = time_string;

    // 3. Write parameters to file
    for (auto i = recoveryExtensionParameters.begin(); i != recoveryExtensionParameters.end(); ++i) {
        if (!WriteStringToFd( i->first + "=" + i->second + "\n", fd)) {
            LOG(ERROR) << "Write parameters to config file failed for " << filepath;
            return;
        }
    }
    syncfs(fd);
}

void save_reboot_log() {
    bootloader_message boot = {};
    std::string log_string = "\n---------- " + get_date_time() + " -----\n";

    if (std::string err; !read_bootloader_message(&boot, &err)) {
        log_string += StringPrintf("read bootloader message failed: %s.\n", err.c_str());
        log_string += "------------------------------------\n\n";
        write_str_to_file(log_string);
        return;
    }

    if (boot.command[0] != 0) {
        std::string command = std::string(boot.command, sizeof(boot.command));
        boot.recovery[sizeof(boot.recovery) - 1] = '\0';
        std::string value = std::string(boot.recovery, sizeof(boot.recovery));

        log_string += StringPrintf("Boot command: %s\n", command.c_str());
        log_string += StringPrintf("Boot value: %s", value.c_str());
        log_string += "------------------------------------\n\n";
        write_str_to_file(log_string);

        /*
           --wipe_data reason:
           --prompt_and_wipe_data reasons:
                --reason=RescueParty
                --reason=enablefilecrypto_failed
                --reason=init_user0_failed
                --reason=set_policy_failed
                --reason=fs_mgr_mount_all
           */
        if (strcmp(command.c_str(), "boot-recovery") == 0) {
            if (value.find("RescueParty") != std::string::npos) {
                save_kmsg();
                save_logcat("RescuePartyLog");
            } else if (value.find("wipe_data") != std::string::npos) {
                save_kmsg();
                save_logcat("BootFailure");
            }
        }
    } else if (android::base::GetProperty("sys.attempting_reboot", "false") == "true") {
        log_string += StringPrintf("Boot command: reboot,RescueParty\n");
        log_string += StringPrintf("Boot reason: RescueParty,LEVEL_WARM_REBOOT\n");
        log_string += "------------------------------------\n\n";
        write_str_to_file(log_string);
        save_kmsg();
        save_logcat("RescuePartyLog");
    } else if (boot.recovery[0] == 0) {
        save_kmsg();
    }
}

static void reboot_into_recovery(const std::vector<std::string>& options) {
    std::string err;
    if (!write_bootloader_message(options, &err)) {
        write_str_to_file("Failed to set bootloader message" + err);
    }
    trigger_shutdown("reboot,recovery");
}

/* 
 * -- "enablefilecrypto_failed"
 * -- "init_user0_failed"
 * -- "set_policy_failed"
 */
void monitor_boot_failed(const std::string& reboot_reason) {
    if (!android::base::GetBoolProperty("ro.debuggable", false)) {
        return;
    }

    if (!fscrypt_is_native()) {
        return;
    }

    static std::string monitor_path = StringPrintf("%s/%s",
            GetBoolProperty("ro.build.ab_update", false)
            ? RESUCE_LOG_DIR : CACHE_LOG_DIR, "monitor_crash");

    unique_fd fd(open(monitor_path.c_str(), O_RDWR|O_CLOEXEC, 0));
    if (fd.get() == -1) {
        // write_str_to_file(StringPrintf("failed open for: %s\n", strerror(errno)));
        return;
    }

    std::string content;
    if (!android::base::ReadFdToString(fd, &content)) {
        // write_str_to_file(StringPrintf("failed to read crash file: %s\n", strerror(errno)));
        return;
    }

    if (content.back() == '\n') { content.pop_back(); }

    write_str_to_file("[BootMonitor] read monitor crash '" + content + "'\n");

    if (content == reboot_reason) {
        if (!android::base::RemoveFileIfExists(monitor_path, nullptr)) {
            write_str_to_file("failed to remove monitor_path\n");

            fd.reset(TEMP_FAILURE_RETRY(open(monitor_path.c_str(), O_RDWR|O_CLOEXEC|O_TRUNC, 0)));
            if (fd == -1) {
                write_str_to_file(StringPrintf("failed write open for: %s\n", strerror(errno)));
                return;
            }
            if (!android::base::WriteStringToFd("", fd)) {
                write_str_to_file(StringPrintf("failed to write for: %s\n", strerror(errno)));
                return;
            }
        }

        write_str_to_file("[BootMonitor] reboot into recovery\n");
        reboot_into_recovery({"--prompt_and_wipe_data", "--reason=" + reboot_reason});
    }
}

}  // namespace init
}  // namespace android
