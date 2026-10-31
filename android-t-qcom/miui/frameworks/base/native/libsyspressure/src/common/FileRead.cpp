#include "common/Common.h"
#include "common/FileRead.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "syspressure.FileRead"


namespace android {

namespace syspressure {

FileRead::FileRead() : mReadBufSize(PAGE_SIZE) {
    mpReadBuf = static_cast<char*>(malloc(mReadBufSize));
}

FileRead::~FileRead() {
    free(mpReadBuf);
}
/*
 * Read file content from the beginning up to max_len bytes or EOF
 * whichever happens first.
 */
ssize_t FileRead::readAllContent(int fd, char *buf, size_t max_len) {
    ssize_t ret = 0;
    off_t offset = 0;

    while (max_len > 0) {
        ssize_t r = TEMP_FAILURE_RETRY(pread(fd, buf, max_len, offset));
        if (r == 0) {
            break;
        }
        if (r == -1) {
            return -1;
        }
        ret += r;
        buf += r;
        offset += r;
        max_len -= r;
    }

    return ret;
}

/*
 * Read a new or already opened file from the beginning.
 * If the file has not been opened yet param->mFd should be set to -1.
 * To be used with files which are read often and possibly during high
 * memory pressure to minimize file opening which by itself requires kernel
 * memory allocation and might result in a stall on memory stressed system.
 */
char* FileRead::rereadFile(FileReadParam *param) {
    ssize_t size;
    char *new_buf = nullptr;

    if (param->mFd == -1) {
        param->mFd = TEMP_FAILURE_RETRY(open(param->mFilename, O_RDONLY | O_CLOEXEC));
        if (param->mFd < 0) {
            ALOG(LOG_ERROR, LOG_TAG, "%s open: %s", param->mFilename, strerror(errno));
            return nullptr;
        }
    }

    // std::lock_guard<std::mutex> locker(mMutex);
    while (true) {
        size = readAllContent(param->mFd, mpReadBuf, mReadBufSize - 1);
        if (size < 0) {
            ALOG(LOG_ERROR, LOG_TAG, "%s read: %s", param->mFilename, strerror(errno));
            closeFile(param);
            return nullptr;
        }
        if (size < mReadBufSize - 1) {
            break;
        }
        /*
         * Since we are reading /proc files we can't use fstat to find out
         * the real size of the file. Double the buffer size and keep retrying.
         */
        if ((new_buf = static_cast<char*>(realloc(mpReadBuf, mReadBufSize * 2))) == nullptr) {
            errno = ENOMEM;
            closeFile(param);
            return nullptr;
        }
        mpReadBuf = new_buf;
        mReadBufSize *= 2;
    }
    mpReadBuf[size] = 0;

    return mpReadBuf;
}

void FileRead::closeFile(FileReadParam *param) {
    if(param->mFd >= 0) {
        close(param->mFd);
        param->mFd = -1;
    }
}
} // namespace syspressure

} // namespace android