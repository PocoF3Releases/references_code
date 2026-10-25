#include "SysPressureCtrlManager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "syspressure.manager"

namespace android {

namespace syspressure {

SysPressureCtrlManager* SysPressureCtrlManager::spManager;
Mutex SysPressureCtrlManager::sLock;

SysPressureCtrlManager::SysPressureCtrlManager() :
                mpPressureMonitor(std::make_shared<PressureMonitor>()),
                mCpuPressureCall(nullptr),
                mCpuExceptionCall(nullptr),
                mThermalTempCall(nullptr) {
    //Singleton<CpuLimitInfo>::Instance();
}

SysPressureCtrlManager::~SysPressureCtrlManager() {
    //Singleton<CpuLimitInfo>::DestroyInstance();
}

SysPressureCtrlManager* SysPressureCtrlManager::getInstance() {
    if (nullptr == spManager) {
        Mutex::Autolock _l(sLock);
        if( nullptr == spManager ) {
            spManager = new SysPressureCtrlManager();
        }
    }
    //static SysPressureCtrlManager spManager;
    return spManager;
}

status_t SysPressureCtrlManager::init() {
    return STATE_OK;
}

void SysPressureCtrlManager::startPressureMonitor() {
    if(KEnablePressure) {
       mpPressureMonitor->init();
    }
}

void SysPressureCtrlManager::endPressureMonitor() {
    if(KEnablePressure) {
       mpPressureMonitor->destory();
    }
}

uint64_t SysPressureCtrlManager::getCpuUsage(int32_t pid, std::string processName) {
    uint64_t ret = 0;
    if((ret = mpPressureMonitor->getProcCpuUsage(pid)) < 0) {
        return 0;
    }
    return ret;
}

void SysPressureCtrlManager::setCpuPressureCall(CpuPressureCall callback) {
    mCpuPressureCall = callback;
}

void SysPressureCtrlManager::setCpuExceptionCall(CpuExceptionCall callback) {
    mCpuExceptionCall = callback;
}

void SysPressureCtrlManager::setThermalTempCall(ThermalTempCall callback) {
    mThermalTempCall = callback;
}

void SysPressureCtrlManager::notifyCpuPressure(int level) {
    if(KEnablePressure && mCpuPressureCall != nullptr) {
        mCpuPressureCall(level);
    }
}

void SysPressureCtrlManager::notifyCpuException(int type) {
    if(KEnablePressure && mCpuExceptionCall != nullptr) {
        mCpuExceptionCall(type);
    }
}

void SysPressureCtrlManager::notifyThermalTempChange(int temp) {
    if(mThermalTempCall != nullptr) {
        mThermalTempCall(temp);
    }
}

std::string SysPressureCtrlManager::getBackgroundCpuPolicy() {
    return mpPressureMonitor->getBackgroundCpuPolicy();
}
} // namespace syspressure

} // namespace android