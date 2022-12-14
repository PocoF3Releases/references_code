/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight pipeline test implement file
 *
 */

#include "service_test.h"

#include <future>
#include <iostream>
#include <thread>
#include <string>

#include <binder/IPCThreadState.h>
#include <utils/Thread.h>
#include <utils/Looper.h>



#include "misight_native_service.h"
#include "misight_service.h"
#include "platform_common.h"
#include "plugin_platform.h"

using namespace android;
using namespace android::MiSight;

TEST_F(ServiceTest, TestBinderMsg) {
    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(false));
    auto &platform = PluginPlatform::getInstance();
    sp<MiSightService> service = new MiSightService(&platform);
    MiSightNativeService* nativeService = new MiSightNativeService(service, true);
    int32_t packRet = 0;

    nativeService->notifyPackLog(&packRet);
    String16 testName("test");
    String16 testFile("filePath");
    nativeService->setRunMode(testName, testFile);
    nativeService->notifyUploadSwitch(true);
    nativeService->notifyConfigUpdate(String16("update0"));
    nativeService->notifyUserActivated();

    IPCThreadState* ipc = IPCThreadState::self();
    int64_t token = ((int64_t)1000<<32)|333;
    ipc->restoreCallingIdentity(token);
    nativeService->setRunMode(testName, testFile);
    nativeService->notifyUploadSwitch(true);
    nativeService->notifyConfigUpdate(String16("update1"));
    nativeService->notifyUserActivated();

    nativeService->dump(0, {});
    Vector<String16> veArgs;
    veArgs.push(String16("plugin"));
    veArgs.push(String16("PluginSample1"));
    nativeService->dump(0, veArgs);

    ASSERT_EQ(true, PlatformCommon::InitPluginPlatform(true));
    nativeService->dump(0, veArgs);
}

