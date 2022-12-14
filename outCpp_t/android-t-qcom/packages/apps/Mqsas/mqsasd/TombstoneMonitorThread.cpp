/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#define LOG_TAG "TombstoneMonitor"

#include "TombstoneMonitorThread.h"
#include "utils.h"

#include <iostream>
#include <cstring>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <cutils/log.h>
#include <android/log.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android-base/strings.h>
#include <android-base/stringprintf.h>
#include <android-base/properties.h>
#include <private/android_filesystem_config.h>

#define TOMBSTONE_PREFIX    "tombstone_"
#define TOMBSTONE_PB        ".pb"
#define BUFF_SIZE           4096

namespace android {

inline bool TombstoneMonitorThread::isFile(const std::string& path) {
    std::unique_ptr<FILE, int (*)(FILE*)> fp(fopen(path.c_str(), "r"), fclose);
    if (fp.get() == nullptr) {
        ALOGW("open file [%s] error: %s", path.c_str(), strerror(errno));
        return false;
    }
    struct stat fileInfo;
    if (fstat(fileno(fp.get()), &fileInfo)) {
        ALOGE("Failed to fstat file: %s, %s", path.c_str(), strerror(errno));
    }
    if (!S_ISREG(fileInfo.st_mode)) {
        return false;
    } else {
        return true;
    }
}

bool TombstoneMonitorThread::copy(const std::string& src, const std::string& dest) {
    if (isFile(src)) {
        if (copy_file(src.c_str(), dest.c_str())) {
            ALOGE("Faile to copy file: %s, %s", src.c_str(), strerror(errno));
            return false;
        }
        chown(dest.c_str(), AID_SYSTEM, AID_SYSTEM);
        chmod(dest.c_str(), S_IRWXU | S_IRWXG);
        return true;
    }
    return false;
}

bool TombstoneMonitorThread::threadLoop() {
    ALOGI("Tombstone Monitor is Start!");
    int watcher = 0;
    int errorCount = 0;

    std::string monitorDir = android::base::GetProperty(TOMBSTONE_MONITOR_DIR_PROP, TOMBSTONE_MONITOR_DIR);
    std::string copyToDir = android::base::GetProperty(TOMBSTONE_COPY_TO_DIR_PROP, TOMBSTONE_COPY_TO_DIR);

    // wait for post-fs-data finish and create new dir
    while (access(copyToDir.c_str(), F_OK)) {
        if (errorCount++ > 60) {
            ALOGE("wait %s timeout, so exit: %s", copyToDir.c_str(), strerror(errno));
            return false;
        }
        // wait create /data/miuilog/stability folder finish
        sleep(1);
    }

    // wait set policy for /data/miuilog/stability/nativecrash/tombstones/ successed!
    sleep(5);

    android::base::unique_fd monitorfd(TEMP_FAILURE_RETRY(inotify_init()));
    if (-1 == monitorfd) {
        ALOGE("inotify_init error: %s", strerror(errno));
        return false;
    }

    // add dir to watch
    watcher = inotify_add_watch(monitorfd, monitorDir.c_str(), IN_ALL_EVENTS);
    if (-1 == watcher) {
        ALOGE("inotify_add_watch error: %s", strerror(errno));
        return false;
    }
    std::unique_ptr<FILE, int (*)(FILE*)> fp(android::base::Fdopen(std::move(monitorfd), "r"), fclose);
    if (nullptr == fp.get()) {
        ALOGE("fdopen error: %s", strerror(errno));
        return false;
    }

    struct inotify_event ie;
    int read_len = 0;
    while (true) {
        // block here until the tombstones folder changes
        read_len = fread(&ie, sizeof(ie), 1, fp.get());
        if (read_len <= 0) {
            ALOGW("read error %d:%s\n", errno, strerror(errno));
            break;
        }
        // ALOGW("%d, %0x, %d\n", ie.wd, ie.mask, ie.len);
        if (ie.len > 0) {
            char name[ie.len];
            read_len = fread(name, ie.len, 1, fp.get());
            // skip other file
            if (read_len > 0
                && std::string(name).find(TOMBSTONE_PREFIX) != std::string::npos
                && std::string(name).find(TOMBSTONE_PB) == std::string::npos) {
                if (ie.mask & IN_DELETE) {
                    ALOGW("Detected deletd file: %s", name);
                    std::string removeFileName = copyToDir + name;
                    if (remove(removeFileName.c_str())) {
                        ALOGE("remove path %s failed! info: %s\n", removeFileName.c_str(), strerror(errno));
                    }
                } else if (ie.mask & (IN_CREATE | IN_MODIFY)) {
                    ALOGW("Detected creat or modify file: %s", name);
                    size_t retryCount = 0;
                    std::string srcPath = monitorDir + name;
                    std::string destPath = copyToDir + name;
                    while (retryCount < 3 && !copy(srcPath, destPath)) {
                        retryCount++;
                    }
                    if (retryCount >= 3) {
                        ALOGE("Faile to copy file: %s, %s !", srcPath.c_str(), strerror(errno));
                    }
                // skip other event.
                } else {
                    ALOGV("Skip detected event: %d  filename: %s", ie.mask, name);
                }
            }
        }
    }

    ALOGE("Listening for events stopped.");

    return false;
}

}
