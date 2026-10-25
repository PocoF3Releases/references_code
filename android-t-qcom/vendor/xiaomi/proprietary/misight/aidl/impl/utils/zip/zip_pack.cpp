/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight zip pack implements file
 *
 */

#include "zip_pack.h"

#include <algorithm>

#include <android-base/logging.h>

#include <utils/FileUtil.h>
#include <utils/StringUtil.h>
#include <utils/TimeUtil.h>

namespace aidl::vendor::xiaomi::hardware::misight {

// Numbers of pending entries that trigger writting them to the ZIP file.
constexpr size_t MAX_PENDING_FILE_COUNT = 50;
const int ZIP_BUF_SIZE = 8192;
const std::string SEPARATOR = "/";

ZipPack::ZipPack()
    : zipFile_(nullptr)
{
    LOG(DEBUG) << "zip pack construct";
}

ZipPack::~ZipPack()
{
    pendingFiles_.clear();
    Close();
    LOG(DEBUG) << "zip pack ~construct end";
}

bool ZipPack::CreateZip(const std::string& filePath)
{
    // zipFile_ not nullptr, exist not close zip file
    if (zipFile_) {
        Close();
    }

    zipFile_ = OpenZip(filePath, APPEND_STATUS_CREATE);
    if (!zipFile_) {
        LOG(ERROR) << "Couldn't create ZIP file at path " << filePath.c_str() << " " << zipFile_;
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
        LOG(ERROR) << "force flush files failed";
    }
    if (zipClose(zipFile_, nullptr) != ZIP_OK) {
        LOG(ERROR) << "close zip file failed";
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
                    LOG(WARNING) << "Failed to write file " << absolutePath;
                    failed++;
                    continue;
                }
            } else {
                // Missing file or directory case.
                if (!AddDirectoryEntryToZip(relativePath)) {
                    LOG(WARNING) << "Failed to write directory " << absolutePath.c_str();
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
    LOG(INFO) << "start..." << filePath;
    int num_bytes;
    char buf[ZIP_BUF_SIZE];

    if (!FileUtil::FileExists(filePath)) {
        LOG(INFO) << "file not exist " << filePath;
        return false;
    }

    FILE *fp = fopen(filePath.c_str(), "rb");
    if (fp == nullptr) {
        LOG(INFO) << "open file " << filePath << " failed";
        return false;
    }

    while (!feof(fp)) {
        num_bytes = fread(buf, 1, ZIP_BUF_SIZE, fp);
        if (num_bytes > 0) {
            if (zipWriteInFileInZip(zipFile_, buf, num_bytes) != ZIP_OK) {
                LOG(INFO) << "Could not write data to zip for path: " << filePath;
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
} // namespace aidl::vendor::xiaomi::hardware::misight

