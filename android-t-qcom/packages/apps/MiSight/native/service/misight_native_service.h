/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight service head file
 *
 */
#ifndef MISIGHT_SERVICE_MISIGHT_NATIVE_SERVICE_H
#define MISIGHT_SERVICE_MISIGHT_NATIVE_SERVICE_H

#include <utils/RefBase.h>
#include <binder/Status.h>
#include <utils/String16.h>
#include <utils/StrongPointer.h>
#include <utils/Vector.h>



#include "android/MiSight/BnMiSightNativeService.h"

#include "misight_service.h"


namespace android {
namespace MiSight {
class MiSightNativeService : public BnMiSightNativeService {
public:
    explicit MiSightNativeService(sp<MiSightService> service, bool safeCheck);
    ~MiSightNativeService();
    binder::Status notifyPackLog(/*out*/ int32_t* packRet) override;
    binder::Status setRunMode(const String16& runName, const String16& filePath) override;
    binder::Status notifyUploadSwitch(bool isOn) override;
    binder::Status notifyConfigUpdate(const String16& folderName) override;
    binder::Status notifyUserActivated() override;
    status_t dump(int fd, const Vector<String16>& args) override;


private:
    sp<MiSightService> service_;
    bool safeCheck_;
    bool CanProcess();
};
} // namespace MiSight
} // namespace android
#endif

