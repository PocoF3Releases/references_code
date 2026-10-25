#include "MiSys.h"
#include <utils/Log.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include<fstream>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "misys.impl@2.0"

using std::string;
const std::string slash = "/";

#define FILE_MODE (0644)
#define FILE_MODE_V2 (0755)

#ifndef BLKGETSIZE64
#define BLKGETSIZE64 _IOR(0x12,114,size_t)
#endif

/* TODO: limit the file size to 512KB,
 * if bigger,
 * then HIDL may be crash.
 * we need to creat read new loop API.
 */
static const uint64_t READ_MAX = 524288;

namespace vendor {
namespace xiaomi {
namespace hardware {
namespace misys {
namespace V2_0 {
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

// Methods from ::vendor::xiaomi::hardware::misys::V2_0::IMiSys follow.
Return<IResultValue> MiSys::MiSysCreateFolder(const hidl_string& path, const hidl_string& folder_name) {
    IResultValue ret = IResultValue::MISYS_EINVAL;
    string pathFinal;
    int value = -1;

    //check if string path is NULL, if NULL, return MISYS_NOENT
    if (path.empty() || folder_name.empty()) {
      ALOGE("path or folder_name is NULL, return error");
      ret = IResultValue::MISYS_EINVAL;
      return ret;
    }

    pathFinal.append(path);

    value = handlePath(folder_name.c_str(), pathFinal);
    if (value) {
      ALOGE("handPath function return error(%d)\n", value);
      ret = IResultValue::MISYS_EINVAL;
      return ret;
    }

    if (access(pathFinal.c_str(), F_OK) != 0) {
      value = mkdir(pathFinal.c_str(), FILE_MODE_V2);
      if (value == 0) {
        ret = IResultValue::MISYS_SUCCESS;
      } else {
        ret = IResultValue::MISYS_EPERM;
      }
    } else {
      ret = IResultValue::MISYS_EEXIST;
    }

    return ret;
}

Return<IResultValue> MiSys::MiSysWriteBuffer(const hidl_string& path,
    const hidl_string& file_name, const hidl_vec<uint8_t>& writebuf, uint64_t buf_len) {
    std::fstream fs;
    char * buf = NULL;
    IResultValue ret = IResultValue::MISYS_EINVAL;
    string pathFinal;
    int result = -1;

    //check if string path is NULL, if NULL, return MISYS_NOENT
    if (path.empty() || file_name.empty()|| buf_len == 0|| buf_len != writebuf.size()) {
      ALOGE("parameter is invalid, return error");
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

    fs.open(pathFinal.c_str(), std::ios::out | std::ios::binary);

    //TODO: need to check the writebuf length: is not equal_to writebuf.size()
    if (fs.is_open()) {
      buf = new char[buf_len];
      if (!buf) {
        ALOGE("request buffer failed\n");
        ret = IResultValue::MISYS_EINVAL;
        fs.close();
        return ret;
      }

      for (uint64_t i = 0; i < buf_len; i++) {
        buf[i] = writebuf[i];
      }

      fs.write(buf, buf_len);

      if (fs.bad() || fs.fail()) {
        ret = IResultValue::MISYS_EINVAL;
        ALOGE("write %s failed\n", pathFinal.c_str());
        fs.close();
        return ret;
      }

      ret = IResultValue::MISYS_SUCCESS;
      fs.close();
      delete[] buf;
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

Return<void> MiSys::MiSysReadBuffer(const hidl_string& path, const hidl_string& file_name, MiSysReadBuffer_cb _hidl_cb) {
    IBufferReadResult readResult;
    std::fstream fs;
    string pathFinal;
    uint64_t file_size = 0;
    char *buffer = NULL;
    int result = 0;

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

    fs.open(pathFinal.c_str(),  std::ios::in | std::ios::binary);

    if (fs.is_open()) {
      fs.seekg(0, std::fstream::end);
      file_size = fs.tellg();

      fs.seekg(0, std::fstream::beg);

      //TODO: check the return value
      if (fs.bad() || fs.fail() || fs.eof()) {
        ALOGE("request buffer failed\n");
        readResult.value = IResultValue::MISYS_EINVAL;
        _hidl_cb(readResult);
        fs.close();
        return Void();
      }

      //TODO: HIDL will report error when file_size large, make this limitation and fix it later
      if (file_size >= READ_MAX) {
        ALOGE("file_size >= READ_MAX, return failed\n");
        readResult.value = IResultValue::MISYS_EINVAL;
        _hidl_cb(readResult);
        fs.close();
        return Void();
      }

      if (fs) {
        buffer = new char [file_size];

        if (buffer == NULL) {
          ALOGE("request buffer failed\n");
          readResult.value = IResultValue::MISYS_EINVAL;
          _hidl_cb(readResult);
          fs.close();
          return Void();
        }

        fs.read(buffer, file_size);

        readResult.data.resize(file_size);

        for (uint64_t i = 0; i< file_size; i++) {
          readResult.data[i] = buffer[i];
        }

        readResult.value = IResultValue::MISYS_SUCCESS;
         _hidl_cb(readResult);
        fs.close();
        delete[] buffer;
        return Void();
      } else {
        readResult.value = IResultValue::MISYS_EEXIST;
        ALOGE("read %s file failed\n", pathFinal.c_str());
      }
    } else {
      readResult.value = IResultValue::MISYS_EEXIST;
      ALOGE("open %s file failed\n", pathFinal.c_str());
    }

    _hidl_cb(readResult);
    fs.close();
    return Void();
}

Return<void> MiSys::GetPartitionSize(const hidl_string& path,  const hidl_string& partitionName,  GetPartitionSize_cb _hidl_cb) {
    string pathFinal;
    int fd;
    int result = 0;
    uint64_t partitionSize = 0;
    IPartitionSizeResult sizeResult;

    //initialize the initial value of sizeResult
    sizeResult.partitionSize = 0;
    sizeResult.result = IResultValue::MISYS_EINVAL;

    if (path.empty() || partitionName.empty()) {
      ALOGE("parameter is invalid, return MISYS_EINVAL");
      _hidl_cb(sizeResult);
      return Void();
    }

    pathFinal.append(path);
    result = handlePath(partitionName, pathFinal);

    if (result) {
      ALOGE("handlePath return error, result=%d\n", result);
      _hidl_cb(sizeResult);
      return Void();
    }

    if ((fd = open(pathFinal.c_str(), O_RDONLY)) < 0) {
      ALOGE("open failed, path=%s\n", pathFinal.c_str());
      _hidl_cb(sizeResult);
      return Void();
    }

    if (ioctl(fd, BLKGETSIZE64, &partitionSize) < 0) {
      close(fd);
      ALOGE("ioctl failed");
      _hidl_cb(sizeResult);
      return Void();
    }

    sizeResult.partitionSize = partitionSize;
    sizeResult.result = IResultValue::MISYS_SUCCESS;

    _hidl_cb(sizeResult);
    close(fd);

    return Void();
}

Return<bool> MiSys::IsExists(const hidl_string& path, const hidl_string& file_name) {
    bool result = false;
    string pathFinal;
    int value = -1;

    //check if string path is NULL, if NULL, return MISYS_NOENT
    if (path.empty()) {
      ALOGE("path is NULL, return error");
      return false;
    }

    // check folder if file_name is NULL
    if (file_name.empty()) {
      DIR *dir;
      if  ((dir = opendir(path.c_str())) == NULL) {
        result = false;
      } else {
        result = true;
      }

     closedir(dir);
     return result;
    }

    //begin to handle slash...
    pathFinal.append(path);

    value = handlePath(file_name.c_str(), pathFinal);
    if (value) {
      ALOGE("handPath function return error(%d)\n", value);
      result = false;
      return result;
    }

    //begin to try if pathFinal exists
    if (!access(pathFinal.c_str(), F_OK)) {
      result = true;
    } else {
      result = false;
    }

    return result;
}

// Methods from ::android::hidl::base::V1_0::IBase follow.
IMiSys* HIDL_FETCH_IMiSys(const char* /* name */) {
    return new MiSys();
}
//
}  // namespace implementation
}  // namespace V2_0
}  // namespace misys
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor
