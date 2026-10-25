#define ATRACE_TAG ATRACE_TAG_RESOURCES
#define LOG_TAG "asset"

#include <inttypes.h>
#include <linux/capability.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <private/android_filesystem_config.h> // for AID_SYSTEM

#include <sstream>
#include <string>

#include "android-base/logging.h"
#include "android-base/properties.h"
#include "android-base/stringprintf.h"
#include "android_runtime/android_util_AssetManager.h"
#include "android_runtime/AndroidRuntime.h"
#include "android_util_Binder.h"
#include "androidfw/Asset.h"
#include "androidfw/AssetManager.h"
#include "androidfw/AssetManager2.h"
#include "androidfw/AttributeResolution.h"
#include "androidfw/MutexGuard.h"
#include "androidfw/PosixUtils.h"
#include "androidfw/ResourceTypes.h"
#include "androidfw/ResourceUtils.h"

#include "core_jni_helpers.h"
#include "jni.h"
#include "nativehelper/JNIHelp.h"
#include "nativehelper/ScopedPrimitiveArray.h"
#include "nativehelper/ScopedStringChars.h"
#include "nativehelper/ScopedUtfChars.h"
#include "utils/Log.h"
#include "utils/misc.h"
#include "utils/String8.h"
#include "utils/Trace.h"

using ::android::base::StringPrintf;
using ::android::util::ExecuteBinary;

using namespace android;

jclass g_stringClass = nullptr;

static jobjectArray NativeCreateIdmapsForStaticOverlaysTargetingMiui(JNIEnv* env,
        jclass /*clazz*/) {
  if (access("/system/bin/idmap2", X_OK) == -1) {
    PLOG(WARNING) << "unable to execute idmap2";
    return nullptr;
  }

  std::vector<std::string> argv{"/system/bin/idmap2",
      "create-multiple",
      "--target-apk-path", "/system_ext/framework/framework-ext-res/framework-ext-res.apk",
      "--overlay-apk-path", "/product/overlay/MiuiFrameworkResOverlay.apk",
      "--overlay-apk-path", "/product/overlay/CcmMiuiFrameworkResOverlay.apk",
      "--overlay-apk-path", "/product/overlay/FrameworkExtResCustomizedRegionOverlay.apk",
      "--overlay-apk-path", "/product/overlay/MiuiProcessResOverlay.apk",
      "--overlay-apk-path", "/product/overlay/MiuiDefaultConfigOverlay.apk",
      "--policy", "public",
      "--policy", "product",
      "--policy", "signature",
  };

  const auto result = ExecuteBinary(argv);

  if (!result) {
      LOG(ERROR) << "failed to execute idmap2 create-multiple";
      return nullptr;
  }

  if (result->status != 0) {
    // LOG(ERROR) << "idmap2: " << result->stderr;
    LOG(ERROR) << "idmap2: " << result->stderr_str;
    return nullptr;
  }

  // Return the paths of the idmaps created or updated during the idmap invocation.
  std::vector<std::string> idmap_paths;
  // std::istringstream input(result->stdout);
  std::istringstream input(result->stdout_str);
  std::string path;
  while (std::getline(input, path)) {
    idmap_paths.push_back(path);
  }

  jobjectArray array = env->NewObjectArray(idmap_paths.size(), g_stringClass, nullptr);
  if (array == nullptr) {
    return nullptr;
  }
  for (size_t i = 0; i < idmap_paths.size(); i++) {
    const std::string path = idmap_paths[i];
    jstring java_string = env->NewStringUTF(path.c_str());
    if (env->ExceptionCheck()) {
      return nullptr;
    }
    env->SetObjectArrayElement(array, i, java_string);
    env->DeleteLocalRef(java_string);
  }
  return array;
}

static const JNINativeMethod methods[] = {
     {"nativeCreateIdmapsForStaticOverlaysTargetingMiui", "()[Ljava/lang/String;",
         (void*)NativeCreateIdmapsForStaticOverlaysTargetingMiui},
};

static const char* const kClassPathName = "android/content/res/AssetManagerImpl";

jboolean register_android_content_res_AssetManagerImpl(JNIEnv* env)
{
    jclass stringClass = FindClassOrDie(env, "java/lang/String");
    g_stringClass = MakeGlobalRefOrDie(env, stringClass);
    jclass clazz;
    clazz = env->FindClass(kClassPathName);
    LOG_FATAL_IF(clazz == NULL, "Unable to find class android.content.res.AssetManagerImpl");

    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        ALOGE("RegisterNatives failed for '%s'", kClassPathName);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
