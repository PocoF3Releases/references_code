#include <sys/epoll.h>
#include <string.h>
#include <functional>
#include "pressure/PressureMonitor.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "syspressure.pressure"

namespace android {

namespace syspressure {


static struct STPressureEventInfo sPressureEventInfo[PRESSURE_TYPE_MAX][PRESSURE_LEVEL_MAX];

PressureMonitor::PressureMonitor() : mInit(false),
            mpParseCpuPressure(std::make_shared<ParseCpuPressure>()),
            mLock("PressureMonitor::mLock"){
    mpPsiThread = new PsiThread();
    mpPsiThread->run("PsiThread");
}

PressureMonitor::~PressureMonitor() {
    if (mInit) {
        destory();
    }
}

status_t PressureMonitor::init() {
    ALOG(LOG_DEBUG, LOG_TAG, "monitor start");
    Mutex::Autolock _l(mLock);
    if (!mInit) {
        mInit = true;
        for (int type = 0; type < PRESSURE_TYPE_MAX; type++) {
            for (int level = 0; level < PRESSURE_LEVEL_MAX; level++) {
                initPsiMonitor(type, level);
            }
        }
        mpPsiThread->startThread();
    }
    return STATE_OK;
}

/**
* unregister psi and close event fd
*/
void PressureMonitor::destory() {
    ALOG(LOG_DEBUG, LOG_TAG, "monitor end");
    Mutex::Autolock _l(mLock);
    if (mInit) {
        mpPsiThread->stopThread();
        for (int type = 0; type < PRESSURE_TYPE_MAX; type++) {
            for (int level = 0; level < PRESSURE_LEVEL_MAX; level++) {
                int fd = sPressureEventInfo[type][level].fd;
                if(fd >= 0) {
                    mpPsiThread->unRegisterPsiMonitor(fd);
                    mpPsiThread->destroyPsiMonitor(fd);
                    sPressureEventInfo[type][level].fd = -1;
                }
            }
        }
        mInit = false;
    }
}

/**
* initialization psi monitoring
*
* @param type pressure type @link{PressureType}
* @param level pressure level @link{PressureLevel}
* @return error(false) success(true)
*/
status_t PressureMonitor::initPsiMonitor(int type, int level) {
    sPressureEventInfo[type][level].fd = -1;
    int fd = mpPsiThread->openPsiMonitor(type, level);
    if (fd < 0) {
        return fd;
    }
    int res = mpPsiThread->registerPsiMonitor(fd, &sPressureEventInfo[type][level]);
    if ( res != STATE_OK) {
        mpPsiThread->destroyPsiMonitor(fd);
        return res;
    }
    sPressureEventInfo[type][level].handler = std::bind(&PressureMonitor::eventHandler, this,
                                                    std::placeholders::_1, std::placeholders::_2);
    sPressureEventInfo[type][level].type = type;
    sPressureEventInfo[type][level].level = level;
    sPressureEventInfo[type][level].fd = fd;
    return STATE_OK;
}

/**
* pressure epoll events handler
*
* @param type pressure type @link{PressureType}
* @param level pressure level @link{PressureLevel}
*/
void PressureMonitor::eventHandler(int type, int level) {
    switch (type) {
        case CPU_PRESSURE_TYPE:
            processCpuEvent(level);
            break;
        case IO_PRESSURE_TYPE:
            processIOEvent(level);
            break;
        default:
            return;
    }
}

/**
* processing cpu message events
*
* @param level pressure level @link{PressureLevel}
*/
void PressureMonitor::processCpuEvent(int level) {
    if (mpParseCpuPressure->parseCpuInfo() < 0) {
        ALOG(LOG_ERROR, LOG_TAG, "Failed to parse cpu info!");
        return;
    }
}

/**
* processing IO message events
*
* @param level pressure level @link{PressureLevel}
*/
void PressureMonitor::processIOEvent(int level) {
}

uint64_t PressureMonitor::getProcCpuUsage(int32_t pid) {
    return mpParseCpuPressure->getProcCpuUsage(pid);
}

std::string PressureMonitor::getBackgroundCpuPolicy() {
    return mpParseCpuPressure->getBackgroundCpuPolicy();
}
} // namespace syspressure

} // namespace android