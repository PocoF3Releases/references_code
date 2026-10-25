/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight zip common internal  head file
 *
 */

#include <string>
#include <utils/RefBase.h>

#include "minizip/zip.h"

namespace aidl::vendor::xiaomi::hardware::misight {
class ZipBase : public android::RefBase {
public:
    ZipBase();
    ~ZipBase();
    bool OpenNewFileInZip(zipFile zip_file, const std::string &strPath);
    bool TimeToZipFileInfo(zip_fileinfo &zipInfo);
    bool CloseNewFileEntry(zipFile zip_file);
    zipFile OpenZip(const std::string &fileNameUtf8, int appendFlag);
};
} // namespace aidl::vendor::xiaomi::hardware::misight

