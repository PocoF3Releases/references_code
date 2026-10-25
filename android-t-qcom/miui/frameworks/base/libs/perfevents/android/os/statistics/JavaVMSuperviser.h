#ifndef ANDROID_OS_STATISTICS_JAVAVMSUPERVISER_H
#define ANDROID_OS_STATISTICS_JAVAVMSUPERVISER_H

#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "JNIHelp.h"
#include "jni.h"
#include "core_jni_helpers.h"

namespace android {
namespace os {
namespace statistics {

class JavaVMSuperviser {
public:
  static void init(JNIEnv* env);
};

} //namespace statistics
} //namespace os
} //namespace android

#endif //ANDROID_OS_STATISTICS_JAVAVMSUPERVISER_H
