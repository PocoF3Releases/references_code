/*
 *    Copyright (c) 2019 Xiaomi Technologies, Inc. All Rights Reserved.
 *    Xiaomi Technologies Proprietary and Confidential.
 */

// enable verbose logs
#define LOG_NDEBUG  0

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "misys@3.0-JNI"

#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <jni.h>
#include <utils/Log.h>
#include "cutils/properties.h"

#include <hidl/Status.h>
#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <hidl/HidlTransportSupport.h>
#include <hidlmemory/mapping.h>

#include <android/hidl/memory/1.0/IMemory.h>
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/block/1.0/types.h>

#include <vendor/xiaomi/hardware/misys/3.0/IMiSys.h>
#include <vendor_xiaomi_hardware_misys_V3_0_MiSys.h>

using namespace android;

using ::android::sp;
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hardware::hidl_memory;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::Return;

using ::vendor::xiaomi::hardware::misys::V3_0::IMiSysCallBack;
using ::vendor::xiaomi::hardware::misys::V1_0::IResultValue;
using ::vendor::xiaomi::hardware::misys::V3_0::IBlockResult;

using IMiSys_V3_0 = ::vendor::xiaomi::hardware::misys::V3_0::IMiSys;

//The required heap size alignment is 4096 bytes
static const uint64_t kHeapSizeAlignment = (0x1ULL << 12);

// ----------------------------------------------------------------------------
//    Native Methods
// ----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

static sp<IMiSys_V3_0> service_V30 = nullptr;
static bool init = false;

static sp<IMiSys_V3_0> getMiSysService_V30() {
    if (service_V30 == nullptr) {
      service_V30 = IMiSys_V3_0::getService();
      if (service_V30 != nullptr) {
        ALOGI("get misys@3.0 service successful");
      } else {
        ALOGI("get misys@3.0 service failed");
      }
    }

    return service_V30;
}

/*
 * Class:     vendor_xiaomi_hardware_V3_10_MiSys
 * Method:    init
 * Signature:
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Return - current value for this parameter returned as a int
 */

JNIEXPORT jint JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_init
  (__attribute__((unused)) JNIEnv *env,   __attribute__((unused)) jobject thiz)
{
    int32_t iValue = 0;

    if (getMiSysService_V30() == nullptr) {
      ALOGI("Unable to initialize the HIDL misys@3.0");
      return (jint)iValue;
    }

    if (init == false) {
      ALOGI("set init to true");
      init = true;
      iValue = 1;
    }

   return (jint)iValue;
}

/*
 *
 *
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    getFileSize
 * Signature:
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Param [in]
 * Return - current value for this parameter returned as a string
 */
JNIEXPORT jlong JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_getFileSize
  (__attribute__((unused)) JNIEnv *env,__attribute__((unused)) jobject thiz, jstring path, jstring name)
{
    uint64_t fileSize = 0;
    IResultValue result = IResultValue::MISYS_EINVAL;

    if (init == false) {
      ALOGE("libmisys_jni.xiaomi not init, return fileSize is -1\n");
      fileSize = -1;
      return (jlong)fileSize;
    }

    // convert value string parameter to int
    const char *nativePath = env->GetStringUTFChars(path, 0);
    const char *nativeName = env->GetStringUTFChars(name, 0);

    // validate the parameter
    if (nativePath == nullptr || nativeName == nullptr) {
      ALOGI("%s parameter is invalid, return fileSize is -1\n", __func__);
      fileSize = -1;
      goto err;
    }

    ALOGV("%s nativePath = %s, nativeName = %s\n", __func__, nativePath, nativeName);

    service_V30->GetFileSize(nativePath, nativeName,
           [&](auto&IFileSizeResult) {
        fileSize = IFileSizeResult.fileSize;
        result = IFileSizeResult.result;

        if (result != IResultValue::MISYS_SUCCESS) {
          ALOGI("%s GetFileSize() failed(), result=%d\n", __func__, result);
        }
    });

    if (result != IResultValue::MISYS_SUCCESS) {
      ALOGE("%s GetFileSize() failed(), result = %d\n", __func__, result);
      fileSize = -1;
    }

err:
    // **MUST** release native strings
    env->ReleaseStringUTFChars(path, nativePath);
    env->ReleaseStringUTFChars(name, nativeName);

    return (jlong)fileSize;
}

/*
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    ReadFromFile
 * Signature:
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Param [in]  sParamType - type specified as string
 * Return - current value for this parameter returned as a string
 */
JNIEXPORT jbyteArray JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_readFromFile
  (JNIEnv *env,__attribute__((unused)) jobject thiz, jstring path, jstring name, jlong fileSize)
{
    uint64_t mSize = 0;
    IResultValue result = IResultValue::MISYS_SUCCESS;
    struct IBlockResult block;
    jbyteArray jArray = NULL;
    sp<IMemory> m = NULL;
    jbyte *by = nullptr;
    uint8_t * data = nullptr;

    if (init == false) {
      ALOGE("libmisys_jni.xiaomi not init, return\n");
      return NULL;
    }

     // convert value string parameter to int
    const char *nativePath = env->GetStringUTFChars(path, 0);
    const char *nativeName = env->GetStringUTFChars(name, 0);

    // validate the parameter
    if ((nativePath == nullptr) || (nativeName == nullptr) || (fileSize < 0)) {
      result = IResultValue::MISYS_EINVAL;
      ALOGI("%s parameter is invalid,return\n", __func__);
      goto err0;
    }

    //Allocate MemoryBlock for read
    if (fileSize % kHeapSizeAlignment != 0) {
      mSize = ((fileSize / kHeapSizeAlignment) + 3) * kHeapSizeAlignment;
    } else {
      mSize = fileSize;
    }
    // release the nativePath and nativeName we requested
    if (mSize == 0) {
      result = IResultValue::MISYS_EINVAL;
      ALOGE("mSize == 0, abort\n");
      goto err0;
    }

    service_V30->AllocateBlock(mSize,
        [&](auto& BlockResult) {
        block.result = BlockResult.result;
        block.block = BlockResult.block;

        ALOGV("%s result = %d\n", __func__, block.result);
    });

    // release the nativePath and nativeName we requested
    if (block.result != IResultValue::MISYS_SUCCESS) {
      ALOGE("%s AllocateBlock() failed(), result = %d\n", __func__, result);
      goto err0;
    }

    service_V30->ReadFromFile(block.block,
        nativePath,
        nativeName,
        [&](auto& IBlockResult) {
        block.result = IBlockResult.result;
        block.block = IBlockResult.block;

        ALOGV("%s ReadFromFile() result = %d\n", __func__, block.result);
    });

    // release the memory block and nativePath/nativeName
    if (block.result != IResultValue::MISYS_SUCCESS) {
      ALOGE("%s ReadFromFile() failed(), result = %d\n", __func__, result);
      goto err;
    }

    m = android::hardware::mapMemory(block.block);

    data = static_cast<uint8_t*>(static_cast<void*>(m->getPointer()));
    // release the memory block
    if (data == NULL) {
      ALOGE("data is null\n");
      goto err;
    }

    by = (jbyte*)data;
    jArray = env->NewByteArray(fileSize);
    env->SetByteArrayRegion(jArray, 0, fileSize, by);

err:
    // release MemoryBLock
    service_V30->ReleaseBlock(block.block);

err0:
    // **MUST** release native strings
    env->ReleaseStringUTFChars(path, nativePath);
    env->ReleaseStringUTFChars(name, nativeName);

    if (result != IResultValue::MISYS_SUCCESS) return NULL;

    if (block.result != IResultValue::MISYS_SUCCESS) {
      return NULL;
    } else {
      return jArray;
    }
}

/*
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    WriteToFile
 * Signature:
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Return - current value for this parameter returned as a string
 */
