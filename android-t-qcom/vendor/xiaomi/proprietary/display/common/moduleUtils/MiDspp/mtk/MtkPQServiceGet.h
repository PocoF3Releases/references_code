#ifndef MTK_PQ_SERVICE_GET_H
#define MTK_PQ_SERVICE_GET_H

#include <utils/StrongPointer.h>
#include <utils/Mutex.h>

#include <vendor/mediatek/hardware/pq/2.11/IPictureQuality.h>

using vendor::mediatek::hardware::pq::V2_11::IPictureQuality;

using android::hardware::hidl_array;
using android::hidl::base::V1_0::IBase;
using android::hardware::hidl_death_recipient;

namespace android {

class MTKPQServiceGet : public hidl_death_recipient {
public:
    MTKPQServiceGet() { }
    virtual ~MTKPQServiceGet() { }

    virtual void serviceDied(uint64_t cookie, const wp<IBase>& who);
    const sp<IPictureQuality> getMTKPQService();

private:
    Mutex mServiceLock;
    sp<IPictureQuality> mMTKPQService;
};

}

#endif
