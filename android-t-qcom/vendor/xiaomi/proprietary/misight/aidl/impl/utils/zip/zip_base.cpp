/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight zip common internal  head file
 *
 */

#include "zip_base.h"

#include <android-base/logging.h>

#include <time.h>
#include <unistd.h>

#include <utils/TimeUtil.h>

namespace aidl::vendor::xiaomi::hardware::misight {
ZipBase::ZipBase()
{}

ZipBase::~ZipBase()
{}

bool ZipBase::OpenNewFileInZip(zipFile zip_file, const std::string &strPath)
{
    const unsigned long LANGUAGE_ENCODING_FLAG = 0x1 << 11;

    zip_fileinfo fileInfo = {};
    TimeToZipFileInfo(fileInfo);
    if (ZIP_OK != zipOpenNewFileInZip4(zip_file,    // file
        strPath.c_str(),    // filename
        &fileInfo,    // zip_fileinfo
        nullptr,    // extrafield_local,
        0,   // size_extrafield_local
        nullptr,    // extrafield_global
        0,    // size_extrafield_global
        nullptr,    // comment
        Z_DEFLATED,    // method
        Z_DEFAULT_COMPRESSION,    // level:default Z_DEFAULT_COMPRESSION
        0,    // raw
        -MAX_WBITS,    // windowBits
        DEF_MEM_LEVEL,    // memLevel: default DEF_MEM_LEVEL
        Z_DEFAULT_STRATEGY,    // strategy:default Z_DEFAULT_STRATEGY
        NULL,    // password
        0,    // crcForCrypting
        0,    // versionMadeBy
        LANGUAGE_ENCODING_FLAG)) {    // flagBase
        LOG(ERROR) << "zipOpenNewFileInZip4 failed";
        return false;
    }
    return true;
}


bool ZipBase::TimeToZipFileInfo(zip_fileinfo &zipInfo)
{
    time_t rawtime = time(nullptr);
    struct tm *fileTime = localtime(&rawtime);
    zipInfo.tmz_date.tm_year = (int)fileTime->tm_year + 1900;
    zipInfo.tmz_date.tm_mon = (int)fileTime->tm_mon + 1;
    zipInfo.tmz_date.tm_mday = (int)fileTime->tm_mday;
    zipInfo.tmz_date.tm_hour = (int)fileTime->tm_hour;
    zipInfo.tmz_date.tm_min = (int)fileTime->tm_min;
    zipInfo.tmz_date.tm_sec = (int)fileTime->tm_sec;
    return true;
}


bool ZipBase::CloseNewFileEntry(zipFile zip_file)
{
    return zipCloseFileInZip(zip_file) == ZIP_OK;
}

zipFile ZipBase::OpenZip(const std::string &fileNameUtf8, int appendFlag)
{
    return zipOpen2(fileNameUtf8.c_str(), appendFlag, nullptr, nullptr);
}

} // namespace aidl::vendor::xiaomi::hardware::misight


