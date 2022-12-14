/**
 * pressure monitoring.
 * Monitor cpu/io/ memory pressure
 */
#pragma once

#include <thread>
#include "common/Common.h"
#include "pressure/ParseCpuPressure.h"
#include "pressure/PsiThread.h"

namespace android {

namespace syspressure {

class PressureMonitor {
public:
    PressureMonitor();
    ~PressureMonitor();
    status_t init();
    void destory();
    /**
    * get proc/pid/stat cpu usage
    * @param pid
    */
    uint64_t getProcCpuUsage(int32_t pid);
    std::string getBackgroundCpuPolicy();
private:
    void mainLoop();

    // PSI
    /**
    * initialization psi monitoring
    *
    * @param type pressure type @link{PressureType}
    * @param level pressure level @link{PressureLevel}
    * @return error(false) success(true)
    */
    status_t initPsiMonitor(int type, int level);

    /**
    * pressure epoll events handler
    *
    * @param type pressure type @link{PressureType}
    * @param level pressure level @link{PressureLevel}
    */
    void eventHandler(int type, int level);

    /**
    * processing cpu message events
    *
    * @param level pressure level @link{PressureLevel}
    */
    void processCpuEvent(int level);

    /**
    * processing IO message events
    *
    * @param level pressure level @link{PressureLevel}
    */
    void processIOEvent(int level);

private:
    volatile bool mInit;
    // memory parse
    std::shared_ptr<ParseCpuPressure> mpParseCpuPressure;

    sp<PsiThread> mpPsiThread;

    Mutex mLock;
};

} // namespace syspressure

} // namespace android
