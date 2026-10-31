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

#include <utils/Log.h>
#include "MiSys.h"
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include<iostream>
#include <fstream>
#include <hidlmemory/mapping.h>

#include <include/CheckPid.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "cb-test"

using namespace android;
using std::string;
using namespace::std;

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hidl::memory::V1_0::IMemory;

using IMiSys_V1_0 = ::vendor::xiaomi::hardware::misys::V1_0::IMiSys;
using IMiSys_V3_0 = ::vendor::xiaomi::hardware::misys::V3_0::IMiSys;
using ::vendor::xiaomi::hardware::misys::V1_0::IResultValue;
using ::vendor::xiaomi::hardware::misys::V1_0::IFileListResult;
using ::vendor::xiaomi::hardware::misys::V1_0::IReadResult;
using ::vendor::xiaomi::hardware::misys::V3_0::IBlockResult;
using ::vendor::xiaomi::hardware::misys::V3_0::IGetPidResult;

static sp<IMiSys_V3_0> service_V30 = nullptr;

//The required heap size alignment is 4096 bytes
static const uint64_t kHeapSizeAlignment = (0x1ULL << 12);

static const string logPath = "/mnt/vendor/persist/misys/";
static const string logPrefix = "log_";

struct IBlockResult block;

static sp<IMiSys_V3_0> getMiSysService_V30() {
    if (service_V30 == nullptr) {
      service_V30 = IMiSys_V3_0::getService();
      if (service_V30 != nullptr) {
        ALOGI("Initialize the misys@3.0 successful");
      }
    }
    return service_V30;
}

/* test misys@3.0 */
static int testCb(const hidl_string& path, const hidl_string& name, uint32_t moduleID) {
    uint64_t fileSize = 0;
    uint64_t mSize = 0;
    IResultValue result = IResultValue::MISYS_EINVAL;
    IGetPidResult pidResult;
    string logFile;
    string pidFile;

    service_V30->GetFileSize(path, name,
        [&](auto&IFileSizeResult) {
          fileSize = IFileSizeResult.fileSize;
          result = IFileSizeResult.result;

          if (result != IResultValue::MISYS_SUCCESS) {
            ALOGE("%s GetFileSize() failed(), result = %d\n", __func__, result);
          }
        });

     if (result != IResultValue::MISYS_SUCCESS) {
        ALOGE("%s GetFileSize() failed(), result = %d\n", __func__, result);
        return -1;
     }

    //Allocate MemoryBlock for read
    if (fileSize % kHeapSizeAlignment != 0) {
      mSize = ((fileSize / kHeapSizeAlignment) + 1) * kHeapSizeAlignment;
    } else {
      mSize = fileSize;
    }

    service_V30->AllocateBlock(mSize,
        [&](auto& BlockResult) {
        block.result = BlockResult.result;
        block.block = BlockResult.block;

        ALOGE("%s result = %d\n", __func__, block.result);
    });

    if (block.result != IResultValue::MISYS_SUCCESS) {
      ALOGE("%s AllocateBlock() failed(), result = %d\n", __func__, result);
      return -1;
    }

    service_V30->ReadFromFile(block.block,
        path,
        name,
        [&](auto& IBlockResult) {
        block.result = IBlockResult.result;

        ALOGD("%s ReadFromFile() result = %d\n", __func__, block.result);
    });

    if (block.result != IResultValue::MISYS_SUCCESS) {
      ALOGE("%s ReadFromFile() failed(), result = %d\n", __func__, result);
      return -1;
    }

    sp<IMemory> m = android::hardware::mapMemory(block.block);

    uint8_t * data = static_cast<uint8_t*>(static_cast<void*>(m->getPointer()));

    if (data == NULL) {
      ALOGE("data is null\n");
      return -1;
    }

    uint64_t size = m->getSize();

    // check the callback process exists or not before call the callback function.
    service_V30->GetCallBackPid(moduleID,
        [&](auto& IGetPidResult) {
        pidResult.result = IGetPidResult.result;
        pidResult.pid = IGetPidResult.pid;

        ALOGD("%s GetCallBackPid() result = %d, pid = %d\n", __func__, pidResult.result, pidResult.pid);
    });

    string pidStr = std::to_string(pidResult.pid);
    pidFile.append(logPrefix);
    pidFile.append(pidStr);

    logFile = logPath;
    logFile.append(pidFile);


    // check if the pid related process still alive
    // check if the pid is valid, if invalid, then just return ERROR.
    int ret = -1;
    ret = checkPidExists(pidResult.pid);
    if (ret < 0) {
      ALOGE("%s pid(%d) check error(%d)\n", __func__, pidResult.pid, ret);
      return -1;
    }

    if (ret == 1) {
      ALOGI("FIND pid(%d)", pidResult.pid);
      service_V30->OnDataReady(block.block, size, 8);
    } else {
      ALOGE("cannot find pid(%d), ignore the callback function", pidResult.pid);
    }

   return 0;
}

int main(int /* argc */, char ** /* argv */) {

   if (getMiSysService_V30() == nullptr) {
      ALOGE("Unable to initialize the HIDL misys@3.0");
      return -1;
    }

    /* test 3.0 HIDL interface Callback function */
    testCb("/data/vendor/", "vmlinux", 8);

    return 0;
}
