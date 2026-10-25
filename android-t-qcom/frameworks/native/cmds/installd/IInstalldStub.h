//
// Created by mi on 21-7-6.
//

#ifndef ANDROID_IINSTALLD_IINSTALLDSTUB_H
#define ANDROID_IINSTALLD_IINSTALLDSTUB_H
#include <inttypes.h>
#include <unistd.h>
#include <binder/Status.h>
#include <android-base/macros.h>

#define INSTALLD_IMPL_ERROR (-10)

namespace android {
namespace installd {
class IInstalldStub {
public:
        virtual ~IInstalldStub() {}
        virtual bool moveData(const std::string& src, const std::string& dst,
                            int32_t uid, int32_t gid, const std::string& seInfo, int32_t flag);
        virtual void dexopt_async(const std::string package_name, pid_t pid, int secondaryId);
        virtual void send_mi_dexopt_result(const char* pkg_name, int secondaryId, int result);
};
typedef IInstalldStub* Create();
typedef void Destroy(IInstalldStub *);
}
}
#endif //ANDROID_IINSTALLD_IINSTALLDSTUB_H
