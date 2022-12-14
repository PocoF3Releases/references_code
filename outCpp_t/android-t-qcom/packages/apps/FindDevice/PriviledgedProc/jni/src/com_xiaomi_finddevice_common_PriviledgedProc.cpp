#include "../jnidef/com_xiaomi_finddevice_common_PriviledgedProc.h"

#include "../common/log.h"
#include "../../../common/native/include/util_common.h"
#include "../../../common/native/include/JNIEnvWrapper.h"

#include "PriviledgedProcService.h"

#include <android_util_Binder.h>
#include <errno.h>

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeExists
  (JNIEnv * pRawEnv, jclass, jstring j_path, jbooleanArray j_rst) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_rst || env.GetArrayLength(j_rst) < 1) {
        LOGE("nativeExists: !j_rst || env.GetArrayLength(j_rst) < 1");
        errno = EINVAL;
        return JNI_FALSE;
    }

    jboolean rstBuf = JNI_FALSE;
    env.SetBooleanArrayRegion(j_rst, 0, 1, &rstBuf);

    if (!j_path) {
        LOGE("nativeExists: !j_path");
        errno = EINVAL;
        return JNI_FALSE;
    }

    std::vector<char> vPath;
    env.getStringUTF(j_path, &vPath);
    const char * path = addr(vPath);

    bool ex = false;
    bool success = getPriviledgedProcService()->exists(path, &ex);

    if (!success) {
        return JNI_FALSE;
    }

    rstBuf = ex ? JNI_TRUE : JNI_FALSE;
    env.SetBooleanArrayRegion(j_rst, 0, 1, &rstBuf);

    return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeRemove
  (JNIEnv * pRawEnv, jclass, jstring j_path) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_path) {
        LOGE("nativeRemove: !j_path");
        errno = EINVAL;
        return JNI_FALSE;
    }

    std::vector<char> vPath;
    env.getStringUTF(j_path, &vPath);
    const char * path = addr(vPath);

    bool success = getPriviledgedProcService()->remove(path);

    return success ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jobjectArray JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeLsDir
  (JNIEnv * pRawEnv, jclass, jstring j_path) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_path) {
        LOGE("nativeLsDir: !j_path");
        errno = EINVAL;
        return NULL;
    }

    std::vector<char> vPath;
    env.getStringUTF(j_path, &vPath);
    const char * path = addr(vPath);

    std::vector<std::vector<char> > list;
    bool success = getPriviledgedProcService()->lsDir(path, &list);

    if (!success) {
        return NULL;
    }

    jclass strClass = env.FindClass("java/lang/String");
    EXCEPTION_RETURN(NULL);
    jobjectArray j_list = env.NewObjectArray(list.size(), strClass, NULL);
    EXCEPTION_RETURN(NULL);

    std::vector<std::vector<char> >::size_type listSize = list.size();
    for (std::vector<std::vector<char> >::size_type i = 0; i < listSize; i++) {
        jstring j_str = env.NewStringUTF(addr(list[i]));
        EXCEPTION_RETURN(NULL);
        env.SetObjectArrayElement(j_list, i, j_str);
    }

    return j_list;
}


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeIsFile
  (JNIEnv * pRawEnv, jclass, jstring j_path, jbooleanArray j_rst) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_rst || env.GetArrayLength(j_rst) < 1) {
        LOGE("nativeIsFile: !j_rst || env.GetArrayLength(j_rst) < 1");
        errno = EINVAL;
        return JNI_FALSE;
    }

    if (!j_path) {
        LOGE("nativeIsFile: !j_path");
        errno = EINVAL;
        return JNI_FALSE;
    }

    jboolean rstBuf = false;
    env.SetBooleanArrayRegion(j_rst, 0, 1, &rstBuf);

    std::vector<char> vPath;
    env.getStringUTF(j_path, &vPath);
    const char * path = addr(vPath);

    bool isFile = false;
    bool success = getPriviledgedProcService()->isFile(path, &isFile);

    if (!success) {
        return JNI_FALSE;
    }

    rstBuf = isFile ? JNI_TRUE : JNI_FALSE;
    env.SetBooleanArrayRegion(j_rst, 0, 1, &rstBuf);

    return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeIsDir
  (JNIEnv * pRawEnv, jclass, jstring j_path, jbooleanArray j_rst) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_rst || env.GetArrayLength(j_rst) < 1) {
        LOGE("nativeIsDir: !j_rst || env.GetArrayLength(j_rst) < 1");
        errno = EINVAL;
        return JNI_FALSE;
    }

    if (!j_path) {
        LOGE("nativeIsDir: !j_path");
        errno = EINVAL;
        return JNI_FALSE;
    }

    jboolean rstBuf = false;
    env.SetBooleanArrayRegion(j_rst, 0, 1, &rstBuf);

    std::vector<char> vPath;
    env.getStringUTF(j_path, &vPath);
    const char * path = addr(vPath);

    bool isDir = false;
    bool success = getPriviledgedProcService()->isDir(path, &isDir);

    if (!success) {
        return JNI_FALSE;
    }

    rstBuf = isDir ? JNI_TRUE : JNI_FALSE;
    env.SetBooleanArrayRegion(j_rst, 0, 1, &rstBuf);

    return JNI_TRUE;
}


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeMove
  (JNIEnv * pRawEnv, jclass, jstring j_oldPath, jstring j_newPath) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_oldPath) {
        LOGE("nativeMove: !j_oldPath");
        errno = EINVAL;
        return JNI_FALSE;
    }

    if (!j_newPath) {
        LOGE("nativeMove: !j_newPath");
        errno = EINVAL;
        return JNI_FALSE;
    }

    std::vector<char> vOldPath, vNewPath;
    env.getStringUTF(j_oldPath, &vOldPath);
    env.getStringUTF(j_newPath, &vNewPath);
    const char * oldPath = addr(vOldPath);
    const char * newPath = addr(vNewPath);

    bool success = getPriviledgedProcService()->move(oldPath, newPath);

    return success ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeMkdir
  (JNIEnv * pRawEnv, jclass, jstring j_path) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_path) {
        LOGE("nativeMkdir: !j_path");
        errno = EINVAL;
        return JNI_FALSE;
    }

    std::vector<char> vPath;
    env.getStringUTF(j_path, &vPath);
    const char * path = addr(vPath);

    bool success = getPriviledgedProcService()->mkdir(path);

    return success ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeWriteTinyFile
  (JNIEnv * pRawEnv, jclass, jstring j_path, jboolean j_append, jbyteArray j_data) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_path) {
        LOGE("nativeWriteTinyFile: !j_path");
        errno = EINVAL;
        return JNI_FALSE;
    }

    if (!j_data) {
        LOGE("nativeWriteTinyFile: !j_data");
        errno = EINVAL;
        return JNI_FALSE;
    }

    std::vector<char> vPath;
    env.getStringUTF(j_path, &vPath);
    const char * path = addr(vPath);
    std::vector<jbyte> vData;
    vData.resize(env.GetArrayLength(j_data));
    env.GetByteArrayRegion(j_data, 0, vData.size(), addr(vData));
    jbyte * data = addr(vData);

    bool success = getPriviledgedProcService()->writeTinyFile(path, j_append ? true : false,
        (const unsigned char *)data, env.GetArrayLength(j_data));

    return success ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jbyteArray JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeReadTinyFile
  (JNIEnv * pRawEnv, jclass, jstring j_path) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_path) {
        LOGE("nativeReadTinyFile: !j_path");
        errno = EINVAL;
        return JNI_FALSE;
    }

    std::vector<char> vPath;
    env.getStringUTF(j_path, &vPath);
    const char * path = addr(vPath);

    std::vector<unsigned char>  bytes;
    bool success = getPriviledgedProcService()->readTinyFile(path, &bytes);

    if (!success) {
        return NULL;
    }

    jsize j_dataLen = bytes.size();
    jbyteArray j_data = env.NewByteArray(j_dataLen);
    EXCEPTION_RETURN(JNI_FALSE);
    env.SetByteArrayRegion(j_data, 0, j_dataLen, (const jbyte *)addr(bytes));

    return j_data;
}

JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeAddService
  (JNIEnv * pRawEnv, jclass, jstring j_name, jobject j_service) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_name) {
        LOGE("nativeAddService: !j_name");
        errno = EINVAL;
        return JNI_FALSE;
    }

    if (!j_service) {
        LOGE("nativeAddService: !j_service");
        errno = EINVAL;
        return JNI_FALSE;
    }

    android::sp<android::IBinder> service = android::ibinderForJavaObject(env, j_service);
    if (service == NULL) {
        LOGE("nativeAddService: !service");
        errno = EINVAL;
        return JNI_FALSE;
    }

    std::vector<char> vName;
    env.getStringUTF(j_name, &vName);
    const char * name = addr(vName);

    bool success = getPriviledgedProcService()->addService(name, service);

    return success ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jboolean JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_nativeGetSetRebootClearVariable
  (JNIEnv * pRawEnv, jclass, jstring j_name, jobjectArray j_oldValue, jboolean j_modify, jstring j_newValue) {

    JNIEnvWrapper env(pRawEnv);

    if (!j_name) {
        LOGE("nativeGetSetRebootClearVariable: !j_name");
        errno = EINVAL;
        return JNI_FALSE;
    }

    if (!j_oldValue || env.GetArrayLength(j_oldValue) < 1) {
        LOGE("nativeGetSetRebootClearVariable: !j_oldValue || env.GetArrayLength(j_oldValue) < 1");
        errno = EINVAL;
        return JNI_FALSE;
    }

    std::vector<char> vName;
    env.getStringUTF(j_name, &vName);
    const char * name = addr(vName);

    std::vector<char> vNewValue;
    const char * newValue = NULL;
    if (j_newValue) {
        env.getStringUTF(j_newValue, &vNewValue);
        newValue = addr(vNewValue);
    }

    std::vector<char> oldValue;
    bool success = getPriviledgedProcService()->getSetRebootClearVariable(name,
                                                                          &oldValue,
                                                                          j_modify ? true : false,
                                                                          newValue);

    if (success) {
        if (!oldValue.size()) {
            env.SetObjectArrayElement(j_oldValue, 0, NULL);
        } else {
            jstring j_str = env.NewStringUTF(addr(oldValue));
            EXCEPTION_RETURN(JNI_FALSE);
            env.SetObjectArrayElement(j_oldValue, 0, j_str);
        }
    }


    return success ? JNI_TRUE : JNI_FALSE;
}


JNIEXPORT jint JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_getLastErrorCode
  (JNIEnv *, jclass) {
    return errno;
}


JNIEXPORT jstring JNICALL Java_com_xiaomi_finddevice_common_PriviledgedProc_getErrorMessage
  (JNIEnv * pRawEnv, jclass, jint errnoCode) {

    JNIEnvWrapper env(pRawEnv);

    // Be portably thread-safe.

    const int ERROR_MESSAGE_BUFFER_LEN = 1024;

    char errorMsgBuf[ERROR_MESSAGE_BUFFER_LEN] = {};
    ::strerror_r(errnoCode,
               errorMsgBuf,
               ERROR_MESSAGE_BUFFER_LEN - 1 /* in case the implementation forgets to put a NULL. */);

    return env.NewStringUTF(errorMsgBuf);

}