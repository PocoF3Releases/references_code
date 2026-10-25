#ifndef ANDROID_IPRIVILEDGEDPROCSERVICE_H
#define ANDROID_IPRIVILEDGEDPROCSERVICE_H

#include <binder/IInterface.h>

#include <vector>
#include <string>

#define PRIVILEDGED_PROC_PERMISSION "com.xiaomi.permission.fdpp"

class IPriviledgedProcService : public android::IInterface {
public:
    DECLARE_META_INTERFACE(PriviledgedProcService)

public:
    virtual bool exists(const char * path, bool * pRst) const = 0;
    virtual bool remove(const char * path) const = 0;
    virtual bool lsDir(const char * path, std::vector<std::vector<char> > * pList) const = 0;
    virtual bool isFile(const char * path, bool * pRst) const = 0;
    virtual bool isDir(const char * path, bool * pRst) const = 0;
    virtual bool move(const char * oldPath, const char * newPath) const = 0;
    virtual bool mkdir(const char * path) const = 0;
    virtual bool writeTinyFile(const char * path, bool append, const unsigned char * bytes, ssize_t len) const = 0;
    virtual bool readTinyFile(const char * path, std::vector<unsigned char> * pBytes) const = 0;
    virtual bool addService(const char * name, const android::sp<android::IBinder> & service) const = 0;
    virtual bool getSetRebootClearVariable(const char * name, std::vector<char> * pOldValue, bool modify, const char * pNewValue) = 0;
};


#endif //ANDROID_IPRIVILEDGEDPROCSERVICE_H
