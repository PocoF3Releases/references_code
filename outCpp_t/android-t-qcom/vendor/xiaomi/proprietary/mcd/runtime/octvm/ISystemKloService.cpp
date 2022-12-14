#define LOG_TAG "octvm_runtime"

#include <stdint.h>
#include <sys/types.h>

#include <utils/Log.h>
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include "octvm/ISystemKloService.h"

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

class BpSystemKloService: public BpInterface<ISystemKloService>
{
public:
    BpSystemKloService(const sp<IBinder>& impl)
        : BpInterface<ISystemKloService>(impl)
    {
    }

    //enable/disable klo feature
    virtual int32_t klo_feature_enable(int enable)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemKloService::getInterfaceDescriptor());
        data.writeInt32(enable);
        status_t status = remote()->transact(BnSystemKloService::KLO_FEATURE_ENABLE, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("klo_feature_enable() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("klo_feature_enable() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //register a new klo task
    virtual int32_t register_klo_task(klo_task_t *task)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemKloService::getInterfaceDescriptor());
        data.write(task, sizeof(*task));
        status_t status = remote()->transact(BnSystemKloService::REGISTER_KLO_TASK, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("register_klo_task() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("register_klo_task() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //unregister a klo task
    virtual int32_t unregister_klo_task(int32_t task_id)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemKloService::getInterfaceDescriptor());
        data.writeInt32(task_id);
        status_t status = remote()->transact(BnSystemKloService::UNREGISTER_KLO_TASK, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("unregister_klo_task() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("unregister_klo_task() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //set klo dump files
    virtual int32_t set_klo_dump_files(const Vector<String16>& fileDescs)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemKloService::getInterfaceDescriptor());
        const size_t numFiles = fileDescs.size();
        data.writeInt32(numFiles);
        for (size_t i = 0; i < numFiles; i++) {
            data.writeString16(fileDescs[i]);
        }
        status_t status = remote()->transact(BnSystemKloService::SET_KLO_DUMP_FILES, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("set_klo_dump_files() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("set_klo_dump_files() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //dump system full info to a file
    virtual int32_t klo_dump_system(const char *file_path)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemKloService::getInterfaceDescriptor());
        data.writeCString(file_path);
        status_t status = remote()->transact(BnSystemKloService::KLO_DUMP_SYSTEM_FULL, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("klo_dump_system() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("klo_dump_system() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
    //get anonymous id
    virtual int32_t klo_get_anonymous_id(String16& anonymous_id)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemKloService::getInterfaceDescriptor());
        status_t status = remote()->transact(BnSystemKloService::KLO_GET_ANONYMOUS_ID, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("klo_get_anonymous_id() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        if (err < 0) {
            ALOGD("klo_get_anonymous_id() caught exception %d\n", err);
            return -1;
        }
        int32_t ret = reply.readInt32();
        int id_present = reply.readInt32();
        if (id_present)
            anonymous_id = reply.readString16();
        return ret;
    }
    //klo system debug (run as system uid)
    virtual int32_t klo_system_debug(const Vector<String16>& args)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ISystemKloService::getInterfaceDescriptor());
        const size_t numArgs = args.size();
        data.writeInt32(numArgs);
        for (size_t i = 0; i < numArgs; i++) {
            data.writeString16(args[i]);
        }
        status_t status = remote()->transact(BnSystemKloService::KLO_SYSTEM_DEBUG, data, &reply);
        if (status != NO_ERROR) {
            ALOGD("klo_system_debug() could not contact remote: %d\n", status);
            return -1;
        }
        int32_t err = reply.readExceptionCode();
        int32_t ret = reply.readInt32();
        if (err < 0) {
            ALOGD("klo_system_debug() caught exception %d\n", err);
            return -1;
        }
        return ret;
    }
};

IMPLEMENT_META_INTERFACE(SystemKloService, "miui.whetstone.klo");

// ----------------------------------------------------------------------

status_t BnSystemKloService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case KLO_FEATURE_ENABLE: {
            CHECK_INTERFACE(ISystemKloService, data, reply);
            int enable = data.readInt32();
            int32_t ret = klo_feature_enable(enable);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case REGISTER_KLO_TASK: {
            CHECK_INTERFACE(ISystemKloService, data, reply);
            void *outData = calloc(1, sizeof(klo_task_t));
            data.read(outData, sizeof(klo_task_t));
            if (((klo_task_t *)outData)->magic != 0x4B4C4F) {
                reply->writeInt32(EX_ILLEGAL_ARGUMENT);
                reply->writeInt32(-1);
            } else {
                int32_t ret = register_klo_task((klo_task_t *)outData);
                reply->writeNoException();
                reply->writeInt32(ret);
            }
            return NO_ERROR;
        } break;
        case UNREGISTER_KLO_TASK: {
            CHECK_INTERFACE(ISystemKloService, data, reply);
            int32_t task_id = data.readInt32();
            int32_t ret = unregister_klo_task(task_id);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case SET_KLO_DUMP_FILES: {
            CHECK_INTERFACE(ISystemKloService, data, reply);
            int32_t numFiles = data.readInt32();
            Vector<String16> fileDescs;
            for (int i = 0; i < numFiles && data.dataAvail() > 0; i++) {
                fileDescs.add(data.readString16());
            }
            int32_t ret = set_klo_dump_files(fileDescs);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case KLO_DUMP_SYSTEM_FULL: {
            CHECK_INTERFACE(ISystemKloService, data, reply);
            const char* file_path = data.readCString();
            int32_t ret = klo_dump_system(file_path);
            reply->writeNoException();
            reply->writeInt32(ret);
            return NO_ERROR;
        } break;
        case KLO_GET_ANONYMOUS_ID: {
            CHECK_INTERFACE(ISystemKloService, data, reply);
            String16 anonymous_id = String16("unknown");
            int32_t ret = klo_get_anonymous_id(anonymous_id);
            reply->writeNoException();
            reply->writeInt32(ret);
            if (anonymous_id == String16("unknown")) {
                reply->writeInt32(0);
            } else {
                reply->writeInt32(1);
                reply->writeString16(anonymous_id);
            }
            return NO_ERROR;
        } break;
        case KLO_SYSTEM_DEBUG: {
            CHECK_INTERFACE(ISystemKloService, data, reply);
            int32_t numArgs = data.readInt32();
            Vector<String16> args;
            for (int i = 0; i < numArgs && data.dataAvail() > 0; i++) {
                args.add(data.readString16());
            }
            int32_t ret = klo_system_debug(args);
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
