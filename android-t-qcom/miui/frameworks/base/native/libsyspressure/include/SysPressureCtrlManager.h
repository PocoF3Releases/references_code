/**
 * system resource management module
 */
#pragma once

#include "common/Common.h"
#include "pressure/PressureMonitor.h"

namespace android {

namespace syspressure {

class SysPressureCtrlManager {
public:
    /**
     * get class instance
     **/
    static SysPressureCtrlManager* getInstance();
    status_t init();
    void startPressureMonitor();
    void endPressureMonitor();
    uint64_t getCpuUsage(int32_t pid, std::string processName);

    using CpuPressureCall = std::function<void(int level)>;
    using CpuExceptionCall = std::function<void(int type)>;
    using ThermalTempCall = std::function<void(int type)>;
    void setCpuPressureCall(CpuPressureCall callback);
    void setCpuExceptionCall(CpuExceptionCall callback);
    void setThermalTempCall(ThermalTempCall callback);
    void notifyCpuPressure(int level);
    void notifyCpuException(int type);// @link{CpuExceptionType}
    void notifyThermalTempChange(int temp);
    std::string getBackgroundCpuPolicy();
private:
    SysPressureCtrlManager();
    virtual ~SysPressureCtrlManager();
private:
    SysPressureCtrlManager(const SysPressureCtrlManager&) = delete;
    SysPressureCtrlManager& operator = (const SysPressureCtrlManager&) = delete;

    static SysPressureCtrlManager* spManager;
    static Mutex sLock;
    std::shared_ptr<PressureMonitor> mpPressureMonitor;

    CpuPressureCall mCpuPressureCall;
    CpuExceptionCall mCpuExceptionCall;
    ThermalTempCall mThermalTempCall;
};

} // namespace syspressure

} // namespace android
