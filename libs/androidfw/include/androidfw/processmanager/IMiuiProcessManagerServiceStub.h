#ifndef ANDROID_IMIUIPROCESSMANAGERSERVICESTUB_H
#define ANDROID_IMIUIPROCESSMANAGERSERVICESTUB_H

#include <stdint.h>
#include <sys/types.h>
#include <iostream>

namespace android {

class IMiuiProcessManagerServiceStub {
public:
    virtual ~IMiuiProcessManagerServiceStub() {};
    virtual bool setSchedFifo(const ::std::vector<int32_t>& tids, int64_t duration,
                        int32_t pid, int32_t mode);
};

typedef IMiuiProcessManagerServiceStub* CreateMiuiProcessImpl();
typedef void DestroyMiuiProcessImpl(IMiuiProcessManagerServiceStub *);

}
#endif //ANDROID_IMIUIPROCESSMANAGERSERVICESTUB_H
