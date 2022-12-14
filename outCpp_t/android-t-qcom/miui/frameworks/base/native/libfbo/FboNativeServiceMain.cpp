/**
 * @file FboNativeServiceMain.cpp
 * @author cuijiguang <cuijiguang@xiaomi.com>
 *         lijiaming <lijiaming3@xiaomi.com>
 * @brief Service is responsible for checking and defragmenting the file system. 
 *        Send LBA to the framework through socket, and the framework schedules FBO Hal service
 * @version 2.0
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022 Xiaomi
 * 
 */

#define LOG_TAG "fbo"

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

#include "FboNativeService.h"
#include "f2fs_depend.h"

using namespace android;


int main(){
    FBO_NATIVE_DBG_MSG("FBOStarting " LOG_TAG);

    android::sp<android::IServiceManager> serviceManager = android::defaultServiceManager();
    android::sp<android::FboNativeService> deamon = android::FboNativeService::getInstance();
    android::status_t ret = serviceManager->addService(
            android::String16("FboNativeService"), deamon);
    if (ret != android::OK) {
        FBO_NATIVE_ERR_MSG("Couldn't register " LOG_TAG " binder service!");
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