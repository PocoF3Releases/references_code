#include <sys/epoll.h>
#include <string.h>
#include <functional>
#include "pressure/PsiThread.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "syspressure.pressure"

namespace android {

namespace syspressure {

#define US_PER_MS                                   (US_PER_SEC / MS_PER_SEC)
#define PSI_WINDOW_SIZE_MS                          3000

#define PRESSURE_FILE_CPU                           "/proc/pressure/cpu"
#define PRESSURE_FILE_IO                            "/proc/pressure/io"

enum PressureStallType {
    PRESSURE_STALL_SOME,
    PRESSURE_STALL_FULL,
    PRESSURE_STALL_TYPE_MAX
};

static const char* sStallTypeName[] = {
        "some",
        "full",
};

struct STPressureThreshold {
    enum PressureStallType stallType;
    int thresholdMS;
};

/**
* /proc/pressure some/full thresholds
*/
static struct STPressureThreshold sPsiThresholds[PRESSURE_TYPE_MAX][PRESSURE_LEVEL_MAX] = {
{// CPU
    { PRESSURE_STALL_SOME, 2200 },    /* 2200ms out of 3sec for partial stall */
    { PRESSURE_STALL_SOME, 0 },      /* not available */
    { PRESSURE_STALL_FULL, 0 },      /* not available */
},
{// IO
    { PRESSURE_STALL_FULL, 0 },    /* not available */
    { PRESSURE_STALL_SOME, 0 },   /* not available */
    { PRESSURE_STALL_FULL, 0 },    /* not available */
},
};

PsiThread::PsiThread() : Thread(false),
                         mMaxEvents(0),
                         mMonitoring(false),
                         mLock("PsiThread::mLock"){
    mEpollFd = epoll_create1(EPOLL_CLOEXEC);
}

PsiThread::~PsiThread() {
}

void PsiThread::startThread() {
    mMonitoring = true;
    mCondition.broadcast();
}

void PsiThread::stopThread() {
    mMonitoring = false;
}

bool PsiThread::threadLoop() {
    struct STPressureEventInfo* handlerInfo;
    struct epoll_event *evt;
    while (mMonitoring) {
        struct epoll_event events[mMaxEvents];
        int i =0;
        int nevents = epoll_wait(mEpollFd, events, mMaxEvents, 5 * 60 * 1000);
        if (!mMonitoring) {
            break;
        }
        if (nevents == -1) {
            if (errno == EINTR)
                continue;
            Utils::milliSleep(1000);
            ALOG(LOG_ERROR,  LOG_TAG, "epoll_wait failed: %s (%d)", strerror(errno), errno);
            continue;
        }
        for (i = 0, evt = &events[0]; i < nevents; ++i, evt++) {
            if ((evt->events & EPOLLHUP) || (evt->events & EPOLLERR)) {
                ALOG(LOG_DEBUG, LOG_TAG, "EPOLLHUP on event #%d", i);
                continue;
            }
            if (evt->data.ptr) {
                handlerInfo = (struct STPressureEventInfo*)evt->data.ptr;
                handlerInfo->handler(handlerInfo->type, handlerInfo->level);
            }
        }
    }
    mCondition.wait(mLock);
    return true;
}

/**
* open /proc/pressure fd and write some/full thresholds
*
* @param type pressure type @link{PressureType}
* @param level pressure level @link{PressureLevel}
* @return error(<0) success(fd)
*/
status_t PsiThread::openPsiMonitor(int type, int level) {
    int fd = -1;
    int res = STATE_UNKNOWN_ERROR;
    char buf[256];
    std::string path;
    do {
        int thresholdUs = sPsiThresholds[type][level].thresholdMS * US_PER_MS;
        if (thresholdUs <= 0) {
            res = STATE_INVALID_OPERATION;
            break;
        }
        PressureStallType stallType = sPsiThresholds[type][level].stallType;
        // open
        switch (type) {
            case CPU_PRESSURE_TYPE:
                path = PRESSURE_FILE_CPU;
                break;
            case IO_PRESSURE_TYPE:
                path = PRESSURE_FILE_IO;
                break;
            default:
                return STATE_BAD_TYPE;
        }
        fd = TEMP_FAILURE_RETRY(open(path.c_str(), O_WRONLY | O_CLOEXEC));
        if (fd < 0) {
            ALOG(LOG_ERROR, LOG_TAG,
                            "No kernel psi monitor support: %s(%d)", strerror(errno), errno);
            res = STATE_PERMISSION_DENIED;
            break;
        }
        //write some/full threshold
        if(stallType == PRESSURE_STALL_SOME ||
                    stallType == PRESSURE_STALL_FULL) {
            res = snprintf(buf, sizeof(buf), "%s %d %d",
                                sStallTypeName[stallType], thresholdUs,
                                (int)(PSI_WINDOW_SIZE_MS * US_PER_MS) );
        } else {
            ALOG(LOG_ERROR, LOG_TAG, "Invalid psi stall type: %d", stallType);
            errno = EINVAL;
            res = STATE_BAD_VALUE;
            break;
        }

        if (res >= (ssize_t)sizeof(buf)) {
            ALOG(LOG_ERROR, LOG_TAG, "%s line overflow for psi stall type '%s'",
                                path.c_str(), sStallTypeName[stallType]);
            errno = EINVAL;
            res = STATE_BAD_VALUE;
            break;
        }

        res = TEMP_FAILURE_RETRY(write(fd, buf, strlen(buf) + 1));
        if (res < 0) {
            ALOG(LOG_ERROR, LOG_TAG, "%s write failed for psi stall type '%s'; errno=%d",
                                path.c_str(), sStallTypeName[stallType], errno);
            break;
        }
        return fd;
    }while(0);
    return res;
}

/**
* destory psi node monitoring
*
* @param fd node fd
*/
void PsiThread::destroyPsiMonitor(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

/**
* add epoll_ctl event
* @param fd node fd
* @param data event data ptr. @link{PressureEventInfo}
* @return success(STATE_OK) error(event no)
*/
status_t PsiThread::registerPsiMonitor(int fd, void* data) {
    struct epoll_event epev = {};
    epev.events = EPOLLPRI | EPOLLWAKEUP | EPOLLERR;
    epev.data.ptr = data;
    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &epev)) {
        ALOG(LOG_ERROR, LOG_TAG, "Could not add fd to epoll instance: %s", strerror(errno));
        return -errno;
    }
    mMaxEvents ++;
    return STATE_OK;
}

/**
* delete epoll_ctl event
* @param fd node fd
* @return success(STATE_OK) error(event no)
*/
status_t PsiThread::unRegisterPsiMonitor(int fd) {
    if (epoll_ctl(mEpollFd, EPOLL_CTL_DEL, fd, nullptr)) {
        ALOG(LOG_ERROR, LOG_TAG, "Could not remove fd from epoll instance: %s, fd %d",
                                strerror(errno), fd);
        return -errno;
    }
    mMaxEvents --;
    return STATE_OK;
}
} // namespace syspressure

} // namespace android