JNIEXPORT jint JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_writeToFile
  (JNIEnv *env, __attribute__((unused)) jobject thiz, jbyteArray jArray, jstring path, jstring name, jlong fileSize)
{
    uint64_t mSize = 0;
    jint jResult = -1;
    IResultValue result = IResultValue::MISYS_EINVAL;
    struct IBlockResult block;
    jbyte* srcData = NULL;
    int64_t jSize = 0;

    sp<IMemory> m = nullptr;
    uint8_t * mData = NULL;

    if (init == false) {
      ALOGE("libmisys_jni.xiaomi not init, return\n");
      return jResult;
    }

    // convert value string parameter
    const char *nativePath = env->GetStringUTFChars(path, 0);
    const char *nativeName = env->GetStringUTFChars(name, 0);

    // validate the parameter
    if ((nativePath == nullptr) || (nativeName == nullptr) || (fileSize <= 0)) {
      ALOGI("%s parameter is invalid,return\n", __func__);
      goto err0;
    }

    srcData = env->GetByteArrayElements(jArray, NULL);

    if (srcData == NULL) {
      ALOGI("%s write data is invalid,return\n", __func__);
      goto err0;
    }

    jSize = env->GetArrayLength(jArray);

    if (jSize != fileSize) {
      ALOGI("%s write data is invalid,return\n", __func__);
      goto err1;
    }

    //Allocate MemoryBlock for read
    if (fileSize % kHeapSizeAlignment != 0) {
      mSize = ((fileSize / kHeapSizeAlignment) + 3) * kHeapSizeAlignment;
    } else {
      mSize = fileSize;
    }

    if (mSize == 0) {
      ALOGE("mSize == 0, abort\n");
      goto err1;
    }

    service_V30->AllocateBlock(mSize,
        [&](auto& BlockResult) {
        block.result = BlockResult.result;
        block.block = BlockResult.block;

        ALOGV("%s result = %d\n", __func__, block.result);
    });

    if (block.result != IResultValue::MISYS_SUCCESS) {
      ALOGE("%s AllocateBlock() failed(), result = %d\n", __func__, result);
      goto err1;
    }

    m = android::hardware::mapMemory(block.block);

    if (m == nullptr) {
      ALOGE("handlePath return error\n");
      goto err2;
    }

    // copy the data for write to file
    mData = static_cast<uint8_t*>(static_cast<void*>(m->getPointer()));

    if (mData == NULL) {
      ALOGE("data is null\n");
      goto err2;
    }

    m->update();

    memcpy(mData, srcData, mSize);

    m->commit();

    // call HIDL for write
    service_V30->WriteToFile(block.block,
        nativePath,
        nativeName,
        fileSize);

    if (block.result != IResultValue::MISYS_SUCCESS) {
      ALOGE("%s WriteToFile() failed(), result = %d\n", __func__, result);
      goto err2;
    }

    jResult = 1;

err2:
    // release MemoryBLock
    service_V30->ReleaseBlock(block.block);

err1:
    // release the jni data structure
    env->ReleaseByteArrayElements(jArray, srcData, JNI_ABORT);

err0:
    // **MUST** release native strings
    env->ReleaseStringUTFChars(path, nativePath);
    env->ReleaseStringUTFChars(name, nativeName);

    return jResult;
}

/*
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    setProp
 * Signature:
 *
 * Param [in]  env - ptr to JNIEnv
 * Param [in]  thiz - ptr to java class object this method belongs to
 * Return - current value for this parameter returned as a string
 */

JNIEXPORT jboolean JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_setProp
  (JNIEnv *env, __attribute__((unused)) jobject thiz, jstring key, jstring value)
{
    jboolean jResult = false;

    IResultValue testResult;

    if (init == false) {
      ALOGE("libmisys_jni.xiaomi not init, return\n");
      return jResult;
    }

    // convert value string parameter
    const char *nativeKey = env->GetStringUTFChars(key, 0);
    const char *nativeValue = env->GetStringUTFChars(value, 0);

    // validate the parameter
    if ((nativeKey == nullptr) || (nativeValue == nullptr)) {
      ALOGI("%s parameter is invalid,return\n", __func__);
      goto err;
    }


    testResult = service_V30->MiSysSetProp(nativeKey, nativeValue);

    ALOGV("%s SetProp() result = %d\n", __func__, testResult);

    if (testResult == IResultValue::MISYS_SUCCESS) {
      jResult = true;
    }

err:
    // **MUST** release native strings
    env->ReleaseStringUTFChars(key, nativeKey);
    env->ReleaseStringUTFChars(value, nativeValue);

    return jResult;
}

#ifdef __cplusplus
}
#endif // __cplusplus
