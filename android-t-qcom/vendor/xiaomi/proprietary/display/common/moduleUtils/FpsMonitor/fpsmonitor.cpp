#include "fpsmonitor.h"
#include <mi_disp.h>
#include "mi_stc_service.h"
#include "parseXml.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "FPSMonitor"
#endif
using snapdragoncolor::IMiStcService;
namespace android {
ANDROID_SINGLETON_STATIC_INSTANCE(FpsMonitor);
int DFLOG::loglevel = 1;
FpsMonitor::NOTIFY_LOCK_FPS FpsMonitor::mCallback = NULL;
FpsMonitor::FpsMonitor() {
    registerPollEvent();
    mCheckCond = PTHREAD_COND_INITIALIZER;
    mChecklock = PTHREAD_MUTEX_INITIALIZER;
    pthread_create(&mThreadFpsMonitor, NULL, ThreadWrapperFpsMonitor, this);
    pthread_create(&mThreadCheckUpdateStatus, NULL, ThreadWrapperCheckUpdateStatus, this);
    string key("lowFpsSupport");
    int ret = 0;
    char p[4096];
    sp<MiParseXml> mParseXml = new MiParseXml();
    for (int i = 0; i < DISPLAY_MAX; i++) {
        if (i == DISPLAY_SECONDARY && access(DISP1_DEV_PATH, F_OK)) {
            return;
        }
        if (mParseXml.get()) {
            ret = mParseXml->parseXml(i, key, p, sizeof(p));
            if (ret != 0) {
                DF_LOGE("parse disp[%d] lowFpsSupport failed", i);
                mLowFpsSupport[i] = 0;
            } else {
                mLowFpsSupport[i] = atoi(p);
            }
        }
    }
}

int FpsMonitor::registerPollEvent()
{
    int ret = 0;
    struct disp_event_req event_req;
    int df_event_index = 0;

    mDFPollFd.fd = open(kDispFeaturePath, O_RDONLY);
    if (mDFPollFd.fd  < 0) {
        DF_LOGE("open %s failed", kDispFeaturePath);
        return -1;
    } else {
        DF_LOGD("open %s success", kDispFeaturePath);
    }

    mDFPollFd.events = POLLIN | POLLRDNORM | POLLERR;

    for (int i = 0; i < DISPLAY_MAX; i++) {
        if (i == DISPLAY_SECONDARY && access(DISP1_DEV_PATH, F_OK))
            break;

        event_req.base.flag = 0;
        event_req.base.disp_id = i;
        event_req.type = MI_DISP_EVENT_FPS;

        ret = ioctl(mDFPollFd.fd, MI_DISP_IOCTL_REGISTER_EVENT, &event_req);
        if(ret) {
            DF_LOGE("Disp id %d, ioctl MI_DISP_IOCTL_REGISTER_EVENT fail\n", i);
            continue;
        }
    }
    return 0;
}

void *FpsMonitor::ThreadWrapperCheckUpdateStatus(void *context) {
    if (context) {
        return (void *)(uintptr_t)static_cast<FpsMonitor *>(context)->threadFuncCheckUpdateStatus();
    }
    return NULL;
}

int FpsMonitor::threadFuncCheckUpdateStatus() {
    while (1) {
        pthread_mutex_lock(&mChecklock);
        pthread_cond_wait(&mCheckCond, &mChecklock);
        pthread_mutex_unlock(&mChecklock);
        while(1) {
            if (mCurStatus) {
                usleep(1*1000*1000); // check every 1s
                if (getUpdateStatus()) {
                    continue;
                } else {
                    checkFps(0); // Stc does not update now, can close lock fps.
                    break;
                }
            } else {
                break;
            }
        }
    }
}

void *FpsMonitor::ThreadWrapperFpsMonitor(void *context) {
    if (context) {
        return (void *)(uintptr_t)static_cast<FpsMonitor *>(context)->threadFuncFpsMonitor();
    }
    return NULL;
}

int FpsMonitor::threadFuncFpsMonitor() {
    char event_data[1024] = {0};
    int ret = 0;
    struct disp_event_resp *event_resp = NULL;
    int size;
    int i;


    while(1) {
        ret = poll(&mDFPollFd, 1, -1);
        if (ret <= 0) {
            DF_LOGE("poll failed. error = %s", strerror(errno));
            continue;
        }

        if (mDFPollFd.revents & (POLLIN | POLLRDNORM | POLLERR)) {
            memset(event_data, 0x0, sizeof(event_data));
            size = read(mDFPollFd.fd, event_data, sizeof(event_data));
            if (size < 0) {
                DF_LOGE("read disp feature event failed\n");
                continue;
            }
            if (size < sizeof(struct disp_event_resp)) {
                DF_LOGE("Invalid event size %zd, expect %zd\n", size, sizeof(struct disp_event_resp));
                continue;
            }

            i = 0;
            while (i < size) {
                event_resp = (struct disp_event_resp *)&event_data[i];
                switch (event_resp->base.type) {
                case MI_DISP_EVENT_FPS: {
                    if (event_resp->base.length - sizeof(struct disp_event_resp) < 2) {
                        DF_LOGE("Invalid Backlight value\n");
                        break;
                    }
                    mFps[event_resp->base.disp_id] = (unsigned long)*((int *)(event_resp->data));
                    DF_LOGD("display %d, fps %d", event_resp->base.disp_id, mFps[event_resp->base.disp_id]);
                } break;
                default:
                    break;
                }
                i += event_resp->base.length;
            }
        }
    }
    pthread_exit(0);
    return 0;
}

void FpsMonitor::registerCallback(void *callback)
{
    if (callback) {
        if (!mCallback)
            mCallback = (NOTIFY_LOCK_FPS)callback;
    }
}

void FpsMonitor::checkFps(int enable)
{
    DF_LOGV("checkFps enable %d", enable);
    int i = 0;
    for (i = 0; i < DISPLAY_MAX; i++) {
        if (mDispState[i] == kStateOn)
            break;
    }
    if (enable) {
        if (i == DISPLAY_MAX) {
            DF_LOGW("No active display");
            return;
        }
        if (mLowFpsSupport[i] == 0) {
            // Not ltpo panel, no need to lock fps in high level.
            return;
        }
    }
    if (mCallback) {
        if (enable != mCurStatus) {
            mCallback(enable);
            mCurStatus = enable;
            if (enable) {
                pthread_cond_signal(&mCheckCond);
            }
        }
    }
}

void FpsMonitor::notifyBrightnessUpdate()
{
    gettimeofday(&mBlUpdateTime, NULL);
}

bool FpsMonitor::checkBrightnessUpdateInterval()
{
    struct timeval curTime = {0};
    gettimeofday(&curTime, NULL);
    if (curTime.tv_sec == mBlUpdateTime.tv_sec) {
        return false;
    } else if (curTime.tv_sec - mBlUpdateTime.tv_sec >= 2) {
        return true;
    } else {
        if ((1000000 + curTime.tv_usec - mBlUpdateTime.tv_usec) > 1000000)
            return true;
        else
            return false;
    }
}

bool FpsMonitor::getUpdateStatus()
{
    int ret = 0;
    sp<IMiStcService> imistcservice;
    android::Parcel out;
    bool updateStatus = false;

    if (!checkBrightnessUpdateInterval()) {
        return true;
    }

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::GET_UPDATE_STATUS, NULL, &out);
        if (!ret) {
            updateStatus = out.readBool();
        }
    } else {
        DF_LOGW("Failed to acquire %s", "display.mistcservice");
    }
    return updateStatus;
}

void FpsMonitor::SetDisplayState(int displayId, int state)
{
	DF_LOGV("%d state: %d", displayId, state);
	mDispState[displayId] = state;
}
} //namespace android
