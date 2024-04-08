#ifndef OCTVM_SERVICE_H
#define OCTVM_SERVICE_H

#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include "memcontrol.h"

namespace android {

class IOctVmService: public IInterface {
public:
    enum {
        NET_DNS_ENABLE  = IBinder::FIRST_CALL_TRANSACTION + 0,
        NET_DNS_DISABLE = IBinder::FIRST_CALL_TRANSACTION + 1,
        NET_DNS_RELOAD = IBinder::FIRST_CALL_TRANSACTION + 2,
        SET_MEMORY_MODE = IBinder::FIRST_CALL_TRANSACTION + 3,
        APPLICATION_FOCUSED = IBinder::FIRST_CALL_TRANSACTION + 4,
        APPLICATION_INACTIVE = IBinder::FIRST_CALL_TRANSACTION + 5,
        APPLICATION_STARTED_BG = IBinder::FIRST_CALL_TRANSACTION + 6,
        SUDEBUG_COMMAND_EXECUTE = IBinder::FIRST_CALL_TRANSACTION + 7,
        GET_PID_BY_NAME = IBinder::FIRST_CALL_TRANSACTION + 8,
        GET_GPU_LOAD = IBinder::FIRST_CALL_TRANSACTION + 9,
        SUDEBUG_CAMERA_COMMAND_EXECUTE = IBinder::FIRST_CALL_TRANSACTION + 10,
    };

    DECLARE_META_INTERFACE(OctVmService);

    virtual int32_t local_dns_enable(String16& confPath) = 0;

    virtual int32_t local_dns_disable() = 0;

    virtual int32_t local_dns_reload() = 0;

    virtual int32_t set_memory_mode(MemorySwapMode mode) = 0;

    virtual int32_t application_focused(String16 packageName, int pid, int uid) = 0;

    virtual int32_t application_inactive(String16 packageName, int pid, int uid) = 0;

    virtual int32_t application_started_bg(String16 processName, int pid, int uid) = 0;

    virtual int32_t sudebug_camera_command_execute(const Vector<String16>& args) = 0;

    virtual int32_t sudebug_command_execute(const Vector<String16>& args) = 0;

    virtual int32_t get_pid_by_name(Vector<const char*>& args, Vector<key_value_pair_t<String8, String8>>& outNameId) = 0;

    virtual int32_t get_gpu_load(void) = 0;
};

// ----------------------------------------------------------------------------

class BnOctVmService: public BnInterface<IOctVmService> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply,
            uint32_t flags = 0);
};

} // namespace android

#endif
