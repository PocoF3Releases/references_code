/*
 * Copyright (c) 2019 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

/**
 * Introduction:
 * This 3.0 HIDL aimed to fix belows issues under Treble arch.
 * 1. Large files read/write using share memory.
 *    NO file size limitation by technology. But we limit the MAX size <= 128MB by design.
 *    share memory based on hidl_memory/IAllocator/MemoryBlock designed by google.
 *
 *    BTW: Large files read/write via share memory has JNI interface which can called by Java
 *    layer. See more for libmisys_jni.xiaomi.so.
 *
 * 2. Large memory read/write using share memory.
 * 3. Callback function.
 * 4. Set property function through HIDL. Selinux rules now forbid setprop under Treble.
 *    get propperty do not needed via HIDL.
 */

#include "MiSys.h"

#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <hidlmemory/mapping.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "misys.impl@3.0"

#define FILE_MODE (0644)

// MAX size = 128MB
static const uint64_t MaxSize = 134217728;

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::hidl_memory;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hidl::allocator::V1_0::IAllocator;

using ::vendor::xiaomi::hardware::misys::V3_0::IMiSysCallBack;

using std::string;
const std::string slash = "/";


namespace vendor {
namespace xiaomi {
namespace hardware {
namespace misys {
namespace V3_0 {
namespace implementation {

// internal function to handle path and file_name
static int handlePath(const hidl_string& file_name, string& pathFinal) {
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
// Methods from ::vendor::xiaomi::hardware::misys::V3_0::IMiSys follow.
Return<void> MiSys::AllocateBlock(uint64_t size, AllocateBlock_cb _hidl_cb) {
    IBlockResult blockResult;
    bool retval = false;
    hidl_memory mem;

    //need to initialize the blockResult
    blockResult.result = IResultValue::MISYS_NOENT;

    //check if size is too large, max size is 128MB
    if (size > MaxSize) {
      ALOGE("%s request size too large, max size=128MB\n", __func__);
      blockResult.result = IResultValue::MISYS_EINVAL;
       _hidl_cb(blockResult);
      return Void();
    }

    //get the ashmem HIDL service
    sp<IAllocator> ashmem = IAllocator::getService("ashmem");

    //TODO
    /* ASSERT_NE(nullptr, ashmemAllocator.get() */
    /* ASSERT_TRUE(ashmemAllocator->isRemote()) */  //allocator is always remote

    if (ashmem == 0) {
      blockResult.result = IResultValue::MISYS_NOENT;
      ALOGE("Failed to get ashmem allocator service, return MISYS_NOENT");
      _hidl_cb(blockResult);
      return Void();
    }

    Return<void> result = ashmem->allocate(
      size,
      [&](bool success, const hidl_memory& memory) {
        if (success) {
          mem = memory;
          retval = true;
          ALOGD("%s get hidl_memory successful\n", __func__);
        }
      });

    if (result.isOk() && retval == true) {


      ALOGD("%s get MemoryBlock successful, return MISYS_SUCCESS\n", __func__);
      blockResult.result = IResultValue::MISYS_SUCCESS;
      blockResult.block = mem;
    } else {
      ALOGI("%s get hidl_memory failed, return MISYS_NOENT\n", __func__);
      blockResult.result = IResultValue::MISYS_NOENT;
    }

    _hidl_cb(blockResult);
    return Void();
}

Return<IResultValue> MiSys::ReleaseBlock(const ::android::hardware::hidl_memory& block) {
    IResultValue result = IResultValue::MISYS_SUCCESS;

    if (block.handle() == nullptr) {
      result = IResultValue::MISYS_NOENT;
      ALOGE("%s Null to release, return MISYS_NOENT\n", __func__);
      return result;
    }

    return result;
}

Return<IResultValue> MiSys::WriteToFile(const ::android::hardware::hidl_memory& block,
    const hidl_string& path, const hidl_string& fileName, uint64_t size) {
    string pathFinal;
    std::fstream fs;
    IResultValue result = IResultValue::MISYS_NOENT;
    char *buffer = NULL;
    int ret = -1;
    uint64_t mSize = 0;
    sp<IMemory> m = nullptr;

    ALOGV("path=%s, fileName=%s\n", path.c_str(), fileName.c_str());

    if (path.empty() || fileName.empty() || (size == 0)) {
      ALOGE("parameter is invalid, return MISYS_NOENT");
      return result;
    }

    //check if size is too large, max size is 128MB
    if (size > MaxSize) {
      ALOGE("%s request size too large, max size=128MB\n", __func__);
      result = IResultValue::MISYS_EINVAL;
      return result;
    }

    pathFinal.append(path);

    ret = handlePath(fileName, pathFinal);

    if (ret) {
      result = IResultValue::MISYS_NOENT;
      ALOGE("handlePath return error, ret=%d\n", ret);
      return result;
    }

    m = android::hardware::mapMemory(block);

   if (m == nullptr) {
      result = IResultValue::MISYS_NOENT;
      ALOGE("mapMamory return error, return MISYS_NOENT\n");
      return result;
   }

    uint8_t * data = static_cast<uint8_t*>(static_cast<void*>(m->getPointer()));

    if (data == NULL) {
      result = IResultValue::MISYS_EINVAL;
      ALOGE("data is null, return MISYS_EINVAL\n");
      return result;
    }

    mSize = m->getSize();

    if (mSize == 0) {
      result = IResultValue::MISYS_NOENT;
      ALOGE("mSize is 0, return MISYS_NOENT\n");
      return result;
    }

    if (size > mSize) {
      ALOGE("fieSize too large, return MISYS_EINVAL\n");
      result = IResultValue::MISYS_EINVAL;
      return result;
    }

    // begin to read file of this location
    fs.open(pathFinal.c_str(), std::ios::out | std::ios::binary);

    if (fs.bad() || fs.fail() || fs.eof()) {
      ALOGE("fs open failed, path=%s\n", pathFinal.c_str());
      fs.close();
      return result;
    }

    if (fs) {
      // do chmod first in case failed, then we can just return
      ret = chmod(pathFinal.c_str(), FILE_MODE);

      if (ret < 0) {
        ALOGE("chmod(%s) failed, err=%s\n", pathFinal.c_str(), strerror(errno));
        fs.close();
        result = IResultValue::MISYS_EINVAL;
        return result;
      }

      buffer = new char[size];

      if (buffer == NULL) {
        ALOGE("allocate buffer failed, return MISYS_ENOMEM\n");
        result = IResultValue::MISYS_ENOMEM;
        fs.close();
        return result;
      }

      m->update();

      memcpy(buffer, data, size);

      m->commit();

     fs.write(buffer, size);

     if (fs.bad() || fs.fail()) {
       result = IResultValue::MISYS_EINVAL;
       ALOGE("%s write failed, return MISYS_EINVAL\n", __func__);
       fs.close();
       delete[] buffer;
       return result;
     }

      result = IResultValue::MISYS_SUCCESS;
      fs.close();
      delete[] buffer;
    } else {
      result = IResultValue::MISYS_EINVAL;
      ALOGE("open file %s failed\n", pathFinal.c_str());
    }

    return result;
}

Return<void> MiSys::GetFileSize(const hidl_string& path, const hidl_string& fileName, GetFileSize_cb _hidl_cb) {
    string pathFinal;
    int result = 0;
    std::fstream fs;
    uint64_t fileSize = 0;
    IFileSizeResult sizeResult;

    //initialize the initial value of sizeResult
    sizeResult.fileSize = 0;
    sizeResult.result = IResultValue::MISYS_EINVAL;

    if (path.empty() || fileName.empty()) {
      ALOGE("parameter is invalid, return MISYS_EINVAL");
      _hidl_cb(sizeResult);
      return Void();
    }

    pathFinal.append(path);

    result = handlePath(fileName, pathFinal);

    if (result) {
      ALOGE("handlePath return error, result=%d\n", result);
      _hidl_cb(sizeResult);
      return Void();
    }

    // begin to read file of this location
    fs.open(pathFinal.c_str(), std::ios::in | std::ios::binary);

    if (fs.is_open()) {
      fs.seekg(0, std::fstream::end);
      fileSize = fs.tellg();
      fs.seekg(0, std::fstream::beg);
    }

    if (fs.bad() || fs.fail() || fs.eof()) {
      ALOGE("fs open failed, path=%s\n", pathFinal.c_str());
      fs.close();
      _hidl_cb(sizeResult);
      return Void();
    }

    sizeResult.fileSize = fileSize;
    sizeResult.result = IResultValue::MISYS_SUCCESS;

    _hidl_cb(sizeResult);
    fs.close();

    return Void();
}

Return<void> MiSys::ReadFromFile(const ::android::hardware::hidl_memory& block,
    const hidl_string& path, const hidl_string& fileName,
    ReadFromFile_cb _hidl_cb) {
    IBlockResult readResult;
    string pathFinal;
    int result = 0;
    std::fstream fs;
    uint64_t fileSize = 0;
    char *buffer = NULL;
    uint64_t size = 0;
    sp<IMemory> m = nullptr;

    ALOGV("path=%s, fileName=%s\n", path.c_str(), fileName.c_str());

    //TODO: we need to initialize the IBlockResult.block

    if (path.empty() || fileName.empty()) {
      readResult.result = IResultValue::MISYS_EINVAL;
      ALOGE("parameter is invalid, return MISYS_EINVAL");
      _hidl_cb(readResult);
      return Void();
    }

    pathFinal.append(path);

    result = handlePath(fileName, pathFinal);

    if (result) {
      readResult.result = IResultValue::MISYS_EINVAL;
      ALOGE("handlePath return error, result=%d\n", result);
      _hidl_cb(readResult);
      return Void();
    }

    // return failed if mapMemory() fail
    m = android::hardware::mapMemory(block);
    if (m == nullptr) {
      readResult.result = IResultValue::MISYS_EINVAL;
      ALOGE("mapMemory(block) failed, return\n");
      _hidl_cb(readResult);
      return Void();
   }

    uint8_t * data = static_cast<uint8_t*>(static_cast<void*>(m->getPointer()));

    if (data == NULL) {
      readResult.result = IResultValue::MISYS_EINVAL;
      ALOGE("data is null, return MISYS_EINVAL\n");
      _hidl_cb(readResult);
      return Void();
    }

    size = m->getSize();

    if (size == 0) {
      readResult.result = IResultValue::MISYS_EINVAL;
      ALOGE("size of memory block is 0, abort, return MISYS_EINVAL\n");
      _hidl_cb(readResult);
      return Void();
    }

    //check if size is too large, max size is 128MB
    if (size > MaxSize) {
      ALOGE("%s request size too large, max size=128MB\n", __func__);
      readResult.result = IResultValue::MISYS_EINVAL;
      _hidl_cb(readResult);
      return Void();
    }

    // begin to read file of this location
    fs.open(pathFinal.c_str(), std::ios::in | std::ios::binary);

    if (fs.is_open()) {
      fs.seekg(0, std::fstream::end);
      fileSize = fs.tellg();
      fs.seekg(0, std::fstream::beg);
    }

    if (fs.bad() || fs.fail() || fs.eof()) {
      readResult.result = IResultValue::MISYS_EINVAL;
      ALOGE("fs open failed, path=%s\n", pathFinal.c_str());
      _hidl_cb(readResult);
      fs.close();
      return Void();
    }

    if (fileSize > size || fileSize == 0) {
      readResult.result = IResultValue::MISYS_EINVAL;
      ALOGE("fieSize invalid, ERROR, return MISYS_EINVAL\n");
      _hidl_cb(readResult);
      return Void();
    }

    if (fs) {
      buffer = new char[fileSize];

      if (buffer == NULL) {
        readResult.result = IResultValue::MISYS_ENOMEM;
        ALOGE("allocate buffer failed, return MISYS_ENOMEM\n");
        _hidl_cb(readResult);
        fs.close();
        return Void();
      }

      fs.read(buffer, fileSize);

      m->update();
      memcpy(data, buffer, fileSize);
      m->commit();
    }

    readResult.result = IResultValue::MISYS_SUCCESS;
    readResult.block = block;
    _hidl_cb(readResult);
    delete[] buffer;
    return Void();
}

Return<IResultValue> MiSys::MiSysSetProp(const hidl_string& key, const hidl_string& value) {
    IResultValue result = IResultValue::MISYS_NOENT;
    int ret = -1;

    if (key.empty() || value.empty()) {
      result = IResultValue::MISYS_EINVAL;
      ALOGE("key or value is empty, return MISYS_EINVAL\n");
      return result;
    }

    ret = property_set(key.c_str(), value.c_str());
    if (ret == -1) {
      result = IResultValue::MISYS_EINVAL;
      ALOGE("set property(%s) to %s failed, error=%d(%s)\n",key.c_str(), value.c_str(), errno, strerror(errno));
      return result;
    } else {
      result = IResultValue::MISYS_SUCCESS;
    }

    return result;
}

Return<IResultValue> MiSys::OnDataReady(const ::android::hardware::hidl_memory& block, uint64_t memorySize, uint32_t moduleID) {
    IResultValue result = IResultValue::MISYS_NOENT;
    sp <IMiSysCallBack> cb = NULL;

    std::lock_guard<std::mutex> _lock(cbLock);

    for (auto it = cbList.begin(); it != cbList.end();) {
      if (((*it).moduleID) == moduleID) {
        //call callback function which equals to moduleID.
        cb = (*it).callback;
        cb->onDataCb(block, memorySize, moduleID);
        result = IResultValue::MISYS_SUCCESS;
        break;
      } else {
        ++it;
      }
    }

    return result;
}

Return<IResultValue> MiSys::SetCallBack(const sp<::vendor::xiaomi::hardware::misys::V3_0::IMiSysCallBack>& callback, uint32_t moduleID, int32_t pid) {
    IResultValue result = IResultValue::MISYS_NOENT;
    struct CallBackEvent cbEvent;

    if (callback == nullptr) {
      ALOGE("%s callback is NULL,return error\n", __func__);
      return result;
    }

    if ((moduleID == 0) || (pid < 0)) {
      ALOGE("%s moduleID(%d) OR pid(%d) is invalid\n", __func__, moduleID, pid);
      return result;
    }

    ALOGV("%s moduleID = %d, pid=%d\n", __func__, moduleID, pid);

    cbEvent.moduleID = moduleID;
    cbEvent.callback = callback;
    cbEvent.pid = pid;

    uint32_t findPid = 0;
    // TODO: check if callback already queued
    // we only check the moduleID, that means one moduleID can only have one callback.
    for (auto it = cbList.begin(); it != cbList.end();) {
      if (((*it).moduleID) == moduleID) {
        findPid = (*it).pid;
        result = IResultValue::MISYS_EEXIST;
        break;
      } else {
        ++it;
      }
    }

    if (result == IResultValue::MISYS_EEXIST) {
      ALOGE("%s FAILED: moduleID(%d) with pid(%d) exists\n", __func__, moduleID, findPid);
      return result;
    }

    // push into the list
    std::lock_guard<std::mutex> _lock(cbLock);
    cbList.push_back(cbEvent);

    result = IResultValue::MISYS_SUCCESS;

    ALOGE("%s callback queue successful for pid(%d)\n", __func__, pid);

    return result;
}

Return<void> MiSys::GetCallBackPid(uint32_t moduleID, GetCallBackPid_cb _hidl_cb) {
    IGetPidResult pidResult;

    // initialize the pidResult value
    pidResult.result = IResultValue::MISYS_NOENT;
    pidResult.pid = -1;

    //TODO: check if the cbList is empty

    for (auto it = cbList.begin(); it != cbList.end();) {
      if (((*it).moduleID) == moduleID) {
        pidResult.pid = (*it).pid;
        pidResult.result = IResultValue::MISYS_SUCCESS;
        break;
      } else {
        ++it;
      }
    }

    _hidl_cb(pidResult);
    return Void();
}

Return<IResultValue> MiSys::ReleaseCallBack(const sp<::vendor::xiaomi::hardware::misys::V3_0::IMiSysCallBack>& callback, uint32_t moduleID, int32_t pid) {
    IResultValue result = IResultValue::MISYS_NOENT;

    if (callback == NULL) {
      ALOGE("%s callback is NULL,return error\n", __func__);
      return result;
    }

    ALOGV("%s moduleID = %d, pid=%d\n", __func__, moduleID, pid);

    std::lock_guard<std::mutex> _lock(cbLock);

    for (auto it = cbList.begin(); it != cbList.end();) {
      if (((*it).callback) == callback) {
        it = cbList.erase(it);
        result = IResultValue::MISYS_SUCCESS;
        break;
      } else {
        ++it;
      }
    }

    return result;
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

IMiSys* HIDL_FETCH_IMiSys(const char* /* name */) {
    return new MiSys();
}
//
}  // namespace implementation
}  // namespace V3_0
}  // namespace misys
}  // namespace hardware
}  // namespace xiaomi
}  // namespace vendor
