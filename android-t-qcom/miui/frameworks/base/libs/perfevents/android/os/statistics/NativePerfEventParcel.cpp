#define LOG_TAG "NativePerfEventParcel"

#include <stdlib.h>
#include <utils/String16.h>

#include "PerfSuperviser.h"
#include "NativePerfEventParcel.h"
#include "ShortCStringCache.h"
#include "JavaPerfEventParcelCache.h"

namespace android {
namespace os {
namespace statistics {

NativePerfEventParcel::NativePerfEventParcel() {
  mFieldTypeFlags = 0;
  mFieldCount = 0;
  mNumberCount = 0;
  mObjectCount = 0;
  mHasProcLazyInfo = false;
  memset(mNumbers, 0, sizeof(mNumbers));
  memset(mObjectTypes, 0, sizeof(mObjectTypes));
  memset(mObjects, 0, sizeof(mObjects));
  mJavaParcel = nullptr;
}

NativePerfEventParcel::~NativePerfEventParcel() {
}

void NativePerfEventParcel::reset(JNIEnv* env) {
  for (int i = 0; i < mObjectCount; i++) {
    switch (mObjectTypes[i]) {
      case TYPE_CSTRING:
        mObjects[i].mCString.reset();
        break;
      case TYPE_SHORTCSTRING:
        if (mObjects[i].mShortCString != nullptr) {
          ShortCStringCache::recycle(mObjects[i].mShortCString);
          mObjects[i].mShortCString = nullptr;
        }
        break;
      case TYPE_BINDER:
        if (mObjects[i].mBinder != nullptr) {
          mObjects[i].mBinder->decStrong((void *)this);
          mObjects[i].mBinder = nullptr;
        }
        break;
    }
    mObjectTypes[i] = TYPE_UNKNOWN;
  }
  if (env != nullptr && mJavaParcel != nullptr) {
    JavaPerfEventParcelCache::recycle(env, mJavaParcel);
    mJavaParcel = nullptr;
  }
  mNumberCount = 0;
  mHasProcLazyInfo = false;
  mObjectCount = 0;
  mFieldCount = 0;
  mFieldTypeFlags = 0;
}

void NativePerfEventParcel::writeInt32(int32_t value) {
  if (CC_LIKELY(mNumberCount < NATIVEPERFEVENTPARCEL_NUMBER_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_INT32 << (mFieldCount << 1));
    mNumbers[mNumberCount] = value;
    mFieldCount++;
    mNumberCount++;
  }
}

void NativePerfEventParcel::writeInt64(int64_t value) {
  if (CC_LIKELY(mNumberCount < NATIVEPERFEVENTPARCEL_NUMBER_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_INT64 << (mFieldCount << 1));
    mNumbers[mNumberCount] = value;
    mFieldCount++;
    mNumberCount++;
  }
}

void NativePerfEventParcel::writeCString(const std::shared_ptr<std::string>& value) {
  if (CC_LIKELY(mObjectCount < NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_OBJECT << (mFieldCount << 1));
    mObjectTypes[mObjectCount] = TYPE_CSTRING;
    mObjects[mObjectCount].mCString = value;
    mFieldCount++;
    mObjectCount++;
  }
}

void NativePerfEventParcel::writeShortCString(char* value) {
  if (CC_LIKELY(mObjectCount < NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_OBJECT << (mFieldCount << 1));
    mObjectTypes[mObjectCount] = TYPE_SHORTCSTRING;
    ShortCString* shortCString = nullptr;
    if (CC_LIKELY(value != nullptr)) {
      shortCString = ShortCStringCache::obtain();
      shortCString->write(value);
    }
    mObjects[mObjectCount].mShortCString = shortCString;
    mFieldCount++;
    mObjectCount++;
  }
}

void NativePerfEventParcel::writeBinder(IBinder* value) {
  if (CC_LIKELY(mObjectCount < NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_OBJECT << (mFieldCount << 1));
    mObjectTypes[mObjectCount] = TYPE_BINDER;
    mObjects[mObjectCount].mBinder = value;
    if (value != nullptr) {
      value->incStrong((void *)this);
    }
    mFieldCount++;
    mObjectCount++;
    mHasProcLazyInfo = true;
  }
}

void NativePerfEventParcel::writeJString(JNIEnv* env, jstring str) {
  if (CC_LIKELY(mObjectCount < NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_OBJECT << (mFieldCount << 1));
    if (mJavaParcel == nullptr) {
      mJavaParcel = JavaPerfEventParcelCache::obtain(env);
    }
    mObjectTypes[mObjectCount] = TYPE_JSTRING;
    mJavaParcel->writeObject(env, str);
    mFieldCount++;
    mObjectCount++;
  }
}

void NativePerfEventParcel::writeJObject(JNIEnv* env, jobject obj) {
  if (CC_LIKELY(mObjectCount < NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_OBJECT << (mFieldCount << 1));
    if (mJavaParcel == nullptr) {
      mJavaParcel = JavaPerfEventParcelCache::obtain(env);
    }
    mObjectTypes[mObjectCount] = TYPE_JOBJECT;
    mJavaParcel->writeObject(env, obj);
    mFieldCount++;
    mObjectCount++;
    mHasProcLazyInfo = true;
  }
}

void NativePerfEventParcel::writeProcessName() {
  if (CC_LIKELY(mObjectCount < NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_OBJECT << (mFieldCount << 1));
    mObjectTypes[mObjectCount] = TYPE_PROCESS_NAME;
    mFieldCount++;
    mObjectCount++;
  }
}

void NativePerfEventParcel::writePackageName() {
  if (CC_LIKELY(mObjectCount < NATIVEPERFEVENTPARCEL_OBJECT_MAX_COUNT)) {
    mFieldTypeFlags |= (FIELD_TYPE_OBJECT << (mFieldCount << 1));
    mObjectTypes[mObjectCount] = TYPE_PACKAGE_NAME;
    mFieldCount++;
    mObjectCount++;
  }
}

void NativePerfEventParcel::readToJniParcel(JNIEnv* env, JniParcelWriter& jniParcelWriter) {
  for (int i = 0; i < mNumberCount; i++) {
    jniParcelWriter.writeLong(env, mNumbers[i]);
  }
  for (int i = 0; i < mObjectCount; i++) {
    switch (mObjectTypes[i]) {
      case TYPE_CSTRING:
        jniParcelWriter.writeString(env, mObjects[i].mCString.get());
        mObjects[i].mCString.reset();
        break;
      case TYPE_SHORTCSTRING:
        if (mObjects[i].mShortCString != nullptr) {
          jniParcelWriter.writeString(env, mObjects[i].mShortCString->value);
          ShortCStringCache::recycle(mObjects[i].mShortCString);
          mObjects[i].mShortCString = nullptr;
        } else {
          jniParcelWriter.writeString(env, (char* )nullptr);
        }
        break;
      case TYPE_BINDER:
        jniParcelWriter.writeBinder(env, mObjects[i].mBinder);
        if (mObjects[i].mBinder != nullptr) {
          mObjects[i].mBinder->decStrong((void *)this);
          mObjects[i].mBinder = nullptr;
        }
        break;
      case TYPE_JSTRING:
        jniParcelWriter.writeObject(env, mJavaParcel->readObject(env));
        break;
      case TYPE_JOBJECT:
        jniParcelWriter.writeObject(env, mJavaParcel->readObject(env));
        break;
      case TYPE_PROCESS_NAME:
        jniParcelWriter.writeObject(env, PerfSuperviser::getProcessName());
        break;
      case TYPE_PACKAGE_NAME:
        jniParcelWriter.writeObject(env, PerfSuperviser::getPackageName());
        break;
    }
    mObjectTypes[i] = TYPE_UNKNOWN;
  }
  if (env != nullptr && mJavaParcel != nullptr) {
    JavaPerfEventParcelCache::recycle(env, mJavaParcel);
    mJavaParcel = nullptr;
  }
  mNumberCount = 0;
  mObjectCount = 0;
  mFieldCount = 0;
  mHasProcLazyInfo = false;
  mFieldTypeFlags = 0;
}

static inline void writeJStringToParcel(JNIEnv* env, Parcel* parcel, jstring value) {
  if (value != nullptr) {
    const jchar* str = env->GetStringCritical(value, 0);
    if (str) {
      parcel->writeString16(
        reinterpret_cast<const char16_t*>(str),
        env->GetStringLength(value));
      env->ReleaseStringCritical(value, str);
    } else {
      parcel->writeString16(nullptr, 0);
    }
  } else {
    parcel->writeString16(nullptr, 0);
  }
}

void NativePerfEventParcel::readToParcel(JNIEnv* env, Parcel* parcel) {
  int8_t numberPos = 0;
  int8_t objectPos = 0;
  for (int8_t i = 0; i < mFieldCount; i++) {
    int64_t fieldType = (mFieldTypeFlags & (FIELD_TYPE_MASK << (i << 1))) >> (i << 1);

    if (fieldType == FIELD_TYPE_INT32) {
      parcel->writeInt32((int32_t)mNumbers[numberPos]);
      numberPos++;
      continue;
    }

    if (fieldType == FIELD_TYPE_INT64) {
      parcel->writeInt64(mNumbers[numberPos]);
      numberPos++;
      continue;
    }

    switch (mObjectTypes[objectPos]) {
      case TYPE_CSTRING: {
        std::string* str = mObjects[objectPos].mCString.get();
        if (str != nullptr) {
          String16 value(str->c_str());
          parcel->writeString16(value);
        } else {
          parcel->writeString16(nullptr, 0);
        }
        mObjects[objectPos].mCString.reset();
        break;
      }
      case TYPE_SHORTCSTRING:
        if (mObjects[objectPos].mShortCString != nullptr) {
          String16 value(mObjects[objectPos].mShortCString->value);
          parcel->writeString16(value);
          ShortCStringCache::recycle(mObjects[objectPos].mShortCString);
          mObjects[objectPos].mShortCString = nullptr;
        } else {
          parcel->writeString16(nullptr, 0);
        }
        break;
      case TYPE_BINDER:
        abort(); //不应该走入该路径
        break;
      case TYPE_JSTRING: {
        jstring value = (jstring)mJavaParcel->readObject(env);
        writeJStringToParcel(env, parcel, value);
        env->DeleteLocalRef(value);
        break;
      }
      case TYPE_JOBJECT:
        abort(); //不应该走入该路径
        break;
      case TYPE_PROCESS_NAME:
        writeJStringToParcel(env, parcel, PerfSuperviser::getProcessName());
        break;
      case TYPE_PACKAGE_NAME:
        writeJStringToParcel(env, parcel, PerfSuperviser::getPackageName());
        break;
    }
    mObjectTypes[objectPos] = TYPE_UNKNOWN;
    objectPos++;
  }
  if (env != nullptr && mJavaParcel != nullptr) {
    JavaPerfEventParcelCache::recycle(env, mJavaParcel);
    mJavaParcel = nullptr;
  }
  mNumberCount = 0;
  mObjectCount = 0;
  mHasProcLazyInfo = false;
  mFieldCount = 0;
  mFieldTypeFlags = 0;
}

} //namespace statistics
} //namespace os
} //namespace android
