#include "PriviledgedProcService.h"

#include "../../../common/native/include/util_common.h"
#include "../common/log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

#undef MY_LOG_TAG
#define MY_LOG_TAG "PriviledgedProcService"

static bool existsPrivate(const char * path, bool * pRst) {
    *pRst = false;

    struct stat st;
    if (::lstat(path, &st) == 0) {
        *pRst = true;
        return true;
    }
    if (errno == ENOENT || errno == ENOTDIR) {
        *pRst = false;
        return true;
    }
    LOGE("existsPrivate: Can not stat %s, error: %s. ", path, strerror(errno));
    return false;
}

static bool removePrivate(const char * path) {
    if (::remove(path) == 0) {
        return true;
    }
    LOGE("removePrivate: Can not remove %s, error: %s. ", path, strerror(errno));
    return false;
}

static bool lsDirPrivate(const char * path, std::vector<std::vector<char> > * pList) {

   pList->clear();

   bool success = false;
   DIR * dir = NULL;
   do {
       dir = ::opendir(path);
       if (dir == NULL) {
           LOGE("lsDirPrivate: Can not opendir %s, error: %s. ", path, strerror(errno));
           break;
       }

       dirent * ent = NULL;
       errno = 0;
       while ((ent = ::readdir(dir))) {
           pList->push_back(std::vector<char>());
           pList->back().resize(strlen(ent->d_name) + 1);
           strcpy(addr(pList->back()), ent->d_name);
       }
       if (errno != 0) {
           LOGE("lsDirPrivate: Can not readdir %s, error: %s. ", path, strerror(errno));
           break;
       }

       success = true;

   } while (false);

   if (dir) {
       if (::closedir(dir) != 0) {
           LOGE("lsDirPrivate: Can not closedir %s, error: %s. ", path, strerror(errno));
           success = false;
       }
   }

   return success;
}

static bool isFilePrivate(const char * path, bool * pRst) {
    *pRst = false;

    struct stat st;
    if (::lstat(path, &st) == 0) {
        *pRst = S_ISREG(st.st_mode);
        return true;
    }
//    if (errno == ENOENT || errno == ENOTDIR) {
//        *pRst = false;
//        return true;
//    }
    LOGE("isFilePrivate: Can not stat %s. error: %s. ", path, strerror(errno));
    return false;
}

static bool isDirPrivate(const char * path, bool * pRst) {
    *pRst = false;

    struct stat st;
    if (::lstat(path, &st) == 0) {
        *pRst = S_ISDIR(st.st_mode);
        return true;
    }
//    if (errno == ENOENT || errno == ENOTDIR) {
//        *pRst = false;
//        return true;
//    }
    LOGE("isDirPrivate: Can not stat %s. error: %s. ", path, strerror(errno));
    return false;
}

static bool movePrivate(const char * oldPath, const char * newPath) {
    if (::rename(oldPath, newPath) != 0) {
        LOGE("movePrivate: Can not rename %s to %s. error: %s. ", oldPath, newPath, strerror(errno));
        return false;
    }
    return true;
}

static bool mkdirPrivate(const char * path) {
    bool success = false;

    mode_t oldUmask = umask(0);

    if (::mkdir(path, 0700) != 0) {
        LOGE("mkdirPrivate: Can not mkdir %s. error: %s. ", path, strerror(errno));
        success = false;
    } else {
        success = true;
    }

    umask(oldUmask);

    return success;
}

static bool writeTinyFilePrivate(const char * path, bool append, const unsigned char * bytes, ssize_t len) {
    int openflags = O_WRONLY | O_CREAT | O_NOFOLLOW;
    openflags |= append ? O_APPEND : O_TRUNC;

    mode_t oldUmask = umask(0);

    bool success = false;
    int fd = -1;
    do {

        fd = TEMP_FAILURE_RETRY(::open(path, openflags, S_IRUSR | S_IWUSR | S_IXUSR));
        if (fd == -1) {
            LOGE("writeTinyFilePrivate: Can not open %s. error: %s. ", path, strerror(errno));
            break;
        }

        ssize_t offset = 0;
        while (offset < len) {
            ssize_t written = TEMP_FAILURE_RETRY(::write(fd, bytes + offset, len - offset));
            if (written == -1) {
                LOGE("writeTinyFilePrivate: Can not write %s. error: %s. ", path, strerror(errno));
                break;
            }
            offset += written;
        }
        if (offset < len) { break; }

        if ( TEMP_FAILURE_RETRY(::fsync(fd)) != 0 ) {
            LOGE("writeTinyFilePrivate: Can not sync %s. error: %s. ", path, strerror(errno));
            break;
        }

        success = true;

    } while (false);

    if (fd != -1) {
        if (TEMP_FAILURE_RETRY(::close(fd)) != 0) {
            LOGE("writeTinyFilePrivate: Can not close %s. error: %s. ", path, strerror(errno));
            success = false;
        }
    }

    oldUmask = umask(oldUmask);

    return success;
}

