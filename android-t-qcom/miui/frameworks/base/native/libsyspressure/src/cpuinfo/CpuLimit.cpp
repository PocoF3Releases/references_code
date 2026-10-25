#include "common/Common.h"
#include "cpuinfo/CpuLimit.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "syspressure.cpulimit"

namespace android {

namespace syspressure {

#define CPU_LIMIT_CYC               1000 // ms

CpuLimit::CpuLimit() :
          mpProcessPools(std::make_shared<ProcessPools>(kCpuLimitThreadPoolCount)),
          mPreTrackTime(0) {
    mpProcessPools->setTaskCallback(
            std::bind(&CpuLimit::handleCpuLimit, this, std::placeholders::_1));
}

CpuLimit::~CpuLimit() {

}

int CpuLimit::addProcessLimit(int32_t pid, std::string& processName, int32_t limit) {
    return mpProcessPools->enqueueProcess(pid, processName, limit);
}

int CpuLimit::removeProcessLimit(int32_t pid) {
    return mpProcessPools->dequeueProcess(pid);
}

void CpuLimit::handleCpuLimit(Process *process) {
    int32_t pid = process->getPID();
    // calculate cpu usage time
    int32_t cpuUsage = process->calcCpuUsage();
    if(cpuUsage <= 0 || cpuUsage <  kCpuLimtThreshold) {
        return;
    }
    // calculate current process running time and stop time
    int32_t runTime = process->getCpuLimit()*CPU_LIMIT_CYC/cpuUsage;
    int32_t sleepTime = CPU_LIMIT_CYC - runTime;
    ALOG(LOG_DEBUG, LOG_TAG,
            "handleCpuLimit: pid[%d] pck[%s] cpuUsage[%d], limitTo[%d] runTime[%d], sleepTime[%d]",
                            pid, process->getProcessName().c_str(), cpuUsage,
                            process->getCpuLimit(), runTime, sleepTime);
    // history
    std::shared_ptr<CpuLimitInfo> limitInfo = std::make_shared<CpuLimitInfo>(process->getPID(),
                         process->getProcessName(),
                         cpuUsage,
                         Utils::milliTime(true));
    mHistorys.addLimitInfo(limitInfo);
    int64_t startCpuTime = process->getCurrentCpuTime();
    // limit
    while(process->isPausingState()) {
        if(!process->stopProcess()) {
            ALOG(LOG_DEBUG, LOG_TAG, "handleCpuLimit: stop pid[%d] error[process dead]", pid);
            break;
        }
        // stop process time
        Utils::milliSleep(sleepTime);
        if(!process->startProcess()) {
            ALOG(LOG_DEBUG, LOG_TAG, "handleCpuLimit: start pid[%d] error[process dead]", pid);
            break;
        }
        // start process time
        Utils::milliSleep(runTime);
    }
    // history
    int64_t endCpuTime = process->getCurrentCpuTime();
    limitInfo->mLimitEndTime = Utils::milliTime(true);
    // usage
    if(startCpuTime > 0 && endCpuTime > 0) {
        int32_t limitTime = limitInfo->mLimitEndTime - limitInfo->mLimitStartTime;
        limitInfo->mLimitAvgUsage = (endCpuTime - startCpuTime)*kCpuUsagePercent/limitTime;
    }
    ALOG(LOG_DEBUG, LOG_TAG,
                "handleCpuLimit: pid[%d] pck[%s] stop", pid, process->getProcessName().c_str());
}

std::shared_ptr<VectorCpuLimitInfo> CpuLimit::getHistoryInfos(uint64_t sinceUptime) {
    return mHistorys.getHistoryInfos(sinceUptime);
}

std::shared_ptr<VectorCpuLimitInfo> CpuLimit::getCpuLimitInfos() {
    return mHistorys.getCurrentLimitInfos();
}

void CpuLimit::onCpuPressure(int level) {
    if(!kEnableCpuLimit) return;
    // filter
    uint64_t currentTime = Utils::milliTime();
    if((currentTime - mPreTrackTime) < 5000) return; // 5s

    mPreTrackTime = currentTime;
}
} // namespace syspressure

} // namespace android