/*
 * Copyright (c) 2019 Xiaomi, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi, Inc.
 */

/**
 * Introduction:
 * This 3.0 HIDL aimed to fix belows issues under Treble arch.
 * 1. Large files read/write using share memory.
 *    NO file size limitation by technology. But we limit the MAX size <= 128MB by design.
 *    share memory based on hidl_memory/IAllocator/MemoryBlock designed by google.
 *
 *    BTW: Large files read/write via share memory has JNI interface which can called by Java
 *    layer. See more for libmisys_jni.xiaomi.so.
 *
 * 2. Large memory read/write using share memory.
 * 3. Callback function.
 * 4. Set property function through HIDL. Selinux rules now forbid setprop under Treble.
 *    get propperty do not needed via HIDL.
 */

#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <utils/Log.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include<iostream>

#include "CheckPid.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "libcheckpid"

using namespace android;
using namespace::std;

using std::string;
using ::android::sp;

static const string slash = "/";
static const string logPath = "/mnt/vendor/persist/misys/";
static const string logPrefix = "log_";

static const int MaxArraySize = 20;

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;

using IMiSys_V1_0 = ::vendor::xiaomi::hardware::misys::V1_0::IMiSys;
using ::vendor::xiaomi::hardware::misys::V1_0::IResultValue;
using ::vendor::xiaomi::hardware::misys::V1_0::IFileListResult;
using ::vendor::xiaomi::hardware::misys::V1_0::IReadResult;

static sp<IMiSys_V1_0> service_V10 = nullptr;

static sp<IMiSys_V1_0> getMiSysService_V10() {
    if (service_V10 == nullptr) {
      service_V10 = IMiSys_V1_0::getService();
      if (service_V10 != nullptr) {
        ALOGI("Initialize the misys@1.0 successful");
      }
    }
    return service_V10;
}

// internal function to check if the process with the pid exists or not
int checkPidExists(int pid) {
    int rc = -1;
    bool find = false;
    int i = 0;
    std::vector<const char*> argsList;
    ifstream fileStream;

    string line;

    string logFile;
    string pidFile;
    string pidStr;

    IResultValue eraseResult = IResultValue::MISYS_EINVAL;

    // sanity check for parameter
    if (pid < 0) {
      ALOGE("pid(%d) is invalid, return error\n", pid);
      return -1;
    }

    //get the misys@1.0 HIDL
    if (getMiSysService_V10() == nullptr) {
      ALOGE("Unable to initialize the HIDL");
      return -1;
    }

    pidStr = std::to_string(pid);
    pidFile.append(logPrefix);
    pidFile.append(pidStr);

    logFile = logPath;
    logFile.append(pidFile);

    argsList.push_back("/vendor/bin/ps");
    argsList.push_back("-A");
    argsList.push_back("-o");
    argsList.push_back("-pid");

    const char* pidArgs[MaxArraySize];

    // sanity check for the argsList size, if more than MAX of pidArgs, return failed.
    if (argsList.size() >= MaxArraySize) {
      ALOGE("argsList size() to large, return failed\n");
      return -1;
    }

    for (auto it = argsList.begin(); it != argsList.end(); it++, i++) {
      pidArgs[i] = *it;
      ALOGV("i = %d, argsList[i]=%s\n", i, argsList[i]);
    }

    rc = logwrap_fork_execvp(argsList.size(), const_cast<char**>(pidArgs), NULL, true,
            LOG_KLOG | LOG_FILE, true, const_cast<char *>(logFile.c_str()));

    if (rc) {
      ALOGE("pid=%d android_fork_execvp_ext() failed, rc=%d\n", pid, rc);
      return rc;
    }

    // begin to parse the exec result to get the aimed pid value
    fileStream.open(logFile, ios::in);

    if (fileStream.is_open()) {
      while(getline(fileStream, line)) {
        if (line.length() > 1) {
          string subStr = line.substr(1, line.length());
          if (subStr.compare(pidStr) == 0) {
            ALOGE(" pid(%d), str=%s, subStr=%s", pid, line.c_str(), subStr.c_str());
            find = true;
            break;
          }
        }
      }

     fileStream.close();
    } else {
      ALOGE("open file=%s failed\n", logFile.c_str());
      fileStream.close();
      return rc;
    }

    // remove the log file finally.
    eraseResult = service_V10->EraseFileOrDirectory(logPath, pidFile);

    if (eraseResult != IResultValue::MISYS_SUCCESS) {
      ALOGE("EraseFileOrDirectory result = %d\n",  eraseResult);
      rc = -1;
    }

    // we got the aimed pid from the system, them we can call callback function
    if (find == true) {
      rc = 1;
    } else {
      rc = -1;
      ALOGE("cannot find the pid(%d) related process\n", pid);
      return rc;
    }

    return rc;
}
