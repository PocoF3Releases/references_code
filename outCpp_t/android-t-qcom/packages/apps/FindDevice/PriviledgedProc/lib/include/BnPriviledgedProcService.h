#ifndef ANDROID_BNPRIVILEDGEDPROCSERVICE_H
#define ANDROID_BNPRIVILEDGEDPROCSERVICE_H

#include <binder/IInterface.h>
#include <IPriviledgedProcService.h>

class BnPriviledgedProcService : public android::BnInterface<IPriviledgedProcService> {
public:
    virtual android::status_t onTransact(uint32_t code,
                                        const android::Parcel& data,
                                        android::Parcel* reply,
                                        uint32_t flags = 0);
};

#endif //ANDROID_BNPRIVILEDGEDPROCSERVICE_H