#include <jni.h>

#ifndef _Included_com_xiaomi_finddevice_common_PriviledgedProc
#define _Included_com_xiaomi_finddevice_common_PriviledgedProc
#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeExists
  (JNIEnv *, jclass, jstring, jbooleanArray);


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeRemove
  (JNIEnv *, jclass, jstring);


JNIEXPORT jobjectArray JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeLsDir
  (JNIEnv *, jclass, jstring);


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeIsFile
  (JNIEnv *, jclass, jstring, jbooleanArray);


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeIsDir
  (JNIEnv *, jclass, jstring, jbooleanArray);


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeMove
  (JNIEnv *, jclass, jstring, jstring);


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeMkdir
  (JNIEnv *, jclass, jstring);

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeWriteTinyFile
  (JNIEnv *, jclass, jstring, jboolean, jbyteArray);


JNIEXPORT jbyteArray JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeReadTinyFile
  (JNIEnv *, jclass, jstring);

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeAddService
  (JNIEnv *, jclass, jstring, jobject);

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeGetSetRebootClearVariable
  (JNIEnv *, jclass, jstring, jobjectArray, jboolean, jstring);

JNIEXPORT jint JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_getLastErrorCode
  (JNIEnv *, jclass);

JNIEXPORT jstring JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_getErrorMessage
  (JNIEnv *, jclass, jint);

#ifdef __cplusplus
}
#endif
#endif
