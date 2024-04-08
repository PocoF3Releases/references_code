#include <stdio.h>
#include "utils.h"

#define LOG_TAG "NetworkBoost-utils"
#include <cutils/log.h>

extern JavaVM *gJavaVM;

Get_JNIEnv_Status checkAndAttachThread(JNIEnv ** env) {
    if (gJavaVM == NULL) {
    ALOGE("checkAndAttachThread global JavaVM is NULL");
    return GET_FAIL;
    }
    jint status = gJavaVM->GetEnv((void **) env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        if (gJavaVM->AttachCurrentThread(env, NULL) != JNI_OK) {
            ALOGE("Failed to attach thread to JVM!!!");
            return GET_FAIL;
        }
        return GET_SUCCESS_ATTACH;
    } else if (status == JNI_OK) {
        //The thread is already attached, JVM will take care of GC'ng it
        return GET_SUCCESS_NOATTACH;
    } else if (status == JNI_EVERSION) {
        ALOGE("GetEnv: version JNI_VERSION_1_6 not supported");
    }
    ALOGE("GetEnv: failed:status = %d", status);
    return GET_FAIL;
}

void doDetachThread() {
    if (gJavaVM == NULL) {
        ALOGE("doDetachThread global JavaVM is NULL");
        return;
    }
    gJavaVM->DetachCurrentThread();
}

AttachToJavaThread::AttachToJavaThread() {
    env_ = NULL;
    if (gJavaVM == NULL) {
        ALOGE("construcor global JavaVM is NULL");
        return;
    }
    
    if (checkAndAttachThread(&env_) == GET_SUCCESS_ATTACH) {
        selfAttached = true;
    }
}

AttachToJavaThread::~AttachToJavaThread() {
    if (gJavaVM == NULL) {
    ALOGE("destructor global JavaVM is NULL");
    return;
    }
    if (selfAttached) {
        gJavaVM->DetachCurrentThread();
        selfAttached = false;
    }
}

int jstring2string(JNIEnv* env, jstring origin, std::string &out) {
    const char *str = NULL;
    str = env->GetStringUTFChars(origin, NULL);
    if (str) {
        out = std::string(str);
        env->ReleaseStringUTFChars(origin,str);
        return 0;
    }
    else {
        out = "";
        return -1;
    }
}

int stringSplit(const std::string& str, const char split, std::vector<std::string>& res)
{
    if (str == "")      return -1;
    std::string strs = str + split;
    size_t pos = strs.find(split);
    std::string null_string = "";
    while (pos != strs.npos)
    {
        std::string temp = strs.substr(0, pos);
        if (null_string.compare(temp) != 0) {
            res.push_back(temp);
        }
        strs = strs.substr(pos + 1, strs.size());
        pos = strs.find(split);
    }
    return 0;
}