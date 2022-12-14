#define LOG_TAG "JniParcel"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <sys/types.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

using namespace android;

static struct jniparcel_offsets_t
{
  // Class state.
  jclass mClass;
  jmethodID mWriteLong[13];
  jmethodID mWriteObject[10];
} gkJniParcelOffsets;

static const char* const kJniParcelPathName = "android/os/statistics/JniParcel";

int register_android_os_statistics_JniParcel(JNIEnv* env)
{
  jclass clazz = FindClassOrDie(env, kJniParcelPathName);
  gkJniParcelOffsets.mClass = MakeGlobalRefOrDie(env, clazz);

  gkJniParcelOffsets.mWriteLong[0] = 0;
  gkJniParcelOffsets.mWriteLong[1] = GetMethodIDOrDie(env, clazz, "writeLong1", "(J)V");
  gkJniParcelOffsets.mWriteLong[2] = GetMethodIDOrDie(env, clazz, "writeLong2", "(JJ)V");
  gkJniParcelOffsets.mWriteLong[3] = GetMethodIDOrDie(env, clazz, "writeLong3", "(JJJ)V");
  gkJniParcelOffsets.mWriteLong[4] = GetMethodIDOrDie(env, clazz, "writeLong4", "(JJJJ)V");
  gkJniParcelOffsets.mWriteLong[5] = GetMethodIDOrDie(env, clazz, "writeLong5", "(JJJJJ)V");
  gkJniParcelOffsets.mWriteLong[6] = GetMethodIDOrDie(env, clazz, "writeLong6", "(JJJJJJ)V");
  gkJniParcelOffsets.mWriteLong[7] = GetMethodIDOrDie(env, clazz, "writeLong7", "(JJJJJJJ)V");
  gkJniParcelOffsets.mWriteLong[8] = GetMethodIDOrDie(env, clazz, "writeLong8", "(JJJJJJJJ)V");
  gkJniParcelOffsets.mWriteLong[9] = GetMethodIDOrDie(env, clazz, "writeLong9", "(JJJJJJJJJ)V");
  gkJniParcelOffsets.mWriteLong[10] = GetMethodIDOrDie(env, clazz, "writeLong10", "(JJJJJJJJJJ)V");
  gkJniParcelOffsets.mWriteLong[11] = GetMethodIDOrDie(env, clazz, "writeLong11", "(JJJJJJJJJJJ)V");
  gkJniParcelOffsets.mWriteLong[12] = GetMethodIDOrDie(env, clazz, "writeLong12", "(JJJJJJJJJJJJ)V");

  gkJniParcelOffsets.mWriteObject[0] = 0;
  gkJniParcelOffsets.mWriteObject[1] = GetMethodIDOrDie(env, clazz, "writeObject1",
    "(Ljava/lang/Object;)V");
  gkJniParcelOffsets.mWriteObject[2] = GetMethodIDOrDie(env, clazz, "writeObject2",
    "(Ljava/lang/Object;Ljava/lang/Object;)V");
  gkJniParcelOffsets.mWriteObject[3] = GetMethodIDOrDie(env, clazz, "writeObject3",
    "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
  gkJniParcelOffsets.mWriteObject[4] = GetMethodIDOrDie(env, clazz, "writeObject4",
    "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
  gkJniParcelOffsets.mWriteObject[5] = GetMethodIDOrDie(env, clazz, "writeObject5",
    "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
  gkJniParcelOffsets.mWriteObject[6] = GetMethodIDOrDie(env, clazz, "writeObject6",
    "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
  gkJniParcelOffsets.mWriteObject[7] = GetMethodIDOrDie(env, clazz, "writeObject7",
    "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
  gkJniParcelOffsets.mWriteObject[8] = GetMethodIDOrDie(env, clazz, "writeObject8",
    "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
  gkJniParcelOffsets.mWriteObject[9] = GetMethodIDOrDie(env, clazz, "writeObject9",
    "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");

  return 0;
}

void writeLongsToJniParcel(JNIEnv* env, jobject jniParcel, int32_t count, jvalue* values)
{
  env->CallVoidMethodA(jniParcel, gkJniParcelOffsets.mWriteLong[count], values);
}

void writeObjectsToJniParcel(JNIEnv* env, jobject jniParcel, int32_t count, jvalue* objects)
{
  env->CallVoidMethodA(jniParcel, gkJniParcelOffsets.mWriteObject[count], objects);
}
