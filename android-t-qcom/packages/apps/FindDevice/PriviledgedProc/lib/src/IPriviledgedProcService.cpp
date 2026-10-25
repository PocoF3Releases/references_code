#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

#include <IPriviledgedProcService.h>

#include "../common/log.h"
#include "../../../common/native/include/util_common.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

enum {
    TR_CODE_EXISTS = android::IBinder::FIRST_CALL_TRANSACTION,
    TR_CODE_REMOVE,
    TR_CODE_LSDIR,
    TR_CODE_ISFILE,
    TR_CODE_ISDIR,
    TR_CODE_MOVE,
    TR_CODE_MKDIR,
    TR_CODE_WRITETINYFILE,
    TR_CODE_READTINYFILE,
    TR_CODE_ADDSERVICE,
    TR_CODE_GET_SET_REBOOT_CLEAR_VARIABLE,
};


#define PERMISSION_DENIED android::PERMISSION_DENIED

// Bn definition.
#include <BnPriviledgedProcService.h>

// Bp definition.
class BpPriviledgedProcService : public android::BpInterface<IPriviledgedProcService> {
public:
    BpPriviledgedProcService(const android::sp<android::IBinder>& impl) :
        android::BpInterface<IPriviledgedProcService>(impl) {}

public:
    virtual bool exists(const char * path, bool * pRst) const;
    virtual bool remove(const char * path) const;
    virtual bool lsDir(const char * path, std::vector<std::vector<char> > * pList) const;
    virtual bool isFile(const char * path, bool * pRst) const;
    virtual bool isDir(const char * path, bool * pRst) const;
    virtual bool move(const char * oldPath, const char * newPath) const;
    virtual bool mkdir(const char * path) const;
    virtual bool writeTinyFile(const char * path, bool append, const unsigned char * bytes, ssize_t len) const;
    virtual bool readTinyFile(const char * path, std::vector<unsigned char> * pBytes) const;
    virtual bool addService(const char * name, const android::sp<android::IBinder> & service) const;
    virtual bool getSetRebootClearVariable(const char * name, std::vector<char> * pOldValue, bool modify, const char * pNewValue);
};

// Implements:

IMPLEMENT_META_INTERFACE(PriviledgedProcService, "miui.fdpp")

android::status_t BnPriviledgedProcService::onTransact(uint32_t code,
                                        const android::Parcel& data,
                                        android::Parcel* reply,
                                        uint32_t flags) {

    if (!android::checkCallingPermission(android::String16(PRIVILEDGED_PROC_PERMISSION))) {
        LOGE("Permission Denial: can't use fdpp uid=%d pid=%d",
                android::IPCThreadState::self()->getCallingPid(),
                android::IPCThreadState::self()->getCallingUid());
        return PERMISSION_DENIED;
    }

    CHECK_INTERFACE(PriviledgedProcService, data, reply);

    switch (code) {

    case TR_CODE_EXISTS:
    {
        bool rst = false;
        bool rtn = exists(data.readCString(), &rst);
        reply->writeInt32(rst ? 1 : 0);
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_REMOVE:
    {
        bool rtn = remove(data.readCString());
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_LSDIR:
    {
        std::vector<std::vector<char> > list;
        bool rtn = lsDir(data.readCString(), &list);

        reply->writeInt64(list.size());
        for (std::vector<std::vector<char> >::iterator it = list.begin();
            it != list.end(); it++) {
            reply->writeCString(addr(*it));
        }
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_ISFILE:
    {
        bool rst = false;
        bool rtn = isFile(data.readCString(), &rst);
        reply->writeInt32(rst ? 1 : 0);
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_ISDIR:
    {
        bool rst = false;
        bool rtn = isDir(data.readCString(), &rst);
        reply->writeInt32(rst ? 1 : 0);
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_MOVE:
    {
        const char * oldPath = data.readCString();
        const char * newPath = data.readCString();
        bool rtn = move(oldPath, newPath);
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_MKDIR:
    {
        bool rtn = mkdir(data.readCString());
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_WRITETINYFILE:
    {
        const char * path = data.readCString();
        bool append = data.readInt32() != 0;
        ssize_t len = data.readInt32();

        std::vector<unsigned char> bytes(len);
        data.read(addr(bytes), len);

        bool rtn = writeTinyFile(path, append, addr(bytes), len);
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_READTINYFILE:
    {
        std::vector<unsigned char> bytes;
        bool rtn = readTinyFile(data.readCString(), &bytes);
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        } else {
            reply->writeInt32(bytes.size());
            reply->write(addr(bytes), bytes.size());
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_ADDSERVICE:
    {
        const char * name = data.readCString();
        android::sp<IBinder> service = data.readStrongBinder();
        bool rtn = addService(name, service);
        reply->writeInt32(rtn ? 1 : 0);
        if (!rtn) {
            reply->writeInt32(errno);
        }
        return android::NO_ERROR;
    }
    break;

    case TR_CODE_GET_SET_REBOOT_CLEAR_VARIABLE:
    {
        const char * name = data.readCString();
        bool modify = data.readInt32() != 0;

        const char * newValue = NULL;
        bool newValueNotNull = data.readInt32() != 0;
        if (newValueNotNull) {
            newValue = data.readCString();
        }

        // The local method always returns true.
        std::vector<char> oldValue;
        getSetRebootClearVariable(name, &oldValue, modify, newValue);

        if (!oldValue.size()) {
            reply->writeInt32(0);
        } else {
            reply->writeInt32(1);
            reply->writeCString(addr(oldValue));
        }

        return android::NO_ERROR;
    }
    break;

    }


    return BnInterface<IPriviledgedProcService>::onTransact(code, data, reply, flags);
}

bool BpPriviledgedProcService::exists(const char * path, bool * pRst) const {

    *pRst = false;

    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(path);

    android::status_t trRst = remote()->transact(TR_CODE_EXISTS, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::exists: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
    }

    *pRst = reply.readInt32() != 0;
    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;
}


bool BpPriviledgedProcService::remove(const char * path) const {

    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(path);

    android::status_t trRst = remote()->transact(TR_CODE_REMOVE, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::remove: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
    }

    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;

}

bool BpPriviledgedProcService::lsDir(const char * path, std::vector<std::vector<char> > * pList) const {

    pList->clear();

    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(path);

    android::status_t trRst = remote()->transact(TR_CODE_LSDIR, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::lsDir: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
    }

    int64_t size = reply.readInt64();
    pList->resize(size);
    for (int64_t i = 0; i < size; i++) {
        const char * str = reply.readCString();
        pList->at(i).resize(strlen(str) + 1);
        strcpy(addr(pList->at(i)), str);
    }
    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;

}

bool BpPriviledgedProcService::isFile(const char * path, bool * pRst) const {
    *pRst = false;

    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(path);

    android::status_t trRst = remote()->transact(TR_CODE_ISFILE, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::isFile: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
    }

    *pRst = reply.readInt32() != 0;
    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;
}

bool BpPriviledgedProcService::isDir(const char * path, bool * pRst) const {
    *pRst = false;

    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(path);

    android::status_t trRst = remote()->transact(TR_CODE_ISDIR, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::isDir: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
    }

    *pRst = reply.readInt32() != 0;
    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;
}

bool BpPriviledgedProcService::move(const char * oldPath, const char * newPath) const {
    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(oldPath);
    data.writeCString(newPath);

    android::status_t trRst = remote()->transact(TR_CODE_MOVE, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::move: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
    }

    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;
}

bool BpPriviledgedProcService::mkdir(const char * path) const {
    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(path);

    android::status_t trRst = remote()->transact(TR_CODE_MKDIR, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::mkdir: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
    }

    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;
}

bool BpPriviledgedProcService::writeTinyFile(const char * path, bool append, const unsigned char * bytes, ssize_t len) const {
    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(path);
    data.writeInt32(append ? 1 : 0);
    data.writeInt32(len);
    data.write(bytes, len);

    android::status_t trRst = remote()->transact(TR_CODE_WRITETINYFILE, data, &reply);
    if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::writeTinyFile: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
    }

    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;
}

bool BpPriviledgedProcService::readTinyFile(const char * path, std::vector<unsigned char> * pBytes) const {
    pBytes->clear();

    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(path);

    android::status_t trRst = remote()->transact(TR_CODE_READTINYFILE, data, &reply);
    if (trRst != android::NO_ERROR) {
       LOGE("BpPriviledgedProcService::readTinyFile: Transaction failed with code: %lu. ",
           (unsigned long)trRst);
       errno = EREMOTEIO;
       return false;
    }

    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
       errno = reply.readInt32();
    } else {
        pBytes->resize(reply.readInt32());
        reply.read(addr(*pBytes), pBytes->size());
    }
    return rtn;
}

bool BpPriviledgedProcService::addService(const char * name,
                                          const android::sp<android::IBinder> & service) const {

    android::Parcel data, reply;

    data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
    data.writeCString(name);
    data.writeStrongBinder(service);

    android::status_t trRst = remote()->transact(TR_CODE_ADDSERVICE, data, &reply);
    if (trRst != android::NO_ERROR) {
       LOGE("BpPriviledgedProcService::addService: Transaction failed with code: %lu. ",
           (unsigned long)trRst);
       errno = EREMOTEIO;
       return false;
    }

    bool rtn = reply.readInt32() != 0;
    if (!rtn) {
        errno = reply.readInt32();
    }
    return rtn;
}

bool BpPriviledgedProcService::getSetRebootClearVariable(const char * name,
                                                         std::vector<char> * pOldValue,
                                                         bool modify,
                                                         const char * pNewValue) {

     pOldValue->clear();

     android::Parcel data, reply;

     data.writeInterfaceToken(IPriviledgedProcService::getInterfaceDescriptor());
     data.writeCString(name);
     data.writeInt32(modify ? 1 : 0);
     data.writeInt32(pNewValue ? 1 : 0);
     if (pNewValue) {
         data.writeCString(pNewValue);
     }

     android::status_t trRst = remote()->transact(TR_CODE_GET_SET_REBOOT_CLEAR_VARIABLE, data, &reply);
     if (trRst != android::NO_ERROR) {
        LOGE("BpPriviledgedProcService::getSetRebootClearVariable: Transaction failed with code: %lu. ",
            (unsigned long)trRst);
        errno = EREMOTEIO;
        return false;
     }

     bool oldValueNotNull = reply.readInt32() != 0;
     if (oldValueNotNull) {
         const char * oldValue = reply.readCString();
         pOldValue->resize(::strlen(oldValue) + 1);
         ::strcpy(addr(*pOldValue), oldValue);
     }

     return true;
}
