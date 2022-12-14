/**
 * parse memory info.
 */
 #pragma once
#include "common/Common.h"

namespace android {

namespace syspressure {

class FileReadParam {
friend class FileRead;
public:
    FileReadParam(const char* const filename) : mFilename(filename), mFd(-1){

    }
    int getFd() {
        return mFd;
    }
private:
    const char* const mFilename;
    int mFd;
};

class FileRead {
public:
    FileRead();
    ~FileRead();
    char* rereadFile(FileReadParam *param);
    ssize_t readAllContent(int fd, char *buf, size_t max_len);
    void closeFile(FileReadParam *param);
private:
    /**
    * start with page-size buffer and increase if needed
    **/
    char* mpReadBuf;
    ssize_t mReadBufSize;
    // std::mutex mMutex;
};

} // namespace syspressure

} // namespace android
