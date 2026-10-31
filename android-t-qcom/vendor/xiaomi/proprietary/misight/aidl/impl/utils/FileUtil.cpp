/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight file utils in vendor aidl
 *
 */
#include <utils/FileUtil.h>
#include <utils/StringUtil.h>

#include <android-base/logging.h>

#include <cstdio>
#include <dirent.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace aidl::vendor::xiaomi::hardware::misight::FileUtil {
constexpr int MAX_FILE_LENGTH = 32 * 1024 * 1024; // 32m

bool CreateDirectory(const std::string& path, const mode_t& mode)
{
    if (access(path.c_str(), F_OK) == 0) {
        LOG(INFO) << path << " exist, no need create";
        return true;
    }

    std::string dirPath = "/" + StringUtil::TrimStr(path, '/') + "/";
    std::string::size_type n = 0;
    while ((n = dirPath.find("/", n + 1)) != std::string::npos) {
        std::string subPath = dirPath.substr(0, n);
        if (access(subPath.c_str(), F_OK) == -1) {
            if (mkdir(subPath.c_str(), FILE_ALL_MODE) == -1) { // 0777: create wiht all rights
                LOG(ERROR) << "mkdir " << subPath << "failed";
                return false;
            }
        }
    }
    return ChangeMode(dirPath, mode);
}

bool IsDirectory(const std::string &path)
{
    struct stat fsStat;
    if (stat(path.c_str(), &fsStat) != 0) {
        return false;
    }
    return S_ISDIR(fsStat.st_mode);
}

bool FileExists(const std::string& fileName)
{
    if (access(fileName.c_str(), F_OK) == 0) {
        return true;
    }
    return false;
}

bool ChangeMode(const std::string& fileName, const mode_t& mode)
{
    if (!FileExists(fileName)) {
        return false;
    }

    if (chmod(fileName.c_str(), mode)) {
        return false;
    }
    return true;
}

bool CreateDirectoryWithOwner(const std::string& path, int root, int system)
{
    if (!CreateDirectory(path)) {
        return false;
    }
    chown(path.c_str(), root, system);
    return true;
}

int32_t GetFileSize(const std::string& filePath)
{
    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) == 0) {
        return fileInfo.st_size;
    }
    return 0;
}

bool LoadStringFromFile(const std::string& filePath, std::string& content)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        return false;
    }

    file.seekg(0, std::ios::end);
    const long fileLength = static_cast<long>(file.tellg());
    if (fileLength > MAX_FILE_LENGTH) {
        return false;
    }

    content.clear();
    file.seekg(0, std::ios::beg);
    content.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    content = StringUtil::TrimStr(content, '\0');
    return true;
}

bool SaveStringToFd(int fd, const std::string& content)
{
    if (fd <= 0) {
        LOG(ERROR) << "invalid fd: " << fd;
        return false;
    }

    if (content.empty()) {
        LOG(INFO) << "content is empty, no need to save!";
        return true;
    }

    const long len = write(fd, content.c_str(), content.length());
    if (len < 0) {
        LOG(ERROR) << "write file failed! err: " << strerror(errno);
        return false;
    }

    if ((unsigned long)len != content.length()) {
        LOG(ERROR) << "the length write to file is not equal to fileLength!len:" << len << " fileLen:" << content.length();
        return false;
    }

    return true;
}

void DeleteFile(const std::string& filePath)
{

    if (!FileExists(filePath)) {
        return;
    }
    if (remove(filePath.c_str()) != 0) {
        LOG(ERROR) << "remove file " << filePath << " err: " << strerror(errno);
    }
}


bool MoveFile(const std::string& srcFile, const std::string& dstFile)
{
    if (!FileExists(srcFile)) {
        return false;
    }
    if (rename(srcFile.c_str(), dstFile.c_str()) != 0) {
        LOG(ERROR) << "move file " << srcFile << " to " << dstFile << " failed, err:" << strerror(errno);
        return false;
    }
    return true;
}


bool SaveStringToFile(const std::string& filePath, const std::string& content, bool truncated)
{
    if (content.empty()) {
        return true;
    }

    std::ofstream file;
    if (truncated) {
        file.open(filePath.c_str(), std::ios::out | std::ios::trunc);
    } else {
        file.open(filePath.c_str(), std::ios::out | std::ios::app);
    }

    if (!file.is_open()) {
        return false;
    }

    file.write(content.c_str(), content.length());
    if (file.fail()) {
        return false;
    }
    return true;
}

bool IsFileLatest(const std::string& srcFile, const std::string& dstFile)
{
    struct stat srcState;
    struct stat dstState;
    bool srcExist = FileExists(srcFile);
    bool dstExist = FileExists(dstFile);
    if (srcExist && dstExist) {
        if (stat(srcFile.c_str(), &srcState) != 0 || stat(dstFile.c_str(), &dstState) != 0) {
            return false;
        }
        return (srcState.st_mtime >= dstState.st_mtime);
    } else if (dstExist) {
        return false;
    } else {
        return true;
    }
}

size_t GetDirectorySize(std::string& dirString)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    size_t totalSize = 0;
    const char *dir = dirString.c_str();

    if ((dp = opendir(dir)) == NULL) {
        LOG(ERROR) << "Cannot open dir: " << dir;
        return -1;
    }

    lstat(dir, &statbuf);
    totalSize += statbuf.st_size;

    while ((entry = readdir(dp)) != NULL) {
        char subdir[300];
        sprintf(subdir, "%s/%s", dir, entry->d_name);
        lstat(subdir, &statbuf);

        if (S_ISDIR(statbuf.st_mode)) {
            if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name))
                continue;
            std::string subdirStr = subdir;

            size_t subDirSize = GetDirectorySize(subdirStr);
            totalSize += subDirSize;
        } else {
            totalSize += statbuf.st_size;
        }
    }

    closedir(dp);
    return totalSize;
}

int DeleteDirectoryOrFile(const char *path, size_t *deleted_size) {
   DIR *d = opendir(path);
   size_t path_len = strlen(path);
   int r = -1;

   if (d) {
      struct dirent *p;

      r = 0;
      while (!r && (p=readdir(d))) {
          int r2 = -1;
          char *buf;
          size_t len;

          /* Skip the names "." and ".." as we don't want to recurse on them. */
          if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
             continue;

          len = path_len + strlen(p->d_name) + 2;
          buf = (char *) malloc(len);

          if (buf) {
             struct stat statbuf;

             snprintf(buf, len, "%s/%s", path, p->d_name);
             if (!stat(buf, &statbuf)) {
                size_t pdir_size = 0;
                if (S_ISDIR(statbuf.st_mode)) {
                   r2 = DeleteDirectoryOrFile(buf, &pdir_size);
                   *deleted_size += pdir_size;
                } else {
                   *deleted_size += statbuf.st_size;
                   r2 = unlink(buf);
                }
             }
             free(buf);
          }
          r = r2;
      }
      closedir(d);
   } else if (errno == ENOTDIR) {
      std::string pathStr = path;
      *deleted_size = GetFileSize(pathStr);
      r = unlink(path);
      return r;
   }

   if (!r) {
      struct stat statbuf;
      if (!stat(path, &statbuf)) {
        *deleted_size += statbuf.st_size;
      }
      r = rmdir(path);
   }

   return r;
}

} // namespace aidl::vendor::xiaomi::hardware::misight::FileUtil