static bool readTinyFilePrivate(const char * path, std::vector<unsigned char> * pBytes) {

    pBytes->clear();

    struct stat st;
    if (::lstat(path, &st) != 0) {
        LOGE("readTinyFilePrivate: Can not stat %s. error: %s. ", path, strerror(errno));
        return false;
    }

    if (!S_ISREG(st.st_mode)) {
        LOGE("readTinyFilePrivate: %s not a regular file. ", path);
        errno = EINVAL;
        return false;
    }

    bool success = false;
    int fd = -1;
    do {
        fd = TEMP_FAILURE_RETRY(::open(path, O_RDONLY | O_NOFOLLOW));
        if (fd == -1) {
            LOGE("readTinyFilePrivate: Can not open %s. error: %s. ", path, strerror(errno));
            break;
        }

        struct stat info;
        if (::fstat(fd, &info) != 0) {
            LOGE("readTinyFilePrivate: Can not stat %s. error: %s. ", path, strerror(errno));
            break;
        }

        ssize_t size = info.st_size;
        pBytes->resize(size);

        ssize_t offset = 0;
        while (offset < size) {
            ssize_t rd = TEMP_FAILURE_RETRY(::read(fd, addr(*pBytes) + offset, size - offset));
            if (rd == -1) {
                LOGE("readTinyFilePrivate: Can not read %s. error: %s. ", path, strerror(errno));
                break;
            }
            if (rd == 0) {
                LOGE("readTinyFilePrivate: Unexpected EOF encountered while reading %s. ", path);
                errno = ENODATA;
                break;
            }
            offset += rd;
        }
        if (offset < size) { break; }

        success = true;

    } while (false);

    if (fd != -1) {
        if (TEMP_FAILURE_RETRY(::close(fd)) != 0) {
            LOGE("readTinyFilePrivate: Can not close %s. error: %s. ", path, strerror(errno));
            success = false;
        }
    }

    return success;
}

static bool addServicePrivate(const char * name, const android::sp<android::IBinder> & service) {
    android::status_t error =
        android::defaultServiceManager()->addService(android::String16(name), service, false);

    if (error == android::NO_ERROR) {
        return true;
    }

    errno = -error;
    return false;
}

bool PriviledgedProcService::exists(const char * path, bool * pRst) const {
    return existsPrivate(path, pRst);
}

bool PriviledgedProcService::remove(const char * path) const {
    return removePrivate(path);
}

bool PriviledgedProcService::lsDir(const char * path, std::vector<std::vector<char> > * pList) const {
    return lsDirPrivate(path, pList);
}

bool PriviledgedProcService::isFile(const char * path, bool * pRst) const {
    return isFilePrivate(path, pRst);
}

bool PriviledgedProcService::isDir(const char * path, bool * pRst) const {
    return isDirPrivate(path, pRst);
}

bool PriviledgedProcService::move(const char * oldPath, const char * newPath) const {
    return movePrivate(oldPath, newPath);
}

bool PriviledgedProcService::mkdir(const char * path) const {
    return mkdirPrivate(path);
}

bool PriviledgedProcService::writeTinyFile(const char * path, bool append, const unsigned char * bytes, ssize_t len) const {
    return writeTinyFilePrivate(path, append, bytes, len);
}

bool PriviledgedProcService::readTinyFile(const char * path, std::vector<unsigned char> * pBytes) const {
    return readTinyFilePrivate(path, pBytes);
}

bool PriviledgedProcService::addService(const char * name, const android::sp<android::IBinder> & service) const {
    return addServicePrivate(name, service);
}

bool PriviledgedProcService::getSetRebootClearVariable(const char * name,
                                                       std::vector<char> * pOldValue,
                                                       bool modify,
                                                       const char * pNewValue) {
    // Do it with one map search operation.

    pOldValue->clear();

    if (!modify || !pNewValue) {

        std::map<std::string, std::string>::iterator it = mRebootClearValues.find(name);
        if (it != mRebootClearValues.end()) {
            const char * oldValue = it->second.c_str();
            pOldValue->resize(::strlen(oldValue) + 1);
            ::strcpy(addr(*pOldValue), oldValue);

            if (modify) {
                mRebootClearValues.erase(it);
            }
        }

        return true;
    }

    std::pair<std::map<std::string, std::string>::iterator, bool> rst =
        mRebootClearValues.insert(std::pair<std::string, std::string>(name, pNewValue));

    if (!rst.second) {
         const char * oldValue = rst.first->second.c_str();
         pOldValue->resize(::strlen(oldValue) + 1);
         ::strcpy(addr(*pOldValue), oldValue);

         rst.first->second = pNewValue;
    }

    return true;
}