#define LOG_TAG "octvm_runtime"

#include <stdint.h>
#include <sys/types.h>

#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "memcontrol.h"
#include "octvm/IOctVmService.h"

namespace android {

enum {
    EX_SECURITY = -1,
    EX_BAD_PARCELABLE = -2,
    EX_ILLEGAL_ARGUMENT = -3,
    EX_NULL_POINTER = -4,
    EX_ILLEGAL_STATE = -5,
    EX_NETWORK_MAIN_THREAD = -6,
    EX_HAS_REPLY_HEADER = -128,  // special; see below
};

class BpOctVmService: public BpInterface<IOctVmService>
{
public:
    BpOctVmService(const sp<IBinder>& impl)
        : BpInterface<IOctVmService>(impl)
    {
    }
    //local dns enable
    virtual int32_t local_dns_enable(String16& confPath) {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        data.writeString16(confPath);
        remote()->transact(BnOctVmService::NET_DNS_ENABLE, data, &reply);
        return reply.readInt32();
    }
    //local dns service disable
    virtual int32_t local_dns_disable() {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        remote()->transact(BnOctVmService::NET_DNS_DISABLE, data, &reply);
        return reply.readInt32();
    }
    //local dns service reload
    virtual int32_t local_dns_reload() {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        remote()->transact(BnOctVmService::NET_DNS_RELOAD, data, &reply);
        return reply.readInt32();
    }
    //set memory mode more memory or more swap
    virtual int32_t set_memory_mode(MemorySwapMode mode)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        data.writeInt32((int)mode);
        status_t status = remote()->transact(BnOctVmService::SET_MEMORY_MODE, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("set_memory_mode() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("set_memory_mode() caught exception %d\n", err);
            return -1;
        }
        return 0;
    }
    //user is focused on this app, interactive with it
    virtual int32_t application_focused(String16 packageName, int pid, int uid)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        data.writeString16(packageName);
        data.writeInt32(pid);
        data.writeInt32(uid);
        status_t status = remote()->transact(BnOctVmService::APPLICATION_FOCUSED, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("application_focused() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("application_focused() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //app is lose focus by user, and run in background
    virtual int32_t application_inactive(String16 packageName, int pid, int uid)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        data.writeString16(packageName);
        data.writeInt32(pid);
        data.writeInt32(uid);
        status_t status = remote()->transact(BnOctVmService::APPLICATION_INACTIVE, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("application_inactive() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("application_inactive() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //app process is started in the background
    virtual int32_t application_started_bg(String16 processName, int pid, int uid)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        data.writeString16(processName);
        data.writeInt32(pid);
        data.writeInt32(uid);
        status_t status = remote()->transact(BnOctVmService::APPLICATION_STARTED_BG, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("application_started_bg() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("application_started_bg() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }

    //call to executing sudebug camera command
    virtual int32_t sudebug_camera_command_execute(const Vector<String16>& args)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        const size_t numArgs = args.size();
        data.writeInt32(numArgs);
        for (size_t i = 0; i < numArgs; i++) {
            data.writeString16(args[i]);
        }
        status_t status = remote()->transact(BnOctVmService::SUDEBUG_CAMERA_COMMAND_EXECUTE, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("sudebug_camera_command_execute() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("sudebug_camera_command_execute() caught exception %d\n", err);
            return -1;
        }
        return 0;
    }

    //call to executing sudebug command
    virtual int32_t sudebug_command_execute(const Vector<String16>& args)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        const size_t numArgs = args.size();
        data.writeInt32(numArgs);
        for (size_t i = 0; i < numArgs; i++) {
            data.writeString16(args[i]);
        }
        status_t status = remote()->transact(BnOctVmService::SUDEBUG_COMMAND_EXECUTE, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("sudebug_command_execute() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("sudebug_command_execute() caught exception %d\n", err);
            return -1;
        }
        return 0;
    }

    virtual int32_t get_pid_by_name(Vector<const char*>& args
            , Vector<key_value_pair_t<String8, String8>>& outNameId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        const size_t numArgs = args.size();
        data.writeInt32(numArgs);
        for (size_t i = 0; i < numArgs; i++) {
            data.writeString16(String16(String8(args[i])));
        }
        status_t status = remote()->transact(BnOctVmService::GET_PID_BY_NAME, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("get_pid_by_name() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("get_pid_by_name() caught exception %d\n", err);
            return -1;
        }
        return 0;
    }

    // return GPU load
    virtual int32_t get_gpu_load()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IOctVmService::getInterfaceDescriptor());
        remote()->transact(BnOctVmService::GET_GPU_LOAD, data, &reply);
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(OctVmService, "miui.whetstone.mcd");

// ----------------------------------------------------------------------

status_t BnOctVmService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case NET_DNS_ENABLE: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            String16 conf = data.readString16();
            int32_t ret = local_dns_enable(conf);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case NET_DNS_DISABLE: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            int32_t ret = local_dns_disable();
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case NET_DNS_RELOAD: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            int32_t ret = local_dns_reload();
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case SET_MEMORY_MODE: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            int mode = data.readInt32();
            int32_t ret = set_memory_mode((MemorySwapMode)mode);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case APPLICATION_FOCUSED: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            String16 packageName = data.readString16();
            int pid = data.readInt32();
            int uid = data.readInt32();
            int32_t ret = application_focused(packageName, pid, uid);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case APPLICATION_INACTIVE: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            String16 packageName = data.readString16();
            int pid = data.readInt32();
            int uid = data.readInt32();
            int32_t ret = application_inactive(packageName, pid, uid);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case APPLICATION_STARTED_BG: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            String16 processName = data.readString16();
            int pid = data.readInt32();
            int uid = data.readInt32();
            int32_t ret = application_started_bg(processName, pid, uid);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case SUDEBUG_CAMERA_COMMAND_EXECUTE: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            int32_t numArgs = data.readInt32();
            Vector<String16> args;
            for (int i = 0; i < numArgs && data.dataAvail() > 0; i++) {
                args.add(data.readString16());
            }
            int32_t ret = sudebug_camera_command_execute(args);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case SUDEBUG_COMMAND_EXECUTE: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            int32_t numArgs = data.readInt32();
            Vector<String16> args;
            for (int i = 0; i < numArgs && data.dataAvail() > 0; i++) {
                args.add(data.readString16());
            }
            int32_t ret = sudebug_command_execute(args);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case GET_PID_BY_NAME: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            int32_t numArgs = data.readInt32();
            Vector<const char*> args;
            for (int i = 0; i < numArgs && data.dataAvail() > 0; i++) {
                args.add(String8(data.readString16()).string());
            }
            Vector<key_value_pair_t<String8, String8>> output;
            int32_t ret = get_pid_by_name(args, output);
            reply->writeNoException();
            reply->writeInt32(output.size());
            for(int i = 0; i < output.size(); i++) {
                reply->writeString16(String16(output[i].getKey()));
                reply->writeString16(String16(output[i].getValue()));
            }
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case GET_GPU_LOAD: {
            CHECK_INTERFACE(IOctVmService, data, reply);
            int32_t ret = get_gpu_load();
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
