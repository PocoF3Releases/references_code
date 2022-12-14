/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight service implement file
 *
 */

#include "misight_native_service.h"

#include <vector>

#include <binder/IPCThreadState.h>
#include <utils/String8.h>

#include "cmd_processor.h"

namespace android {
namespace MiSight {
constexpr int32_t SYSTEM_ID = 1000;
const std::string PROCESS_NAME = "com.miui.misightservice";
// Convenience methods for constructing binder::Status objects for error returns
#define STATUS_ERROR(errorCode, errorString) \
        binder::Status::fromServiceSpecificError(errorCode, \
        String8::format("%s:%d: %s", __FUNCTION__, __LINE__, errorString))

MiSightNativeService::MiSightNativeService(sp<MiSightService> service, bool safeCheck)
    : service_(service), safeCheck_(safeCheck)
{}

MiSightNativeService::~MiSightNativeService()
{}

bool MiSightNativeService::CanProcess()
{
    if (!safeCheck_) {
        return true;
    }

    IPCThreadState* ipc = IPCThreadState::self();
    int calling_uid = ipc->getCallingUid();
    if(calling_uid != SYSTEM_ID) {
       MISIGHT_LOGI("not suport calling_uid %d", calling_uid);
       return false;
    }

    pid_t pid = ipc->getCallingPid();
    std::string pidName = CmdProcessor::GetProcNameByPid(pid);
    #ifdef MISIGHT_TEST
    pidName = "com.miui.misightservice";
    #endif
    if (pidName != PROCESS_NAME) {
        MISIGHT_LOGI("not suport process %d", pid);
        return false;
    }

    return true;
}

binder::Status MiSightNativeService::notifyPackLog(int32_t* packRet)
{
    if (!CanProcess()) {
        return binder::Status::ok();
    }
    if (service_ == nullptr) {
        String8 msg = String8::format("service is null");
        return STATUS_ERROR(ERROR_INTERNAL, msg.string());
    }
    MISIGHT_LOGI("notify pack log start");
    *packRet = service_->NotifyPackLog();
    return binder::Status::ok();
}

binder::Status MiSightNativeService::setRunMode(const String16& runName, const String16& filePath)
{
    if (!CanProcess()) {
        return binder::Status::ok();
    }

    if (service_ == nullptr) {
        String8 msg = String8::format("service is null");
        return STATUS_ERROR(ERROR_INTERNAL, msg.string());
    }

    String8 runNameStr(runName);
    String8 filePathStr(filePath);
    MISIGHT_LOGI("set run mode name=%s, file=%s", runNameStr.string(), filePathStr.string());
    service_->SetRunMode(std::string(runNameStr.string()), std::string(filePathStr.string()));
    return binder::Status::ok();
}

binder::Status MiSightNativeService::notifyUploadSwitch(bool isOn)
{
    if (!CanProcess()) {
        return binder::Status::ok();
    }
    if (service_ == nullptr) {
        String8 msg = String8::format("service is null");
        return STATUS_ERROR(ERROR_INTERNAL, msg.string());
    }
    MISIGHT_LOGI("notify update switch start %d", isOn);
    service_->NotifyUploadSwitch(isOn);
    return binder::Status::ok();
}

binder::Status MiSightNativeService::notifyUserActivated()
{
    if (!CanProcess()) {
        return binder::Status::ok();
    }

    if (service_ == nullptr) {
        String8 msg = String8::format("service is null");
        return STATUS_ERROR(ERROR_INTERNAL, msg.string());
    }
    MISIGHT_LOGI("notify user activate");
    service_->NotifyUserActivated();
    return binder::Status::ok();
}

binder::Status MiSightNativeService::notifyConfigUpdate(const String16& folderName)
{
    if (!CanProcess()) {
        return binder::Status::ok();
    }

    if (service_ == nullptr) {
        String8 msg = String8::format("service is null");
        return STATUS_ERROR(ERROR_INTERNAL, msg.string());
    }

    String8 updateFolder(folderName);
    MISIGHT_LOGI("notify config update start %s", updateFolder.string());
    service_->NotifyConfigUpdate(std::string(updateFolder.string()));
    return binder::Status::ok();
}

status_t MiSightNativeService::dump(int fd, const Vector<String16>& args)
{
    MISIGHT_LOGD("dump start");
    if (service_ == nullptr) {
        String8 msg = String8::format("service is null");
        dprintf(fd, "%s\n", msg.string());
        return 0;
    }
    std::vector<std::string> cmds;
     for (uint32_t argIndex = 0; argIndex < args.size(); argIndex++) {
        String8 argStr(args[argIndex]);
        cmds.push_back(std::string(argStr.string()));
    }
    service_->Dump(fd, cmds);
    return 0;
}

} // namespace MiSight
} // namespace android

