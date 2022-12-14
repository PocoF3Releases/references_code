#define LOG_TAG "dfc"

#include <cutils/log.h>
#include <utils/Log.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/PermissionCache.h>
#include <utils/String16.h>
#include <binder/Parcel.h>
#include <utils/RefBase.h>
#include <utils/Unicode.h>

#include "DfcNativeService.h"
#include "Dfc.h"

using namespace android;


int main(){
    DFC_NATIVE_DBG_MSG("DFCStarting " LOG_TAG);

    android::sp<android::IServiceManager> serviceManager = android::defaultServiceManager();
    android::sp<android::DfcNativeService> deamon = android::DfcNativeService::getInstance();
    android::status_t ret = serviceManager->addService(
            android::String16("DfcNativeService"), deamon);
    if (ret != android::OK) {
        DFC_NATIVE_ERR_MSG("Couldn't register " LOG_TAG " binder service!");
        return -1;
    }

    /*
     * start thread pool with 4 threads for run command
     */
    android::ProcessState::self()->setThreadPoolMaxThreadCount(4);
    android::sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();
    android::IPCThreadState::self()->joinThreadPool();
    return 0;
}