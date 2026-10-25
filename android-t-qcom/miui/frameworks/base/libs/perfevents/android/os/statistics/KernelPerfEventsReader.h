#ifndef ANDROID_OS_STATISTICS_KERNELPERFEVENTSREADER_H
#define ANDROID_OS_STATISTICS_KERNELPERFEVENTSREADER_H

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

namespace android {
namespace os {
namespace statistics {

class KernelPerfEventsBuffer;

class KernelPerfEventsReader {
public:
  static void start(JNIEnv* env);
  static KernelPerfEventsReader* openProcKernelPerfEventsReader(uint32_t bufferSize);
  static KernelPerfEventsReader* openDeviceKernelPerfEventsReader(uint32_t bufferSize);
  static KernelPerfEventsReader* getProcKernelPerfEventsReader();
private:
  static KernelPerfEventsReader* openKernelPerfEventsReader(const char* filePath, uint32_t bufferSize);

public:
  inline int getfd() {
    return m_fd;
  }
  bool isEmpty();
  int readPerfEvents(JNIEnv* env, jobjectArray resultBuffer);

private:
  int m_fd;
  KernelPerfEventsBuffer* m_buffer;
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_KERNELPERFEVENTSREADER_H
