#ifndef ANDROID_PRIVILEDGEDPROCSERVICE_H
#define ANDROID_PRIVILEDGEDPROCSERVICE_H

#include <binder/BinderService.h>
#include <BnPriviledgedProcService.h>

#include <vector>
#include <map>

class PriviledgedProcService :
    public android::BinderService<PriviledgedProcService>,
    public BnPriviledgedProcService {

public:
    static const char * getServiceName() { return "miui.fdpp"; }

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

private:
    std::map<std::string, std::string> mRebootClearValues;
};

#endif //ANDROID_PRIVILEDGEDPROCSERVICE_H
