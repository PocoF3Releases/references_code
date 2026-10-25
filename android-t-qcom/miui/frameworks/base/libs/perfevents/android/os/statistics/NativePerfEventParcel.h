#ifndef ANDROID_OS_STATISTICS_NATIVEPERFEVENTPARCEL_H
#define ANDROID_OS_STATISTICS_NATIVEPERFEVENTPARCEL_H

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#include <string>
#include <cutils/compiler.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>

#include "jni.h"

#include "ShortCString.h"
#include "JavaPerfEventParcel.h"
#include "JniParcelWriter.h"

namespace android {
namespace os {
namespace statistics {

#define NATIVEPERFEVENTPARCEL_NUMBER_MAX_COUNT 16
#define NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT 6

#define FIELD_TYPE_INT32 0LL
#define FIELD_TYPE_INT64 1LL
#define FIELD_TYPE_OBJECT 2LL
#define FIELD_TYPE_MASK 3LL

class NativePerfEventParcel {
private:
  enum ObjectType {
    TYPE_UNKNOWN = 0,
    TYPE_CSTRING = 1,
    TYPE_SHORTCSTRING = 2,
    TYPE_BINDER = 3,
    TYPE_JSTRING = 4,
    TYPE_JOBJECT = 5,
    TYPE_PROCESS_NAME = 6,
    TYPE_PACKAGE_NAME = 7,
  };
  struct ObjectData {
    std::shared_ptr<std::string> mCString;
    union {
      ShortCString* mShortCString;
      IBinder* mBinder;
      jstring mGlobalJString;
    };
  };
public:
  NativePerfEventParcel();
  ~NativePerfEventParcel();
  inline bool hasProcLazyInfo() { return mHasProcLazyInfo; }
  void reset(JNIEnv* env);
  void writeInt32(int32_t value);
  void writeInt64(int64_t value);
  void writeCString(const std::shared_ptr<std::string>& value);
  void writeShortCString(char* value);
  void writeBinder(IBinder* value);
  void writeJString(JNIEnv* env, jstring str);
  void writeJObject(JNIEnv* env, jobject obj);
  void writeProcessName();
  void writePackageName();
  void readToJniParcel(JNIEnv* env, JniParcelWriter& jniParcelWriter);
  void readToParcel(JNIEnv* env, Parcel* parcel);
private:
  int64_t mFieldTypeFlags;
  int8_t mFieldCount;
  int8_t mNumberCount;
  int8_t mObjectCount;
  bool mHasProcLazyInfo;

  int64_t mNumbers[NATIVEPERFEVENTPARCEL_NUMBER_MAX_COUNT];

  uint8_t mObjectTypes[NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT];
  struct ObjectData mObjects[NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT];
  JavaPerfEventParcel* mJavaParcel;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif
