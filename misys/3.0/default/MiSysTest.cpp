/*
 * Copyright (c) 2019 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

#include "MiSys.h"
#include<iostream>

#include <unistd.h>

#include <utils/Log.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidlmemory/mapping.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "misys.test-3"

using namespace::std;
using std::string;
using namespace android;

using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hidl::memory::V1_0::IMemory;

using ::vendor::xiaomi::hardware::misys::V1_0::IResultValue;
using ::vendor::xiaomi::hardware::misys::V3_0::IBlockResult;

using IMiSys_V3_0 = ::vendor::xiaomi::hardware::misys::V3_0::IMiSys;

static sp<IMiSys_V3_0> service_V30 = nullptr;

//The required heap size alignment is 4096 bytes
static const uint64_t kHeapSizeAlignment = (0x1ULL << 12);

static sp<IMiSys_V3_0> getMiSysService_V30() {
    if (service_V30 == nullptr) {
      service_V30 = IMiSys_V3_0::getService();
      if (service_V30 != nullptr) {
        ALOGI("Initialize the misys@3.0 successful");
      }
    }
    return service_V30;
}

struct IBlockResult block;

/* test misys@3.0 */
static int testFile(const hidl_string& path, const hidl_string& name, const hidl_string& writeName) {
    uint64_t fileSize = 0;
    uint64_t mSize = 0;
    IResultValue result, testResult = IResultValue::MISYS_EINVAL;

    ALOGI("%s, file path = %s, name = %s, writeName = %s\n", __func__, path.c_str(), name.c_str(), writeName.c_str());
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

        ALOGI("%s result = %d\n", __func__, block.result);
    });

    if (block.result != IResultValue::MISYS_SUCCESS) {
      ALOGI("%s AllocateBlock() failed(), result = %d\n", __func__, result);
      return -1;
    }

    service_V30->ReadFromFile(block.block,
        path,
        name,
        [&](auto& IBlockResult) {
        block.result = IBlockResult.result;

        ALOGI("%s ReadFromFile() result = %d\n", __func__, block.result);
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

    testResult = service_V30->WriteToFile(block.block,
        path,
        writeName,
        fileSize);

    ALOGI("%s WriteToFile() result = %d\n", __func__, testResult);

    if (block.result != IResultValue::MISYS_SUCCESS) {
      ALOGE("%s WriteToFile() failed(), result = %d\n", __func__, result);
      return -1;
    }

    // release MemoryBLock
    service_V30->ReleaseBlock(block.block);

    return 0;
}

/* test misys@3.0 setProp function */
static int testSetProp(const hidl_string& key, const hidl_string& value) {
    IResultValue testResult;

    testResult = service_V30->MiSysSetProp(key, value);
    ALOGI("%s SetProp() result = %d\n", __func__, testResult);

    return 0;
}

using ::vendor::xiaomi::hardware::misys::V3_0::IMiSysCallBack;

class TestMiSysCallBack : public IMiSysCallBack {
    public:
      virtual Return<void> onDataCb(const ::android::hardware::hidl_memory& block,
          uint64_t memorySize, uint32_t moduleID) override {
          IResultValue testResult = IResultValue::MISYS_EINVAL;
          struct IBlockResult mBlock;

          ALOGV("%s moduleID =%d\n", __func__, moduleID);

          // write the memory block to the file
          if (service_V30 == nullptr) {
            ALOGE("%s HIDL service is NULL\n", __func__);
            return Void();
          }

          sp<IMemory> m = android::hardware::mapMemory(block);

          uint8_t * data = static_cast<uint8_t*>(static_cast<void*>(m->getPointer()));

          if (data == NULL) {
            ALOGE("data is null\n");
            return Void();
          }

          uint64_t size = m->getSize();

          if(memorySize == size) {
            ALOGV("equal\n");
          }

          string path = "/data/vendor/";
          string writeName = "vmlinux-cb";
          mBlock.block = block;
          mBlock.result = IResultValue::MISYS_EINVAL;
          testResult = service_V30->WriteToFile(mBlock.block,
            path,
            writeName,
            size);

          ALOGE("%s WriteToFile() result = %d\n", __func__, testResult);

         if (testResult != IResultValue::MISYS_SUCCESS) {
           ALOGE("%s WriteToFile() failed(), result = %d\n", __func__, testResult);
           return Void();
         }

        // release MemoryBLock
        service_V30->ReleaseBlock(mBlock.block);

        return Void();
      }

      sp<IMiSys_V3_0> service_V30 = nullptr;
};

int main(int /* argc */, char ** /* argv */) {
    if (getMiSysService_V30() == nullptr) {
      ALOGE("Unable to initialize the HIDL misys@3.0");
      return -1;
    }

    /* test 3.0 HIDL interface */
    testFile("/data/vendor/", "dtbo.img", "dtbo-write.img");
    testFile("/data/vendor/", "vmlinux", "vmlinux-write");
    testSetProp("persist.sys.usb.config", "diag,diag_mdm,qdss,qdss_mdm,serial_cdev,dpl,rmnet,adb");

    // begin to call setCallBack().
    sp<TestMiSysCallBack> testCb = new TestMiSysCallBack();
    testCb->service_V30 = service_V30;
    service_V30->SetCallBack(testCb, 8, getpid());

    sleep(6000);

    return 0;
}
