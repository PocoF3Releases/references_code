#ifndef MI_UPDATER_H
#define MI_UPDATER_H

#include <unistd.h>
#include <memory>
#include <string>
#include <vector>
#include <deque>

#include <android-base/unique_fd.h>
#include <ziparchive/zip_archive.h>
#include <zip_archive_private.h>

#include <cutils/properties.h>

#include <android-base/mapped_file.h>

#include "delegate_interface.h"

namespace mi_update_android {

#define PKG_SETUP_PROP "persist.sys.cota.package"

#ifndef O_BINARY
#define O_BINARY 0
#endif


//定义需要更新的分区
const std::string OpcustPtn{"/dev/block/bootdevice/by-name/opcust"};
const std::string OpconfigPtn{"/dev/block/bootdevice/by-name/opconfig"};

class MiUpdater: public DelegateInterface {
public:
    MiUpdater(const std::string zip_path);
    ~MiUpdater();

    static const char* status[];

    bool Run();

protected:
    bool OpenPackage(const std::string& package_path) override;
    bool FindImage(ZipArchiveHandle& handle) override;
    bool WriteImgToPartition(struct UpdateEntry& update_entry) override;

private:

    android::base::unique_fd zip_fd_;

    std::unique_ptr<android::base::MappedFile> file_map_;
    const std::string zip_path_;

    ZipArchiveHandle package_handle_{nullptr};
    std::deque<struct UpdateEntry> update_entrys_;

};

} //namespace mi_update_android

#endif //MI_UPDATER_H
