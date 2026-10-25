/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight zip common internal  head file
 *
 */

#ifndef MISIGHT_PLUGINS_COMMON_ZIP_BASE_H
#define MISIGHT_PLUGINS_COMMON_ZIP_BASE_H

#include <string>
#include <utils/RefBase.h>

#include "minizip/zip.h"

namespace android {
namespace MiSight {
class ZipBase : public RefBase {
public:
    ZipBase();
    ~ZipBase();
    bool OpenNewFileInZip(zipFile zip_file, const std::string &strPath);
    bool TimeToZipFileInfo(zip_fileinfo &zipInfo);
    bool CloseNewFileEntry(zipFile zip_file);
    zipFile OpenZip(const std::string &fileNameUtf8, int appendFlag);
};
} // namespace MiSight
} // namespace android
#endif

