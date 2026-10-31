#include "common/EventsNodeThread.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "syspressure.events"

namespace android {

namespace syspressure {

#define EVENT_SIZE          (sizeof(struct inotify_event))
#define EVENT_BUF_LEN       (1024*(EVENT_SIZE+16))

#define MAX_DATA_SIZE       16

#define WatchInfo EventsNodeThread::WatchInfo

EventsNodeThread::EventsNodeThread(std::string path) :
        Thread(/*canCallJava*/false) {
    mEventsPath.push_back(path);
    mRunning = true;
    mInotifyFd = 0;
}

EventsNodeThread::EventsNodeThread(Vector<std::string> paths) :
       Thread(/*canCallJava*/false) {
   mEventsPath.appendVector(paths);
   mRunning = true;
   mInotifyFd = 0;
}

EventsNodeThread::~EventsNodeThread() {
}

status_t EventsNodeThread::requestExitAndWait() {
    ALOG(LOG_DEBUG, LOG_TAG, "%s: Not implemented. Use requestExit + join instead",
          __FUNCTION__);
    return INVALID_OPERATION;
}

void EventsNodeThread::requestExit() {
    Mutex::Autolock al(mMutex);

    ALOG(LOG_DEBUG, LOG_TAG, "%s: Requesting thread exit", __FUNCTION__);
    mRunning = false;

    bool rmWatchFailed = false;
    Vector<WatchInfo>::iterator it;
    for (it = mWatchInfos.begin(); it != mWatchInfos.end(); ++it) {

        if (it->watchID != -1 && inotify_rm_watch(mInotifyFd, it->watchID) == -1) {
            ALOG(LOG_DEBUG, LOG_TAG, "%s: Could not remove watch for path %s,"
                  " error: '%s' (%d)",
                 __FUNCTION__, it->path.c_str(), strerror(errno),
                 errno);
            rmWatchFailed = true ;
        } else {
            ALOG(LOG_DEBUG, LOG_TAG, "%s: Removed watch for path s%s",
                __FUNCTION__, it->path.c_str());
        }
    }

    if (rmWatchFailed) { // unlikely
        // Give the thread a fighting chance to error out on the next
        // read
        if (close(mInotifyFd) == -1) {
            ALOG(LOG_DEBUG, LOG_TAG, "%s: close failure error: '%s' (%d)",
                 __FUNCTION__, strerror(errno), errno);
        }
    }

    ALOG(LOG_DEBUG, LOG_TAG, "%s: Request exit complete.", __FUNCTION__);
}

status_t EventsNodeThread::readyToRun() {
    Mutex::Autolock al(mMutex);
    mInotifyFd = -1;
    do {
        ALOG(LOG_DEBUG, LOG_TAG, "%s: Initializing inotify", __FUNCTION__);
        mInotifyFd = inotify_init();
        if (mInotifyFd == -1) {
            ALOG(LOG_DEBUG, LOG_TAG, "%s: inotify_init failure error: '%s' (%d)",
                 __FUNCTION__, strerror(errno), errno);
            mRunning = false;
            break;
        }
        for (size_t i = 0; i < mEventsPath.size(); ++i) {
            if (!addWatch(mEventsPath[i])) {
                mRunning = false;
                break;
            }
        }
    } while(false);

    if (!mRunning) {
        status_t err = -errno;
        if (mInotifyFd != -1) {
            close(mInotifyFd);
        }
        return err;
    }
    return OK;
}

bool EventsNodeThread::threadLoop() {

    // If requestExit was already called, mRunning will be false
    while (mRunning) {
        char buffer[EVENT_BUF_LEN];
        int length = TEMP_FAILURE_RETRY(
                        read(mInotifyFd, buffer, EVENT_BUF_LEN));
        if (length < 0) {
            ALOG(LOG_DEBUG, LOG_TAG, "%s: Error reading from inotify FD, error: '%s' (%d)",
                 __FUNCTION__, strerror(errno),
                 errno);
            mRunning = false;
            break;
        }

        int i = 0;
        while (i < length) {
            inotify_event* event = (inotify_event*) &buffer[i];

            if (event->mask & IN_IGNORED) {
                Mutex::Autolock al(mMutex);
                if (!mRunning) {
                    ALOG(LOG_DEBUG, LOG_TAG, "%s: Shutting down thread", __FUNCTION__);
                    break;
                } else {
                    ALOG(LOG_DEBUG, LOG_TAG, "%s: File was deleted, aborting",
                          __FUNCTION__);
                    mRunning = false;
                    break;
                }
            } else if ((event->mask & IN_CLOSE_WRITE) ||
                        (event->mask & IN_MODIFY)) {
                std::string path = getNodePath(event->wd);

                if (path.empty()) {
                    ALOG(LOG_DEBUG, LOG_TAG, "%s: Got bad path from WD '%d",
                          __FUNCTION__, event->wd);
                } else {
                    std::string content = readFile(path);
                    if (mEventsDataCall != nullptr) {
                        mEventsDataCall(path, content);
                    }
                }

            } else {
                ALOG(LOG_DEBUG, LOG_TAG, "%s: Unknown mask 0x%x",
                      __FUNCTION__, event->mask);
            }
            i += EVENT_SIZE + event->len;
        }
    }
    if (!mRunning) {
        close(mInotifyFd);
        return false;
    }
    return true;
}

void EventsNodeThread::setEventsDataCall(EventsDataCall callback) {
    mEventsDataCall = callback;
}

std::string EventsNodeThread::getNodePath(int wd) const {
    for (size_t i = 0; i < mWatchInfos.size(); ++i) {
        if (mWatchInfos[i].watchID == wd) {
            return mWatchInfos[i].path;
        }
    }
    return "";
}

WatchInfo* EventsNodeThread::getWatchInfo(std::string path) {
    for (size_t i = 0; i < mWatchInfos.size(); ++i) {
        if (mWatchInfos[i].path == path) {
            return (WatchInfo*)&mWatchInfos[i];
        }
    }
    return NULL;
}

bool EventsNodeThread::addWatch(std::string path) {
    int wd = inotify_add_watch(mInotifyFd, path.c_str(), IN_CLOSE_WRITE | IN_MODIFY);
    if (wd == -1) {
        ALOG(LOG_DEBUG, LOG_TAG, "%s: Could not add watch for '%s', error: '%s' (%d)",
             __FUNCTION__, path.c_str(), strerror(errno),
             errno);
        mRunning = false;
        return false;
    }
    ALOG(LOG_DEBUG, LOG_TAG, "%s: Watch added for path=%s, wd='%d'",
          __FUNCTION__, path.c_str(), wd);
    WatchInfo si = { path, wd };
    mWatchInfos.push_back(si);
    return true;
}

bool EventsNodeThread::removeWatch(std::string path) {
    WatchInfo* si = getWatchInfo(path);
    if (!si) return false;
    if (inotify_rm_watch(mInotifyFd, si->watchID) == -1) {

        ALOG(LOG_DEBUG, LOG_TAG, "%s: Could not remove watch for path %s, error: '%s' (%d)",
             __FUNCTION__, path.c_str(), strerror(errno),
             errno);

        return false;
    }
    Vector<WatchInfo>::iterator it;
    for (it = mWatchInfos.begin(); it != mWatchInfos.end(); ++it) {
        if (it->path == path) {
            break;
        }
    }
    if (it != mWatchInfos.end()) {
        mWatchInfos.erase(it);
    }
    return true;
}

std::string EventsNodeThread::readFile(const std::string& filePath) const {
    int fd = TEMP_FAILURE_RETRY(
                open(filePath.c_str(), O_RDONLY, /*mode*/0));
    if (fd == -1) {
        ALOG(LOG_DEBUG, LOG_TAG, "%s: Could not open file '%s', error: '%s' (%d)",
             __FUNCTION__, filePath.c_str(), strerror(errno), errno);
        return "";
    }
    char buffer[MAX_DATA_SIZE] = {0};
    int length;
    length = TEMP_FAILURE_RETRY(read(fd, buffer, sizeof(buffer)));
    close(fd);
    return buffer;
}

} // namespace syspressure

} // namespace android
