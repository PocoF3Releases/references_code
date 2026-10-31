//
// Created by mi on 21-7-6.
//

#ifndef ANDROID_INSTALLD_INSTALLDIMPL_H
#define ANDROID_INSTALLD_INSTALLDIMPL_H

#include <IInstalldStub.h>

namespace android {
namespace installd {
class InstalldImpl : public IInstalldStub {
public:
        virtual ~InstalldImpl(){}
        virtual bool moveData(const std::string& src, const std::string& dst,
                    int32_t uid, int32_t gid, const std::string& seInfo, int32_t flag);
        virtual void send_mi_dexopt_result(const char* pkg_name, int secondaryId, int result);
        virtual void dexopt_async(const std::string package_name, pid_t pid, int secondaryId);
private:
        static int  wait_child(pid_t pid);
        static int  get_dexopt_fd();
        static void write_mi_dexopt_result(const char* pkg_name, int secondaryId, int result);
        static void dex2oat_thread(const std::string packagename, pid_t pid, int secondaryId);
};

extern "C" IInstalldStub* create();
extern "C" void destroy(IInstalldStub* impl);
}
}
#endif //ANDROID_INSTALLD_INSTALLDIMPL_H
