#define LOG_TAG "FPS-BOOST"
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include "utils/Log.h"
#include "FrameBudgetIndicator.h"
#include <linux/types.h>
#include <pthread.h>

static int (*fpNotifyQueue)(__u32) = nullptr;
static void *handle = nullptr, *func = nullptr;

typedef int (*FpNotifyQueue)(__u32);

#define LIB_FULL_NAME "libboost.so"
void fbcInit() {
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

    pthread_mutex_unlock(&mMutex);
    return;

err_fbcInit:
    fpNotifyQueue = NULL;
    dlclose(handle);
    pthread_mutex_unlock(&mMutex);
}

android::status_t notifyFbc(const int32_t& value) {
    fbcInit();

    switch (value) {
        case FBC_QUEUE_BEG:
            if (fpNotifyQueue) {
                const int32_t err = fpNotifyQueue(value == FBC_QUEUE_BEG ? 1 : 0);
                if (err != android::NO_ERROR) {
                    ALOGE("notifyQueue(%d) err:%d", value, err);
                    return android::INVALID_OPERATION;
                }
            } else {
                ALOGE("notifyQueue load error");
                return android::NO_INIT;
            }
            break;
    }
    return android::NO_ERROR;
}