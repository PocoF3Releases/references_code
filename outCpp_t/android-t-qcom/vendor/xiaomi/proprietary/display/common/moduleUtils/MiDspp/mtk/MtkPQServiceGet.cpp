#include <cutils/properties.h>
#include <log/log.h>

#include "MtkPQServiceGet.h"

namespace android {

const sp<IPictureQuality> MTKPQServiceGet::getMTKPQService()
{
    Mutex::Autolock autoLock(mServiceLock);
//    bool bPQServiceRun = false;
    char propertyValue[PROPERTY_VALUE_MAX] = {0};

    if (mMTKPQService.get() == NULL) {
//        if (property_get(DISPLAYFEATURE_HAL_STATUS_PROP, propertyValue, NULL) > 0) {
//            if (!(strncmp(propertyValue, "running", PROPERTY_VALUE_MAX))) {
//                displayfeatureRun = true;
//            }
//        }

//        if (displayfeatureRun) {
            ALOGD("get pq service!");
            mMTKPQService = IPictureQuality::getService();
            if (mMTKPQService.get() == NULL) {
                ALOGE("Query pq service returned null!");
                return mMTKPQService;
            }

            mMTKPQService->linkToDeath(this, /*cookie*/1);
//        }
    }

    return mMTKPQService;
}

void MTKPQServiceGet::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/)
{
    Mutex::Autolock autoLock(mServiceLock);

    ALOGW("receive the notification, mMTKPQService service die!");
    if (mMTKPQService.get()) {
        mMTKPQService->unlinkToDeath(this);
        mMTKPQService.clear();
        ALOGD("clean mMTKPQService handle!");
    }
}

}
