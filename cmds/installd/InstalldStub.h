//
// Created by mi on 21-7-6.
//

#ifndef ANDROID_INSTALLD_INSTALLDSTUB_H
#define ANDROID_INSTALLD_INSTALLDSTUB_H
#include "IInstalldStub.h"
#include <mutex>

#define LIBPATH "/system_ext/lib64/libinstalldimpl.so"

using namespace android::installd;

class InstalldStub {
private:
        InstalldStub() {}
        static void *LibInstalldImpl;
        static IInstalldStub *ImplInstance;
        static IInstalldStub *GetImplInstance();
        static void DestroyImplInstance();
        static std::mutex StubLock;
public:
        virtual ~InstalldStub() {}
        static bool moveData(const std::string& src, const std::string& dst,
                            int32_t uid, int32_t gid, const std::string& seInfo, int32_t flag);
        static void dexopt_async(const std::string package_name, pid_t pid, int secondaryId);
        static void send_mi_dexopt_result(const char* pkg_name, int secondaryId, int result);

};

#endif //ANDROID_INSTALLD_INSTALLDSTUB_H
