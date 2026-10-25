#include <sys/types.h>
#include <sys/stat.h>

#include <android-base/logging.h>
#include <android-base/mapped_file.h>

#include "otautil/dirutil.h"
#include "otautil/error_code.h"
#include "otautil/sysutil.h"

#include "otapackage_verifier.h"

#include "mi_updater.h"

namespace mi_update_android {

namespace {

struct img_table table[] = {
    {.partition = OpcustPtn, .name = "opcust.img"},
    {.partition = OpconfigPtn, .name = "opconfig.img"},
};

}

const char* MiUpdater::status[] = {"failed", "success"};

MiUpdater::MiUpdater(const std::string zip_path)
                    : zip_path_(zip_path) {}

MiUpdater::~MiUpdater() {

    if(close(zip_fd_.release()) != 0) {
        PLOG(ERROR) << "close of \"" << zip_path_ << "\" failed";
    }

    if(package_handle_) {
        CloseArchive(package_handle_);
    }
}

bool MiUpdater::Run() {

    //校验包
    int verify_err = verify_otapackage(zip_path_);
    if(verify_err != 0) {
        LOG(ERROR) << "verify package faild";
        return false;
    }

    //打开package
    if(!OpenPackage(zip_path_)) {
        LOG(ERROR) << "open package faild";
        return false;
    }

    //写img到分区
    while(!update_entrys_.empty()){
        struct UpdateEntry cur_update_entry = update_entrys_.front();
        update_entrys_.pop_front();
        LOG(INFO) << "begin write " << cur_update_entry.table_info.name
                  << " to " << cur_update_entry.table_info.partition;

        if(!WriteImgToPartition(cur_update_entry)) {
            LOG(ERROR) << "faild to write " << cur_update_entry.table_info.name
                       << " to " << cur_update_entry.table_info.partition;
            return false;
        }
    }

    return true;
}

bool MiUpdater::OpenPackage(
                        const std::string& package_path) {
    android::base::unique_fd
                    pkg_fd(open(package_path.c_str(), O_RDONLY | O_BINARY));

    zip_fd_ = std::move(pkg_fd);

    if(zip_fd_ == -1) {
        LOG(ERROR) << "open " << package_path << " faild";
        return false;
    }

    struct stat sb;
    if(fstat(zip_fd_, &sb) != 0) {
        LOG(ERROR) << "failed on fstat";
        return false;
    }

    file_map_ = android::base::MappedFile::FromFd(zip_fd_, 0,
                                                static_cast<size_t>(sb.st_size), PROT_READ);

    int open_err = OpenArchiveFromMemory(file_map_->data(),
                                    file_map_->size(), package_path.c_str(), &package_handle_);

    if(open_err != 0) {
        LOG(ERROR) << "faild to open package " << package_path
                   << ": " << ErrorCodeString(open_err);
        CloseArchive(package_handle_);
        return false;
    }

    if(!FindImage(package_handle_)) {
        LOG(ERROR) << "can't to find imge in " << zip_path_;
        return false;
    }

    return true;
}

bool MiUpdater::FindImage(ZipArchiveHandle& handle) {
    ZipEntry entry;
    for(int i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
        std::string img_name = table[i].name;

        //查找package中包含的目标imge
        int find_err = FindEntry(handle, img_name, &entry);
        if(find_err != 0) {
            //LOG(ERROR) << "no " << img_name << " in package";
            continue;
            //return false;
        }

        LOG(INFO) << "find " << img_name << " in package";
        struct UpdateEntry update_entry = {
            .table_info = table[i],
            .entry = entry,
        };
        update_entrys_.emplace_back(update_entry);
    }

    if(update_entrys_.empty()){
        LOG(ERROR) << "faild to find image in " << zip_path_;
        return false;
    }
    return true;
}

bool MiUpdater::WriteImgToPartition(struct UpdateEntry& update_entry) {

    std::string dest_path = update_entry.table_info.partition;
    std::string img_name = update_entry.table_info.name;
    ZipEntry entry = update_entry.entry;

    android::base::unique_fd dest_fd(TEMP_FAILURE_RETRY(
                    open(dest_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)));
    if(dest_fd == -1) {
        PLOG(ERROR) << "can't open " << dest_path << " for write";
        return false;
    }


    int32_t ret = ExtractEntryToFile(package_handle_, &entry, dest_fd);
    if(ret != 0) {
        LOG(ERROR) << " Failed to extract entry \"" << img_name << "\" ("
                   << entry.uncompressed_length << " bytes) to \"" << dest_path
                   << "\": " << ErrorCodeString(ret);
        return false;
    }

    if(fsync(dest_fd) == -1) {
        PLOG(ERROR) << "fsync of \"" << dest_path << "\" failed";
        return false;
    }

    if(close(dest_fd.release()) != 0) {
        PLOG(ERROR) << "close of \"" << dest_path << "\" failed";
        return false;
    }

    LOG(INFO) << "Write " << img_name <<" to "<< dest_path << " complete.";
    return true;
}

} //namespace mi_update_android
