#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "PerfSuperviser.h"
#include "JavaVMUtils.h"
#include "ThreadUtils.h"

namespace android {
namespace os {
namespace statistics {

static const char* PATH_THREAD_NAME = "/proc/self/task/%d/comm";

static pthread_once_t initflag = PTHREAD_ONCE_INIT;
static pthread_key_t pthread_key_threadinfo;
static volatile bool key_created = false;

static void getThreadName(pid_t tid, std::shared_ptr<std::string>& threadName) {
  char path[40];
  char* threadNamePtr = NULL;
  char threadNameBuf[40];
  FILE* fp;

  snprintf(path, sizeof(path), PATH_THREAD_NAME, tid);
  if ((fp = fopen(path, "r"))) {
    threadNamePtr = fgets(threadNameBuf, sizeof(threadNameBuf), fp);
    fclose(fp);
  }
  if (threadNamePtr == NULL) {
    threadName.reset();
  } else {
    // Strip ending newline
    strtok(threadNamePtr, "\n");
    threadName = std::make_shared<std::string>(threadNamePtr);
  }
}

struct ThreadInfo {
  bool isPerfSupervisionOn;
  int32_t thread_id;
  std::shared_ptr<std::string> thread_name;
};

static void destroy_threadinfo(void *data)
{
  if (data != nullptr) {
    struct ThreadInfo* pThreadInfo = (struct ThreadInfo*)data;
    pThreadInfo->thread_name.reset();
    delete pThreadInfo;
  }
}

static void create_key_threadinfo()
{
  int rc = pthread_key_create(&pthread_key_threadinfo, destroy_threadinfo);
  if (rc != 0) {
    key_created = false;
  } else {
    key_created = true;
  }
}

static struct ThreadInfo* get_threadinfo() {
  if (!key_created) {
    pthread_once(&initflag, create_key_threadinfo);
  }
  struct ThreadInfo* pThreadInfo =
    static_cast<struct ThreadInfo*>(pthread_getspecific(pthread_key_threadinfo));
  if (pThreadInfo == nullptr) {
    pThreadInfo = new struct ThreadInfo();
    pThreadInfo->isPerfSupervisionOn = true;
    pThreadInfo->thread_id = gettid();
    getThreadName(pThreadInfo->thread_id, pThreadInfo->thread_name);
    pthread_setspecific(pthread_key_threadinfo, (void *)pThreadInfo);
  }
  return pThreadInfo;
}

void ThreadUtils::setThreadPerfSupervisionOn(bool isOn) {
  JNIEnv* env = PerfSuperviser::getJNIEnv();
  setThreadPerfSupervisionOn(env, isOn);
}

void ThreadUtils::setThreadPerfSupervisionOn(JNIEnv* env, bool isOn) {
  if (env != nullptr) {
    JavaVMUtils::setThreadPerfSupervisionOn(env, isOn);
  } else {
    struct ThreadInfo* pThreadInfo = get_threadinfo();
    pThreadInfo->isPerfSupervisionOn = isOn;
  }
}

bool ThreadUtils::isThreadPerfSupervisionOn() {
  JNIEnv* env = PerfSuperviser::getJNIEnv();
  return isThreadPerfSupervisionOn(env);
}

bool ThreadUtils::isThreadPerfSupervisionOn(JNIEnv* env) {
  if (env != nullptr) {
    return JavaVMUtils::isThreadPerfSupervisionOn(env);
  } else {
    struct ThreadInfo* pThreadInfo = get_threadinfo();
    return pThreadInfo->isPerfSupervisionOn;
  }
}

void ThreadUtils::getThreadInfo(int32_t& thread_id, std::shared_ptr<std::string>& thread_name) {
  JNIEnv* env = PerfSuperviser::getJNIEnv();
  getThreadInfo(env, thread_id, thread_name);
}

void ThreadUtils::getThreadInfo(JNIEnv* env, int32_t& thread_id, std::shared_ptr<std::string>& thread_name) {
  if (env != nullptr) {
    JavaVMUtils::getThreadInfo(env, thread_id, thread_name);
  } else {
    struct ThreadInfo* pThreadInfo = get_threadinfo();
    thread_id = pThreadInfo->thread_id;
    thread_name = pThreadInfo->thread_name;
  }
}

} //namespace statistics
} //namespace os
} //namespace android
