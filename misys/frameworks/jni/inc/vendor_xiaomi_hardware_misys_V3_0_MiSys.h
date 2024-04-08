/*
 *    Copyright (c) 2019 Xiaomi Technologies, Inc. All Rights Reserved.
 *    Xiaomi Technologies Proprietary and Confidential.
 */
#include <jni.h>
#ifndef VENDOR_XIAOMI_HARDWARE_MISYS_MISYS_V3_10_H
#define VENDOR_XIAOMI_HARDWARE_MISYS_MISYS_V3_10_H

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_init
 * Signature: TBD
 */
JNIEXPORT jint JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_init
  (JNIEnv *, jobject);

/*
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_getFileSize
 * Signature: TBD
 */
JNIEXPORT jlong JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_getFileSize
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_readFromFile
 * Signature: TBD
 */
JNIEXPORT jbyteArray JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_readFromFile
  (JNIEnv *, jobject, jstring, jstring, jlong);

/*
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_WriteToFile
 * Signature: TBD
 */
JNIEXPORT jint JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_WriteToFile
  (JNIEnv *, jobject, jbyteArray, jstring, jstring, jlong);

/*
 * Class:     vendor_xiaomi_hardware_misys_V3_10_MiSys
 * Method:    Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_setProp
 * Signature: TBD
 */
JNIEXPORT jboolean JNICALL Java_vendor_xiaomi_hardware_misys_V3_10_MiSys_setProp
  (JNIEnv *, jobject, jstring, jstring);

#ifdef __cplusplus
}
#endif
#endif  // VENDOR_XIAOMI_HARDWARE_MISYS_V3_0_MISYS_JNI_H
