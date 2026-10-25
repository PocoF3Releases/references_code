#ifndef ANDROID_OS_STATISTICS_VMUTILS_H
#define ANDROID_OS_STATISTICS_VMUTILS_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <atomic>
#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <cutils/compiler.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

#include "javavmsupervision_callbacks.h"

namespace android {
namespace os {
namespace statistics {

extern struct android::os::statistics::JavaVMInterface *g_pJavaVMInterface;

class JavaVMUtils {
public:
  inline static void setThreadPerfSupervisionOn(JNIEnv* env, bool isOn)
  {
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (pVMInterface != nullptr) {
      pVMInterface->setThreadPerfSupervisionOn(env, isOn);
    }
  }

  inline static bool isThreadPerfSupervisionOn(JNIEnv* env) {
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (CC_LIKELY(pVMInterface != nullptr)) {
      return pVMInterface->isThreadPerfSupervisionOn(env);
    } else {
      return false;
    }
  }

  inline static void getThreadInfo(JNIEnv* env,
    int32_t& thread_id, std::shared_ptr<std::string>& thread_name) {
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (CC_LIKELY(pVMInterface != nullptr)) {
      pVMInterface->getThreadInfo(env, thread_id, thread_name);
    } else {
      thread_id = 0;
      thread_name.reset();
    }
  }

  inline static jclass getCurrentClass(JNIEnv* env) {
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (CC_LIKELY(pVMInterface != nullptr)) {
      return pVMInterface->getCurrentClass(env);
    } else {
      return nullptr;
    }
  }

  inline static jobject createJavaStackBackTrace(JNIEnv* env, bool needFillIn) {
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (CC_LIKELY(pVMInterface != nullptr)) {
      return pVMInterface->createJavaStackBackTrace(env, needFillIn);
    } else {
      return nullptr;
    }
  }

  inline static void fillInJavaStackBackTrace(JNIEnv* env, jobject stackBackTrace) {
    if (CC_LIKELY(stackBackTrace != nullptr)) {
      struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
      if (pVMInterface != nullptr) {
        pVMInterface->fillInJavaStackBackTrace(env, stackBackTrace);
      }
    }
  }

  inline static void resetJavaStackBackTrace(JNIEnv* env, jobject stackBackTrace) {
    if (CC_UNLIKELY(stackBackTrace == nullptr)) return;
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (pVMInterface != nullptr) {
      pVMInterface->resetJavaStackBackTrace(env, stackBackTrace);
    }
  }

  inline static jobject cloneJavaStackBackTrace(JNIEnv* env, jobject stackBackTrace) {
    if (CC_UNLIKELY(stackBackTrace == nullptr)) return nullptr;
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (pVMInterface != nullptr) {
      return pVMInterface->cloneJavaStackBackTrace(env, stackBackTrace);
    } else {
      return nullptr;
    }
  }

  inline static jobjectArray resolveJavaStackTrace(JNIEnv* env, jobject stackBackTrace) {
    if (CC_UNLIKELY(stackBackTrace == nullptr)) return nullptr;
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (CC_LIKELY(pVMInterface != nullptr)) {
      return pVMInterface->resolveJavaStackTrace(env, stackBackTrace);
    } else {
      return nullptr;
    }
  }

  inline static jobjectArray resolveClassesOfBackTrace(JNIEnv* env, jobject stackBackTrace) {
    if (CC_UNLIKELY(stackBackTrace == nullptr)) return nullptr;
    struct JavaVMInterface * pVMInterface = g_pJavaVMInterface;
    if (CC_LIKELY(pVMInterface != nullptr)) {
      return pVMInterface->resolveClassesOfBackTrace(env, stackBackTrace);
    } else {
      return nullptr;
    }
  }
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_VMUTILS_H
