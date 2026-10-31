#include "MiSys.h"
#include <utils/Log.h>
#include<iostream>
#include<stdlib.h>
#include<fstream>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "misys.impl"

static const int MAX_LINE = 255;
static const int MaxFileListSize = 512;

#define FILE_MODE (0644)

using std::string;

const std::string slash = "/";

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace misys {
namespace V1_0 {
namespace implementation {

// internal function to handle path and file_name
int handlePath(const hidl_string& file_name, string& pathFinal) {
    string pathTemp;
    int length = 0;

    pathTemp.append(file_name);

    string subStr = pathTemp.substr(0, 1);
    // return error if filename begin with slash
    if (!strncmp(subStr.c_str(), slash.c_str(), 1)) {
       ALOGE("file_name[0] is '/', return error");
       return -1;
     }

    length = pathFinal.length();
    subStr = pathFinal.substr(length-1, 1);

    if (!strncmp(subStr.c_str(), slash.c_str(), 1)) {
      pathFinal.append(file_name);
    } else {
      pathFinal.append(slash);
      pathFinal.append(file_name);
    }

    return 0;
}

static bool traverseFolder(const string path,  hidl_vec<IFileInfo> &list_info, int& file_count) {
    DIR *dir;
    struct dirent *de;
    struct stat st;

    dir = opendir(path.c_str());
    if (dir == 0) {
       closedir(dir);
       ALOGE("open path failed, return error");
       return false;
    }

    while((de = readdir(dir))) {
      /* 8 means file, 4 means folder */
      if (de->d_type == 8)  {
        /**
         * we do NOT support size larger than 1024
         * normally we do not have such requirements
         * just return FAILED
         */
        if (file_count >= 2 * MaxFileListSize) {
           closedir(dir);
           ALOGE("Folder includes more than 1024 files, return FAILED\n");
           return false;
        } else if (file_count == MaxFileListSize) {
            list_info.resize(2 * MaxFileListSize);
        }

        list_info[file_count].name = de->d_name;

        ALOGD("before lstat: %s\n", (path + "/" + de->d_name).c_str());
        if (lstat((path + "/" + de->d_name).c_str(), &st) == 0) {
          list_info[file_count].mtime = st.st_mtime;
          list_info[file_count].fileSize = st.st_size;
        } else {
          ALOGE("lstat return NOT zero path = %s\n", (path + "/" + de->d_name).c_str());
          list_info[file_count].mtime = 0;
          list_info[file_count].fileSize = 0;
        }

        file_count ++;
      } else if( de->d_type == 4)  {
        if( !strncmp(de->d_name, ".", 1) || !strncmp(de->d_name, "..", 2))
          continue;
        ALOGD("current path = %s\n", (path + "/" + de->d_name).c_str());
        traverseFolder(path + "/" + de->d_name  , list_info, file_count);
      }
    }

    closedir(dir);

    return true;
}

// Methods from ::vendor::xiaomi::hardware::misys::V1_0::IMiSys follow.
Return<void> MiSys::DirListFiles(const hidl_string& path, DirListFiles_cb _hidl_cb) {
    IFileListResult result;
    DIR *dir;
    int  file_count = 0;
    struct stat statbuf;
    int ret = 0;

    //check if string path is NULL, if NULL, return MISYS_NOENT
    if (path.empty()) {
      ALOGE("path is NULL, return error");
      result.value  = IResultValue::MISYS_NOENT;
      _hidl_cb(result);
      return Void();
    }

    // just return path if it is file
    ret = lstat(path.c_str(), &statbuf);
    if (ret != 0) {
      ALOGE("lstat for(%s) failed, errno = %d\n", path.c_str(), errno);
      result.value = IResultValue::MISYS_EINVAL;
      _hidl_cb(result);
      return Void();
    }
    if(!(statbuf.st_mode & S_IFDIR)) {
      ALOGD("path is point to one file. return file name");
      result.value = IResultValue::MISYS_SUCCESS;
      result.fileList.resize(1);
      result.fileList[0].name = path;
      _hidl_cb(result);
      return Void();
    }

    // traverse the directory for the all result
    dir = opendir(path.c_str());
    if (dir == 0) {
      closedir(dir);
      ALOGE("open path failed, return error, errno = %d\n", errno);
      result.value  = IResultValue::MISYS_EINVAL;
      _hidl_cb(result);
      return Void();
   }

    result.fileList.resize(MaxFileListSize);
    file_count = 0;

    traverseFolder(path, result.fileList, file_count);

    ALOGD("Finally file_count = %d\n", file_count);

    result.fileList.resize(file_count);

    result.value = IResultValue::MISYS_SUCCESS;
    _hidl_cb(result);
    closedir(dir);
    return Void();
}

Return<IResultValue> MiSys::MiSysWriteFile(const hidl_string& path, const hidl_string& file_name,
                                        const hidl_string& writebuf,
                                        uint32_t sbuf_len, uint8_t append_data) {
    std::fstream fs;
    IResultValue ret = IResultValue::MISYS_EINVAL;
    string pathFinal;
    int result = 0;

    //check if string path is NULL, if NULL, return MISYS_NOENT
    if (path.empty() || file_name.empty()|| sbuf_len == 0|| sbuf_len != writebuf.size()) {
      ALOGE("path is NULL, return error");
      ret = IResultValue::MISYS_NOENT;
      return ret;
    }

    pathFinal.append(path);

    result = handlePath(file_name, pathFinal);

    if (result) {
      ALOGE("handPath function return error(%d)\n", result);
      ret = IResultValue::MISYS_EINVAL;
      return ret;
    }

    if (!append_data) {
      fs.open(pathFinal.c_str(), std::ios::out);
    } else {
      fs.open(pathFinal.c_str(), std::ios::app);
    }

    //TODO: need to check the writebuf length: is not equal_to writebuf.size()
    if (fs.is_open()) {
      fs.write(writebuf.c_str(), sbuf_len);
      ALOGD("write file: %s, length: %d, append_data = %d\n", pathFinal.c_str(), sbuf_len, append_data);
      if (fs.bad() || fs.fail()) {
        ret = IResultValue::MISYS_EINVAL;
        ALOGE("write %s failed\n", pathFinal.c_str());
        fs.close();
        return ret;
      }

      ret = IResultValue::MISYS_SUCCESS;
      fs.close();
      // set correct file mode for new file
      if (chmod(pathFinal.c_str(), FILE_MODE) != 0) {
        ret = IResultValue::MISYS_EINVAL;
        ALOGE("%s chmod failed\n", pathFinal.c_str());
        return ret;
      }
    } else {
      //TODO: add one more result type: open failed
      ret = IResultValue::MISYS_EINVAL;
      ALOGE("open %s failed\n", pathFinal.c_str());
    }

    return ret;
}

Return<void> MiSys::MiSysReadFile(const hidl_string& path, const hidl_string& file_name, MiSysReadFile_cb _hidl_cb) {
    std::fstream fs;
    string pathFinal;
    IReadResult readResult;
    uint32_t file_size = 0;
    char *buffer = NULL;
    int result = 0;
    int flag = 0;

    //check if string path is NULL, if NULL, return MISYS_NOENT
    if (path.empty() || file_name.empty()) {
      ALOGE("path or file_name is NULL, return error");
      readResult.value  = IResultValue::MISYS_NOENT;
      _hidl_cb(readResult);
      return Void();
    }

    pathFinal.append(path);

    result = handlePath(file_name, pathFinal);

    if (result) {
      ALOGE("handPath function return error(%d)\n", result);
      readResult.value = IResultValue::MISYS_EINVAL;
      _hidl_cb(readResult);
      return Void();
    }

    ALOGI("name now = %s\n", pathFinal.c_str());

    fs.open(pathFinal.c_str(),  std::ios::in);

    if (fs.is_open()) {
      fs.seekg(0, std::fstream::end);
      file_size = fs.tellg();
      ALOGI("filezie = %u\n", file_size);

      fs.seekg(0, std::fstream::beg);

      //TODO: check the return value
      if (fs.bad() || fs.fail() || fs.eof()) {
        ALOGE("request buffer failed\n");
        readResult.value = IResultValue::MISYS_EINVAL;
        _hidl_cb(readResult);
        fs.close();
        return Void();
      }

      if (file_size == 0) {
          file_size = MAX_LINE;
          flag = 1;
      }

      buffer = new char [file_size + 1];

      if (buffer == NULL) {
        ALOGE("request buffer failed\n");
        readResult.value = IResultValue::MISYS_EINVAL;
        _hidl_cb(readResult);
        fs.close();
        return Void();
      }

      if (fs) {
        fs.read(buffer, file_size);
        if (flag)
            file_size = fs.gcount();
        buffer[file_size] = '\0';
        ALOGD("%s", buffer);
        readResult.data = buffer;
        ALOGI("readResult.data size = %zu\n", readResult.data.size());
        readResult.value = IResultValue::MISYS_SUCCESS;
         _hidl_cb(readResult);
        fs.close();
        delete[] buffer;
        return Void();
      } else {
        readResult.value = IResultValue::MISYS_EEXIST;
        ALOGD("read %s file failed\n", pathFinal.c_str());
        delete[] buffer;
      }
    } else {
      readResult.value = IResultValue::MISYS_EEXIST;
      ALOGE("open %s file failed\n", pathFinal.c_str());
     }

    _hidl_cb(readResult);
    fs.close();
    return Void();
}

int traverseFolderForDelete(const char* destDir)
{
    int ret = 0;
    DIR* dp = NULL;
    struct dirent* entry = NULL;
    struct stat statbuf;

    dp = opendir(destDir);
    if (dp == NULL)
    {
      ALOGE("cannot open directory %s, errno = %d\n", destDir, errno);
      return errno;
    }

    while ((entry = readdir(dp)) != NULL)
    {
       if( !strncmp(entry->d_name, ".", 1) || !strncmp(entry->d_name, "..", 2)) {
         continue;
       }
       string tempDir = string(destDir) + "/" + string(entry->d_name);
       ALOGD("destDir = %s, tempDir = %s\n", destDir, tempDir.c_str());
       lstat(tempDir.c_str(), &statbuf);
       if(!(statbuf.st_mode & S_IFDIR))
       {
          string tempFile = string(destDir) + "/" + string(entry->d_name);
          ALOGD("destDir = %s, tempDir = %s, tempFile = %s\n", destDir, tempDir.c_str(), tempFile.c_str());
          ret = remove(tempFile.c_str());
          if(ret)
          {
             ALOGE("remove file fail, errno =  %d\n", errno);
             closedir(dp);
             dp = NULL;
             return ret;
          }
      } else {
        ALOGD("traversal dir  %s\n", tempDir.c_str());
        traverseFolderForDelete(tempDir.c_str());
      }
    }

    closedir(dp);
    dp = NULL;

    ret = remove(destDir);
    if(ret)
    {
      ALOGE("remove dir fail, destDir = %s\n", destDir);
    }

    return ret;
}

Return<IResultValue> MiSys::EraseFileOrDirectory(const hidl_string& path, const hidl_string& file_name) {
    IResultValue ret = IResultValue::MISYS_EINVAL;
    string pathFinal;
    int result = 0;
    struct stat statbuf;

    //check if string path is NULL, if NULL, return MISYS_NOENT
    if (path.empty() ) {
      ALOGE("path or file_name is NULL, return error");
      ret  = IResultValue::MISYS_NOENT;
      return ret;
    }

    pathFinal.append(path);

    ALOGD("pathFinal(%s) begin\n", pathFinal.c_str());

    /**
     * code must support below path format like /mnt/vendor/persist/
     * and /mnt/vendor/persist for upper layer use.
     */
    if (!file_name.empty()) {
      result = handlePath(file_name, pathFinal);

      if (result) {
        ALOGE("handPath function return error(%d)\n", result);
        ret = IResultValue::MISYS_EINVAL;
        return ret;
      }
    }

   ALOGD("pathFinal(%s) end\n", pathFinal.c_str());

    // begin to delete the file if pathFinal is file
    result = lstat(pathFinal.c_str(), &statbuf);
    if (result != 0) {
      ALOGE("lstat for(%s) failed, errno = %d\n", pathFinal.c_str(), errno);
      ret = IResultValue::MISYS_EINVAL;
      return ret;
    }

    if(!(statbuf.st_mode & S_IFDIR)) {
      ALOGI("file(%s) removed\n", pathFinal.c_str());
      result = remove(pathFinal.c_str());
      if (result) {
        ALOGE("remove file(%s) failed, error = %d\n", pathFinal.c_str(), errno);
        ret = IResultValue::MISYS_NOENT;
      } else {
        ret = IResultValue::MISYS_SUCCESS;
      }

      return ret;
    }

    // begin to delete the folder
    result = traverseFolderForDelete(pathFinal.c_str());
    if (result == 0) {
      ret  = IResultValue::MISYS_SUCCESS;
    } else {
     ret = IResultValue::MISYS_EINVAL;
    }

    return ret;
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

IMiSys* HIDL_FETCH_IMiSys(const char* /* name */) {
    return new MiSys();
}
//
}  // namespace implementation
}  // namespace V1_0
}  // namespace misys
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor
