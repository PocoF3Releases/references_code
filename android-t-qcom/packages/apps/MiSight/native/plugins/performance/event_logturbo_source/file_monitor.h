#ifndef _FILE_MONITOR_
#define _FILE_MONITOR_

#define LOG_TURBO_MONITOR "/dev/mi_exception_log"
#define MAX_EPOLL_EVENTS 64

namespace android {

class FileMonitor {
    public:
        int openExceptionLogFd();
        int registerExceptionLogMonitor(int exceptionLogFd);
        void unregisterExceptionLogMonitor(int epollFd, int exceptionLogFd);
        void destoryExceptionLogFd(int exceptionLogFd);

};
}//namespace android
#endif /*_FILE_MONITOR_*/
