#define LOG_TAG "Framectr"
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include "utils/Log.h"
#include "Framectr.h"
#include <linux/types.h>
#include <pthread.h>

static int (*fpNotifyQueue)(__u32, __u64, __s32, __u64) = nullptr;
static int (*fpNotifyDequeue)(__u32, __u64, __s32, __u64) = nullptr;
static int (*fpNotifyConnect)(__u32, __u64, __u32, __u64) = nullptr;
static void *handle = nullptr, *func = nullptr;
typedef int (*FpNotifyQueue)(__u32, __u64, __s32, __u64);
typedef int (*FpNotifyDequeue)(__u32, __u64, __s32, __u64);
typedef int (*FpNotifyConnect)(__u32, __u64, __u32, __u64);


#define LIB_FULL_NAME "libperf_ctl.so"

void FbcInit() {
    static bool inited = false;
    static pthread_mutex_t mMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mMutex);
    if (inited) {
        pthread_mutex_unlock(&mMutex);
        return;
    }

    /*
    * Consider if library or funcion is missing, re-try helps
    * nothing but lots of error logs. Thus, just init once no
    * matter if anything is well-prepared or not. However,
    * entire init flow should be lock-protected.
    */
   inited = true;

   handle = dlopen(LIB_FULL_NAME, RTLD_NOW);
   if (handle == NULL) {
       ALOGE("Can't load library: %s", dlerror());
       pthread_mutex_unlock(&mMutex);
       return;
   }

   func = dlsym(handle, "xgfNotifyQueue");
   fpNotifyQueue = reinterpret_cast<FpNotifyQueue>(func);

   if (fpNotifyQueue == NULL) {
        ALOGE("xgfNotifyQueue error: %s", dlerror());
       goto err_fbcInit;
   }

   func = dlsym(handle, "xgfNotifyDequeue");
   fpNotifyDequeue = reinterpret_cast<FpNotifyDequeue>(func);

   if (fpNotifyDequeue == NULL) {
       ALOGE("xgfNotifyDequeue error: %s", dlerror());
       goto err_fbcInit;
   }
    func = dlsym(handle, "xgfNotifyConnect");
    fpNotifyConnect = reinterpret_cast<FpNotifyConnect>(func);

    if (fpNotifyConnect == NULL) {
        ALOGE("xgfNotifyConnect error: %s", dlerror());
        goto err_fbcInit;
    }

    pthread_mutex_unlock(&mMutex);
    return;

err_fbcInit:
    fpNotifyQueue = NULL;
    fpNotifyDequeue = NULL;
    dlclose(handle);
    pthread_mutex_unlock(&mMutex);
}

android::status_t NotifyFbc(const int32_t& value, const uint64_t& bufID, const int32_t& param, const uint64_t& id) {
    FbcInit();

    switch (value) {
        case FBC_CNT:
        case FBC_DISCNT:
            if (fpNotifyConnect) {
                const int32_t err = fpNotifyConnect(value == FBC_CNT ? 1 : 0, 0, static_cast<__u32>(param), id);
                if (err != android::NO_ERROR) {
                    ALOGE("notifyConnect(%d %d) err:%d", value, param, err);
                    return android::INVALID_OPERATION;
                }
            } else {
                ALOGE("notifyConnect load error");
                return android::NO_INIT;
            }
            break;

        case FBC_QUEUE_B:
        case FBC_QUEUE_E:
            if (fpNotifyQueue) {
                const int32_t err = fpNotifyQueue(value == FBC_QUEUE_B ? 1 : 0, 0, 0, id);
                if (err != android::NO_ERROR) {
                    ALOGE("notifyQueue(%d) err:%d", value, err);
                    return android::INVALID_OPERATION;
                }
            } else {
                ALOGE("notifyQueue load error");
                return android::NO_INIT;
            }
            break;

        case FBC_DEQUEUE_B:
        case FBC_DEQUEUE_E:
            if (fpNotifyDequeue) {
                const int32_t err = fpNotifyDequeue(value == FBC_DEQUEUE_B ? 1 : 0, 0, 0, id);
                if (err != android::NO_ERROR) {
                    ALOGE("notifyDequeue(%d) err:%d", value, err);
                    return android::INVALID_OPERATION;
                }
            } else {
                ALOGE("notifyDequeue load error");
                return android::NO_INIT;
            }
            break;

    }
   return android::NO_ERROR;
}