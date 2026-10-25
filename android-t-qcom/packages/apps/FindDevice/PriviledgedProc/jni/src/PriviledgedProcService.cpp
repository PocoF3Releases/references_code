#include "PriviledgedProcService.h"

#include <utils/Mutex.h>
#include <binder/IServiceManager.h>

#include "../common/log.h"

const android::sp<IPriviledgedProcService> & getPriviledgedProcService() {
    static android::sp<IPriviledgedProcService> sService(NULL);
    static android::Mutex sLock;
    android::Mutex::Autolock autolock(sLock);

    bool isBinderAlive = false;
    if (sService != NULL) {
        LOGI("getPriviledgedProcService: sService != NULL. ");
        isBinderAlive =
#ifdef BINDER_STATIC_AS_BINDER
            android::IInterface::asBinder(sService)->pingBinder() == android::NO_ERROR;
#else
            sService->asBinder()->pingBinder() == android::NO_ERROR;
#endif
    } else {
        LOGI("getPriviledgedProcService: sService == NULL. ");
    }

    if (!isBinderAlive) {
       LOGW("getPriviledgedProcService: binder not alive. ");
       android::sp<android::IServiceManager> sm = android::defaultServiceManager();
       do {
          android::sp<android::IBinder> binder = sm->getService(android::String16("miui.fdpp"));
          if (binder != NULL) {
            sService = android::interface_cast<IPriviledgedProcService>(binder);
            break;
          }
          LOGE("Failed to get PriviledgedProcService binder. ");
          sleep(1);
       } while (true);
    } else {
        LOGI("getPriviledgedProcService: binder alive. ");
    }

    return sService;
}