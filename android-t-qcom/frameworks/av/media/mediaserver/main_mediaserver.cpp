/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "mediaserver"
//#define LOG_NDEBUG 0

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/Log.h>
#include "vndksupport/linker.h"
#include <dlfcn.h>
#include "RegisterExtensions.h"

#include <MediaPlayerService.h>
#include <ResourceManagerService.h>

using namespace android;

int main(int argc __unused, char **argv __unused)
{
    signal(SIGPIPE, SIG_IGN);

    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("ServiceManager: %p", sm.get());
    MediaPlayerService::instantiate();
    ResourceManagerService::instantiate();
    registerExtensions();

    // pre dlopen can improve dlopen next time. We do not to close dlhandler by designed.
    void* dlhandler = android_load_sphal_library("/vendor/lib64/hw/android.hardware.graphics.mapper@4.0-impl-qti-display.so", RTLD_NOW | RTLD_LOCAL);
    if (dlhandler == NULL) {
        ALOGI("dlopen, /vendor/lib64/hw/android.hardware.graphics.mapper@4.0-impl-qti-display.so failed! errno = %d", errno);
    }

    ::android::hardware::configureRpcThreadpool(16, false);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
    ::android::hardware::joinRpcThreadpool();
}
