#ifndef OCTVM_ISYSTEMKLOSERVICE_H
#define OCTVM_ISYSTEMKLOSERVICE_H

#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include "klo_task.h"

namespace android {

class ISystemKloService: public IInterface {
public:
    enum {
        KLO_FEATURE_ENABLE = IBinder::FIRST_CALL_TRANSACTION + 0,
        REGISTER_KLO_TASK = IBinder::FIRST_CALL_TRANSACTION + 1,
        UNREGISTER_KLO_TASK = IBinder::FIRST_CALL_TRANSACTION + 2,
        SET_KLO_DUMP_FILES = IBinder::FIRST_CALL_TRANSACTION + 3,
        KLO_GET_ANONYMOUS_ID = IBinder::FIRST_CALL_TRANSACTION + 4,
        KLO_DUMP_SYSTEM_FULL = IBinder::FIRST_CALL_TRANSACTION + 5,
        KLO_SYSTEM_DEBUG = IBinder::FIRST_CALL_TRANSACTION + 6,
    };

    DECLARE_META_INTERFACE(SystemKloService);

    // -------------------------------------------------------------------------
    // system klo operation
    // -------------------------------------------------------------------------
    virtual int32_t klo_feature_enable(int enable) = 0;

    virtual int32_t register_klo_task(klo_task_t *task) = 0;

    virtual int32_t unregister_klo_task(int32_t task_id) = 0;

    virtual int32_t set_klo_dump_files(const Vector<String16>& fileDescs) = 0;

    virtual int32_t klo_get_anonymous_id(String16& anonymous_id) = 0;

    virtual int32_t klo_dump_system(const char *file_path) = 0;

    virtual int32_t klo_system_debug(const Vector<String16>& args) = 0;
};

// ----------------------------------------------------------------------------

class BnSystemKloService: public BnInterface<ISystemKloService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
            uint32_t flags = 0);
};

} // namespace android

#endif
