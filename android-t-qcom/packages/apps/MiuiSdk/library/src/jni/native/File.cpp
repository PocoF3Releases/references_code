/*
 * Copyright (C) 2013, Xiaomi Inc. All rights reserved.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include <utils/Log.h>

#include "Native.h"

namespace miui {

static char* getCString(JNIEnv *env, jstring str){
    int length = env->GetStringUTFLength(str);
    const char* chars = env->GetStringUTFChars(str, NULL);
    char* result = new char[length + 1];
    memcpy(result, chars, length);
    result[length] = 0;
    env->ReleaseStringUTFChars(str, chars);
    return result;
}

static jboolean chmodRecursion(const char *path, int mode);

static jboolean chmodDirectory(const char *path, int mode) {
    jboolean ret;
    DIR *dir;

    dir = opendir(path);
    if (dir == NULL) {
        ALOGE("opendir %s failed: %s", path, strerror(errno));
        return JNI_FALSE;
    }

    ret = JNI_TRUE;
    int length = strlen(path);
    char *file = new char[strlen(path) + 257];
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        int fileLength = strlen(ent->d_name);
        if ((fileLength == 1 && ent->d_name[0] == '.') ||
                (fileLength == 2 && ent->d_name[0] == '.' && ent->d_name[1] == '.')) {
            continue;
        }

        memcpy(file, path, length);
        file[length] = '/';
        memcpy(file + length + 1, ent->d_name, fileLength);
        file[length + 1 + fileLength] = 0;

        if (chmodRecursion(file, mode) == JNI_FALSE) {
            ret = JNI_FALSE;
            break;
        }
    }

    delete [] file;
    closedir(dir);
    return ret;
}

static jboolean chmodRecursion(const char *path, int mode) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        ALOGE("stat %s failed: %s", path, strerror(errno));
        return JNI_FALSE;
    }

    if (chmod(path, mode) != 0) {
        ALOGE("chmod %s failed: %s", path, strerror(errno));
        return JNI_FALSE;
    }

    if (S_ISDIR(st.st_mode)) {
        return chmodDirectory(path, mode);
    } else if (S_ISREG(st.st_mode)) {
        return JNI_TRUE;
    }

    ALOGW("skip unusual file %s (mode=0%o)", path, st.st_mode);
    return JNI_TRUE;
}

static jboolean com_miui_internal_os_Native_chmod(JNIEnv *env, jclass, jstring path, jint mode, jint flag) {
    char *p = getCString(env, path);
    jboolean ret = JNI_FALSE;

    if (0 == flag) {
        ret = chmod(p, mode) == 0;
    } else {
        ret = chmodRecursion(p, mode);
    }

    delete [] p;
    return ret;
}

static jboolean chownRecursion(const char *path, int uid, int gid);

static jboolean chownDirectory(const char *path, int uid, int gid) {
    jboolean ret;
    DIR *dir;

    dir = opendir(path);
    if (dir == NULL) {
        ALOGE("opendir %s failed: %s", path, strerror(errno));
        return JNI_FALSE;
    }

    ret = JNI_TRUE;
    int length = strlen(path);
    char *file = new char[strlen(path) + 257];
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        int fileLength = strlen(ent->d_name);
        if ((fileLength == 1 && ent->d_name[0] == '.') ||
                (fileLength == 2 && ent->d_name[0] == '.' && ent->d_name[1] == '.')) {
            continue;
        }

        memcpy(file, path, length);
        file[length] = '/';
        memcpy(file + length + 1, ent->d_name, fileLength);
        file[length + 1 + fileLength] = 0;

        if (chownRecursion(file, uid, gid) == JNI_FALSE) {
            ret = JNI_FALSE;
            break;
        }
    }

    delete [] file;
    closedir(dir);
    return ret;
}

static jboolean chownRecursion(const char *path, int uid, int gid) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        ALOGE("stat %s failed: %s", path, strerror(errno));
        return JNI_FALSE;
    }

    if (chown(path, uid, gid) != 0) {
        ALOGE("chown %s failed: %s", path, strerror(errno));
        return JNI_FALSE;
    }

    if (S_ISDIR(st.st_mode)) {
        return chownDirectory(path, uid, gid);
    } else if (S_ISREG(st.st_mode)) {
        return JNI_TRUE;
    }

    ALOGW("skip unusual file %s (mode=0%o)", path, st.st_mode);
    return JNI_TRUE;
}

static jboolean com_miui_internal_os_Native_chown(JNIEnv *env, jclass, jstring path, jint uid, jint gid, jint flag) {
    char *p = getCString(env, path);
    jboolean ret = JNI_FALSE;

    if (0 == flag) {
        ret = chown(p, uid, gid) == 0;
    } else {
        ret = chownRecursion(p, uid, gid);
    }

    delete [] p;
    return ret;
}

static jint com_miui_internal_os_Native_umask(JNIEnv *UNUSED(env), jclass, jint mask) {
    return umask(mask);
}

static jboolean remove(const char *path) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        return JNI_TRUE;
    }

    if (S_ISDIR(st.st_mode)) {
        return rmdir(path) == 0;
    }
    return unlink(path) == 0;
}

static jboolean removeRecursion(const char *path);

static jboolean removeDirectory(const char *path) {
    jboolean ret;
    DIR *dir;

    dir = opendir(path);
    if (dir == NULL) {
        ALOGE("opendir %s failed: %s", path, strerror(errno));
        return JNI_FALSE;
    }

    ret = JNI_TRUE;
    int length = strlen(path);
    char *file = new char[strlen(path) + 257];
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        int fileLength = strlen(ent->d_name);
        if ((fileLength == 1 && ent->d_name[0] == '.') ||
                (fileLength == 2 && ent->d_name[0] == '.' && ent->d_name[1] == '.')) {
            continue;
        }

        memcpy(file, path, length);
        file[length] = '/';
        memcpy(file + length + 1, ent->d_name, fileLength);
        file[length + 1 + fileLength] = 0;

        if (removeRecursion(file) == JNI_FALSE) {
            ret = JNI_FALSE;
            break;
        }
    }

    delete [] file;
    closedir(dir);

    if (ret) {
        ret = rmdir(path) == 0;
    }

    return ret;
}

static jboolean removeRecursion(const char *path) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        ALOGE("stat %s failed: %s", path, strerror(errno));
        return JNI_FALSE;
    }

    if (S_ISDIR(st.st_mode)) {
        return removeDirectory(path);
    }
    return unlink(path) == 0;
}

static jboolean com_miui_internal_os_Native_rm(JNIEnv *env, jclass, jstring path, int flag) {
    char *p = getCString(env, path);
    jboolean ret = JNI_FALSE;

    if (0 == flag) {
        ret = remove(p);
    } else {
        ret = removeRecursion(p);
    }

    delete [] p;
    return ret;
}

static JNINativeMethod methods[] = {
    {"chmod", "(Ljava/lang/String;II)Z",
            (void*) com_miui_internal_os_Native_chmod},
    {"chown", "(Ljava/lang/String;III)Z",
            (void*) com_miui_internal_os_Native_chown},
    {"umask", "(I)I",
            (void*) com_miui_internal_os_Native_umask},
    {"rm", "(Ljava/lang/String;I)Z",
            (void*) com_miui_internal_os_Native_rm},
};

int RegisterFileNatives(JNIEnv *env, jclass clazz) {
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]))) {
        ALOGE("RegisterNatives fialed for '%s' for file methods", JAVA_CLASS_NAME);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

}

