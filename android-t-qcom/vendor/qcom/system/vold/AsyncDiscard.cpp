/*
 * Copyright (C) Xiaomi Inc.
 *
 */
#include "VoldImpl.h"

#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include <thread>
#include <sstream>
#include <mutex>

#include <android-base/chrono_utils.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

using android::base::WriteStringToFile;
using android::base::ReadFileToString;
using android::base::Timer;

// true: trim thread is running
// false: trim thread is non-existent
static bool trimState = false;
static std::mutex trimStateMutex;
static std::thread::id trimThreadId;
static bool stopped = false;

static const int PENDING_DISCARD_THRESHOLD = 128;
static const int PENDING_DISCARD_TIMEOUT_SEC = 420;
static std::condition_variable cv_abort_discard;
static std::mutex cv_m;

static int startUrgentGc(const std::string& path) {
    LOG(DEBUG) << "Start GC In Discard on " << path;
    if (!WriteStringToFile("1", path + "/gc_urgent")) {
        PLOG(WARNING) << "Start GC In Discard failed on " << path;
    }
    return android::OK;
}

static int stopUrgentGc(const std::string& path) {
    LOG(DEBUG) << "Stop GC In Discard on " << path;
    if (!WriteStringToFile("0", path + "/gc_urgent")) {
        PLOG(WARNING) << "Stop GC In Discard failed on " << path;
    }
    return android::OK;
}

void thread_exit_handler(int sig) {
    stopped = true;
}

bool VoldImpl::ShouldAbort() {
        return stopped;
}

void VoldImpl::RecordTrimStart() {
        trimThreadId = std::this_thread::get_id();
        std::stringstream sin;
        sin << trimThreadId;
        unsigned long long tid = std::stoull(sin.str());
        LOG(DEBUG) << "current trim thread id is " << tid;

        /*
        * Register a SIGUSR1 signal handle function to stop trim thread if
        * trigger the SCREEN_ON event when trim thread is running
        */
        struct sigaction actions;
        memset(&actions, 0, sizeof(actions));
        sigemptyset(&actions.sa_mask);
        actions.sa_flags = 0;
        actions.sa_handler = thread_exit_handler;
        sigaction(SIGUSR1, &actions, NULL);

        // Reset stopped flag maybe set by signal handler
        stopped = false;

        trimStateMutex.lock();
        trimState = true;
        trimStateMutex.unlock();
}

void VoldImpl::RecordTrimFinish() {
        trimStateMutex.lock();
        trimState = false;
        trimStateMutex.unlock();
}

void VoldImpl::StopTrimThread() {
        std::stringstream sin;
        sin << trimThreadId;
        unsigned long long tid = std::stoull(sin.str());

        trimStateMutex.lock();
        if (trimState == true) {
                LOG(DEBUG) << "Terminating the trim thread: " << tid;
                int kill_rc = pthread_kill((pthread_t)tid, SIGUSR1);
                if (kill_rc == 0) {
                        LOG(DEBUG) << "Succeed in send SIGUSR1 signal";
                } else {
                        LOG(DEBUG) << "Error in terminating trim thread";
                }
                trimState = false;
                cv_abort_discard.notify_one();
        } else {
                LOG(DEBUG) << "The trim thread is not running";
        }
        trimStateMutex.unlock();
}

void VoldImpl::RunUrgentGcIfNeeded(const std::list<std::string>& paths) {
    std::unique_lock<std::mutex> lk(cv_m, std::defer_lock);
    bool stop = false, aborted = false;
    Timer timer;
    std::string pending_discard;
    std::list<std::string> paths_gcurgent;

    for (const auto& path : paths) {
        if (access((path + "/pending_discard").c_str(), F_OK) != 0) {
            PLOG(WARNING) << "Can't find the node of pending_discard" << path;
            continue;
        }
        paths_gcurgent.push_back(path);
    }

    if (paths_gcurgent.empty()) {
            return;
    }

    for (const auto& path_gcurgent : paths_gcurgent) {
        if (stop || aborted)
            break;

        startUrgentGc(path_gcurgent);

        while (!aborted) {
            if (!ReadFileToString(path_gcurgent + "/pending_discard", &pending_discard)) {
                PLOG(WARNING) << "Reading pending_discard failed in " << path_gcurgent;
                stop = true;
                break;
            }

            if (std::stoi(pending_discard) < PENDING_DISCARD_THRESHOLD) {
                LOG(INFO) << "less than PENDING_DISCARD_THRESHOLD";
                break;
            }

            if (timer.duration() >= std::chrono::seconds(PENDING_DISCARD_TIMEOUT_SEC)) {
                stop = true;
                LOG(WARNING) << "Pending_discard timeout";
                break;
            }

            lk.lock();
            aborted = cv_abort_discard.wait_for(lk, 10s, [this] { return ShouldAbort();});
            if (aborted) {
               LOG(WARNING) << "aborting idle maintenance because of screen on";
            }
            lk.unlock();
        }

        stopUrgentGc(path_gcurgent);
    }
}
