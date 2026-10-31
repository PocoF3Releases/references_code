#include <stdlib.h>
#include <utils/Log.h>
#include <cutils/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <string.h>
#include "xlog_utils.h"

#define XLOG_LIB32 "system/lib/libxlog.so"
#define XLOG_LIB "system/lib64/libxlog.so"

typedef int (*xlog_send_java_t) (char* msg);

static pthread_mutex_t xlog_init_lock = PTHREAD_MUTEX_INITIALIZER;

struct xlog_handle {
    void *lib_handle;
    xlog_send_java_t xlog_send_java;
};

struct xlog_handle *g_xlog_handle = NULL;

int _xog_get_handle() {
    ALOGD(" %s ", __func__);
    g_xlog_handle->lib_handle = dlopen(XLOG_LIB, RTLD_NOW);
    if (g_xlog_handle->lib_handle == NULL) {
            ALOGE( "%s DLOPEN failed for %s", __func__, XLOG_LIB);
            return -1;
    } else {
        g_xlog_handle->xlog_send_java = (xlog_send_java_t)dlsym(g_xlog_handle->lib_handle,
                                                    "xlog_send_java");
        if (!g_xlog_handle->xlog_send_java) {
            ALOGE("%s: Could not find the symbol xlog_send_java from %s",
                  __func__, XLOG_LIB);
        return -1;
            }
    }
    ALOGD("get xlog handle");
    return 0;
}

int xlog_init() {
    int ret;
    ALOGD("xlog_init");
    pthread_mutex_lock(&xlog_init_lock);
    if (g_xlog_handle  != NULL) {
        ALOGD("xlog already init");
         pthread_mutex_unlock(&xlog_init_lock);
        return 0;
     }
    g_xlog_handle = (xlog_handle*) calloc(1, sizeof(struct xlog_handle));
    if (!g_xlog_handle) {
        ALOGD("MALLOC FAIL");
         pthread_mutex_unlock(&xlog_init_lock);
        return -1;
    }
    ret =  _xog_get_handle();
    if(ret <0){
        free(g_xlog_handle);
        g_xlog_handle = NULL;
    }
    pthread_mutex_unlock(&xlog_init_lock);
    return ret;
}

void xlog_deinit() {
    ALOGD(" %s ", __func__);
    pthread_mutex_lock(&xlog_init_lock);
    if(g_xlog_handle)
        free(g_xlog_handle);
    g_xlog_handle = NULL;
    pthread_mutex_unlock(&xlog_init_lock);
}


int xlog_send_java(char* msg) {
    ALOGV(" %s ", __func__);
    int ret = -1;
    pthread_mutex_lock(&xlog_init_lock);
    if(g_xlog_handle != NULL)
        ret = g_xlog_handle->xlog_send_java(msg);
    pthread_mutex_unlock(&xlog_init_lock);
    return ret;
}

