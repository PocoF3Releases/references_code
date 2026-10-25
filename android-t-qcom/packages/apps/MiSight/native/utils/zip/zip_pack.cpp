/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight zip pack implements file
 *
 */

#include "zip_pack.h"

#include <algorithm>

#include "file_util.h"
#include "log.h"
#include "string_util.h"
#include "time_util.h"

namespace android {
namespace MiSight {
// Numbers of pending entries that trigger writting them to the ZIP file.
constexpr size_t MAX_PENDING_FILE_COUNT = 50;
const int ZIP_BUF_SIZE = 8192;
const std::string SEPARATOR = "/";

ZipPack::ZipPack()
    : zipFile_(nullptr)
{
    //MISIGHT_LOGD("zip pack construct");
}

ZipPack::~ZipPack()
{
    pendingFiles_.clear();
    Close();
    //MISIGHT_LOGD("zip pack ~construct end");
}

bool ZipPack::CreateZip(const std::string& filePath)
{
    // zipFile_ not nullptr, exist not close zip file
    if (zipFile_) {
        Close();
    }

    zipFile_ = OpenZip(filePath, APPEND_STATUS_CREATE);
    if (!zipFile_) {
        MISIGHT_LOGE("Couldn't create ZIP file at path %s %p", filePath.c_str(), zipFile_);
        return false;
    }
    return true;
}

bool ZipPack::AddFiles(const std::vector<std::string> &paths)
{
    if (!zipFile_) {
        return false;
    }
    pendingFiles_.insert(pendingFiles_.end(), paths.begin(), paths.end());
    return FlushFilesIfNeeded(false);

}

void ZipPack::Close()
{
    if (!zipFile_) {
        return;
    }
    if (!FlushFilesIfNeeded(true)) {
        MISIGHT_LOGE("force flush files failed");
    }
    if (zipClose(zipFile_, nullptr) != ZIP_OK) {
        MISIGHT_LOGE("close zip file failed")
    }
    zipFile_ = nullptr;
    return;
}

bool ZipPack::FlushFilesIfNeeded(bool force)
{
    if (pendingFiles_.size() < MAX_PENDING_FILE_COUNT && !force) {
        return true;
    }
    int failed = 0;
    int exeCnt = 0;
    while (pendingFiles_.size() >= MAX_PENDING_FILE_COUNT || (force && !pendingFiles_.empty())) {
        size_t entry_count = std::min(pendingFiles_.size(), MAX_PENDING_FILE_COUNT);
        std::vector<std::string> absolutePaths;

        absolutePaths.insert(absolutePaths.begin(), pendingFiles_.begin(), pendingFiles_.begin() + entry_count);
        pendingFiles_.erase(pendingFiles_.begin(), pendingFiles_.begin() + entry_count);

        // add file or dir into zip
        for (size_t i = 0; i < absolutePaths.size(); i++) {
            std::string absolutePath = absolutePaths[i];
            std::string relativePath = absolutePath;
            exeCnt++;
            if (StringUtil::StartsWith(absolutePaths[i], SEPARATOR)) {
                relativePath = absolutePath.substr(1, absolutePath.size());
            }
            if (!FileUtil::IsDirectory(absolutePath)) {
                if (!AddFileEntryToZip(relativePath, absolutePath)) {
                    MISIGHT_LOGW("Failed to write file %s", absolutePath.c_str());
                    failed++;
                    continue;
                }
            } else {
                // Missing file or directory case.
                if (!AddDirectoryEntryToZip(relativePath)) {
                    MISIGHT_LOGW("Failed to write directory %s", absolutePath.c_str());
                    failed++;
                    continue;
                }
            }
        }
    }

    if ((failed != 0) && (failed == exeCnt)) {
        return false;
    }
    return true;
}


bool ZipPack::AddFileEntryToZip(const std::string& relativePath, const std::string& absolutePath)
{
    if (!OpenNewFileInZip(zipFile_, relativePath)) {
        return false;
    }
    bool success = AddFileContentToZip(absolutePath);
    if (!CloseNewFileEntry(zipFile_)) {
        return false;
    }

    return success;
}

bool ZipPack::AddDirectoryEntryToZip(const std::string& path)
{
    std::string filePath = path;
    if (!StringUtil::EndsWith(filePath, SEPARATOR)) {
        filePath += "/";
    }
    return OpenNewFileInZip(zipFile_, filePath) && CloseNewFileEntry(zipFile_);
}


bool ZipPack::AddFileContentToZip(const std::string &filePath)
{
    int num_bytes;
    char buf[ZIP_BUF_SIZE];

    if (!FileUtil::FileExists(filePath)) {
        MISIGHT_LOGD("file not exist %s", filePath.c_str());
        return false;
    }

    FILE *fp = fopen(filePath.c_str(), "rb");
    if (fp == nullptr) {
        MISIGHT_LOGI("open file %s failed", filePath.c_str());
        return false;
    }

    while (!feof(fp)) {
        num_bytes = fread(buf, 1, ZIP_BUF_SIZE, fp);
        if (num_bytes > 0) {
            if (zipWriteInFileInZip(zipFile_, buf, num_bytes) != ZIP_OK) {
                MISIGHT_LOGI("Could not write data to zip for path:%s ", filePath.c_str());
                fclose(fp);
                fp = nullptr;
                return false;
            }
        }
    }
    fclose(fp);
    fp = nullptr;
    return true;
}
} // namespace MiSight
} // namespace android

