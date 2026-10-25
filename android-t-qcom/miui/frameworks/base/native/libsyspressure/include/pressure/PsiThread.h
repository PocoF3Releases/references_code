#pragma once

#include <utils/Thread.h>
#include "common/Common.h"

namespace android {

namespace syspressure {

enum PressureType {
    CPU_PRESSURE_TYPE = 0,
    IO_PRESSURE_TYPE,
    PRESSURE_TYPE_MAX
};

enum PressureLevel {
    PRESSURE_LOW_LEVEL = 0,
    PRESSURE_MEDIUM_LEVEL,
    PRESSURE_CRITICAL_LEVEL,
    PRESSURE_LEVEL_MAX
};

struct STPressureEventInfo {
    int type;
    int level;
    std::function<void(int, int)> handler;
    int fd;
};

class PsiThread : virtual public Thread {
public:
    PsiThread();
    ~PsiThread();
    virtual void startThread();
    void stopThread();
    // PSI
    /**
    * open /proc/pressure fd and write some/full thresholds
    *
    * @return error(<0) success(fd)
    */
    status_t openPsiMonitor(int type, int level);

    /**
    * destory psi node monitoring
    *
    * @param fd node fd
    */
    void destroyPsiMonitor(int fd);

    /**
    * add epoll_ctl event
    * @param fd node fd
    * @param data event data ptr. @link{PressureEventInfo}
    * @return success(STATE_OK) error(event no)
    */
    status_t registerPsiMonitor(int fd, void* data);

    /**
    * delete epoll_ctl event
    * @param fd node fd
    * @return success(STATE_OK) error(event no)
    */
    status_t unRegisterPsiMonitor(int fd);
private:
    virtual bool threadLoop();
private:
    int mEpollFd;
    int mMaxEvents;
    volatile bool mMonitoring;          // guarding only when it's important
    mutable Mutex   mLock;
    Condition       mCondition;
};

} // namespace syspressure

} // namespace android
