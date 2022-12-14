#ifndef DELEGATE_INTERFACE_H_
#define DELEGATE_INTERFACE_H_

#include <string>

#include <ziparchive/zip_archive.h>
#include <zip_archive_private.h>

namespace mi_update_android {

struct img_table {
    std::string partition;
    std::string name;
};

struct UpdateEntry{
    struct img_table table_info;
    ZipEntry entry;
};

class DelegateInterface {
public:
    virtual ~DelegateInterface() = default;

protected:
    virtual bool OpenPackage(const std::string& package_path) = 0;
    virtual bool FindImage(ZipArchiveHandle& handle) = 0;
    virtual bool WriteImgToPartition(struct UpdateEntry& update_entry) = 0;
};

} //namespace mi_update_android

#endif //DELEGATE_INTERFACE_H_
