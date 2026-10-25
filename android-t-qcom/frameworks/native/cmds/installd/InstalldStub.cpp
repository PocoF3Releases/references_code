//
// Created by mi on 21-7-6.
//
#include <iostream>
#include <dlfcn.h>
#include <android-base/logging.h>
#include <log/log.h>
#include "InstalldStub.h"

void* InstalldStub::LibInstalldImpl = NULL;
IInstalldStub* InstalldStub::ImplInstance = NULL;
std::mutex InstalldStub::StubLock;

IInstalldStub* InstalldStub::GetImplInstance() {
        std::lock_guard<std::mutex> lock(StubLock);
        if (!ImplInstance) {
            LibInstalldImpl = dlopen(LIBPATH, RTLD_LAZY);
            if (LibInstalldImpl) {
                Create* create = (Create *)dlsym(LibInstalldImpl, "create");
                if (create) {
                    ImplInstance = create();
                } else {
                    LOG(DEBUG) << "InstalldStub : dlsym failed create : reason" << dlerror();
                }
            }
        }
        return ImplInstance;
}

void InstalldStub::DestroyImplInstance() {
    std::lock_guard<std::mutex> lock(StubLock);
    if (LibInstalldImpl) {
        Destroy* destroy = (Destroy *) dlsym (LibInstalldImpl, "destroy");
        destroy(ImplInstance);
        dlclose(ImplInstance);
        LibInstalldImpl = NULL;
        ImplInstance = NULL;
        LOG(DEBUG) << "InstalldStub : destroy installd instance success";
    }
}

bool InstalldStub::moveData(const std::string& src, const std::string& dst,
                            int32_t uid, int32_t gid, const std::string& seInfo, int32_t flag) {
    bool ret = false;
    IInstalldStub* Istub = GetImplInstance();
    if (Istub) {
        ret = Istub->moveData(src, dst, uid, gid, seInfo, flag);
    }
    return ret;
}

void InstalldStub::dexopt_async(const std::string package_name, pid_t pid, int secondaryId) {
    LOG(WARNING) << "dexopt_async: " << package_name << " secondaryId is " << secondaryId;
    IInstalldStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->dexopt_async(package_name, pid, secondaryId);
    }
}

void InstalldStub::send_mi_dexopt_result(const char* pkg_name, int secondaryId, int result) {
    LOG(WARNING) << "no_dexopt_async: " << pkg_name << " secondaryId is " << secondaryId;
    IInstalldStub* Istub = GetImplInstance();
    if (Istub) {
        Istub->send_mi_dexopt_result(pkg_name, secondaryId, result);
    }
}