#define LOG_TAG "LOG_TURBO_FILEMONITOR"

#include <errno.h>
#include <fcntl.h>
#include <cutils/fs.h>
#include <stdio.h>
#include <sys/epoll.h>
#include "log.h"
#include "file_monitor.h"

namespace android {

int FileMonitor::openExceptionLogFd() {
    int logTurboFd;
    logTurboFd = TEMP_FAILURE_RETRY(open(LOG_TURBO_MONITOR, O_RDONLY | O_CLOEXEC));
    if (logTurboFd < 0) {
        MISIGHT_LOGE("No kernel exception log monitor support (errno=%d)", errno);
        return -1;
    }

    return logTurboFd;
}

int FileMonitor::registerExceptionLogMonitor(int exceptionLogFd) {
    int epollFd;
    int res;
    struct epoll_event epev;

    epollFd = epoll_create(MAX_EPOLL_EVENTS);
    if (epollFd == -1) {
        MISIGHT_LOGE("epoll_create failed (errno=%d)", errno);
        goto err;
    }

    epev.events = EPOLLPRI | EPOLLWAKEUP;
    res = epoll_ctl(epollFd, EPOLL_CTL_ADD, exceptionLogFd, &epev);
    if (res < 0) {
        MISIGHT_LOGE("epoll_ctl for exception log monitor failed; errno=%d", errno);
        goto err;
    }

    return epollFd;

err:
    close(exceptionLogFd);
    return -1;
}

void FileMonitor::unregisterExceptionLogMonitor(int epollFd, int exceptionLogFd) {
    if (epollFd != -1 && exceptionLogFd != -1) {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, exceptionLogFd, NULL);
    }
}

void FileMonitor::destoryExceptionLogFd(int exceptionLogFd) {
   if (exceptionLogFd != -1) {
       close(exceptionLogFd);
   }
}

}//namespace android
