#include "hdrhbm.h"
#include <mi_disp.h>

HDRHBMImpl* HDRHBMImpl::mHDRHBMImpl_ = new HDRHBMImpl;

HDRHBMImpl::HDRHBMImpl()
{
    registerPollEvent();
    pthread_create(&mThreadBacklight, NULL, ThreadWrapperBacklight, this);
}

HDRHBMImpl::~HDRHBMImpl()
{
    pthread_join(mThreadBacklight, NULL);
    mThreadBacklight = 0;
    close(mDFPollFd.fd);
    mDFPollFd.fd = -1;
}

HDRHBMImpl* HDRHBMImpl::CreateHDRHBMImpl() {
    if (mHDRHBMImpl_ == NULL)
        mHDRHBMImpl_ = new HDRHBMImpl;
    return mHDRHBMImpl_;
}

void HDRHBMImpl::DestroyHDRHBMImpl() {
    if (mHDRHBMImpl_) {
        delete mHDRHBMImpl_;
        mHDRHBMImpl_ = NULL;
    }
}

bool HDRHBMImpl::IsHBMMode() {
    return mIsHBM;
}

float HDRHBMImpl::GetNitsforHBM() {
    return mCurNits;
}

int HDRHBMImpl::registerPollEvent()
{
    int ret = 0;
    struct disp_event_req event_req;
    int df_event_index = 0;

    mDFPollFd.fd = open(kDispFeaturePath, O_RDONLY);
    if (mDFPollFd.fd  < 0) {
        ALOGE("open %s failed", kDispFeaturePath);
        return -1;
    } else {
        ALOGD("open %s success", kDispFeaturePath);
    }

    mDFPollFd.events = POLLIN | POLLRDNORM | POLLERR;

    event_req.base.flag = 0;
    event_req.base.disp_id = MI_DISP_PRIMARY;
    event_req.type = MI_DISP_EVENT_BRIGHTNESS_CLONE;
    ALOGV("%s, event type name: %s\n", __func__, getDispEventTypeName(event_req.type));
    ret = ioctl(mDFPollFd.fd, MI_DISP_IOCTL_REGISTER_EVENT, &event_req);
    if(ret) {
        ALOGE("%s, ioctl MI_DISP_IOCTL_REGISTER_EVENT fail\n", __func__);
        return -1;
    }
    return 0;
}

void *HDRHBMImpl::ThreadWrapperBacklight(void *context) {
    if (context) {
        return (void *)(uintptr_t)static_cast<HDRHBMImpl *>(context)->threadFuncBacklight();
    }
    return NULL;
}

int HDRHBMImpl::threadFuncBacklight() {
    char event_data[1024] = {0};
    int ret = 0;
    struct disp_event_resp *event_resp = NULL;
    int size;
    int i;
    int brightness = 0;

    while(true) {
        ret = poll(&mDFPollFd, 1, -1);
        if (ret <= 0) {
            ALOGE("poll failed. error = %s", strerror(errno));
            continue;
        }

        if (mDFPollFd.revents & (POLLIN | POLLRDNORM | POLLERR)) {
            memset(event_data, 0x0, sizeof(event_data));
            size = read(mDFPollFd.fd, event_data, sizeof(event_data));
            if (size < 0) {
                ALOGE("read disp feature event failed\n");
                continue;
            }
            if (size < sizeof(struct disp_event_resp)) {
                ALOGE("Invalid event size %zd, expect %zd\n", size, sizeof(struct disp_event_resp));
                continue;
            }

            i = 0;
            while (i < size) {
                event_resp = (struct disp_event_resp *)&event_data[i];
                switch (event_resp->base.type) {
                case MI_DISP_EVENT_BRIGHTNESS_CLONE: {
                    if (event_resp->base.length - sizeof(struct disp_event_resp) < 2) {
                        ALOGE("Invalid Backlight value\n");
                        break;
                    }
                    brightness = (unsigned long)*((int *)(event_resp->data));
                    if (brightness > 8191) {
                        mIsHBM = true;
                        mCurNits = 500.0f + 500.0f * (brightness - 8192) / 8191;
                    } else {
                        mIsHBM = false;
                    }
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

PanelHBMModeFactoryIntf* GetPanelHBMModeFactoryIntf()
{
    static PanelHBMModeFactoryImp factory_imp;
    return &factory_imp;
}

PanelHBMModeInterface* PanelHBMModeFactoryImp::CreatePanelHBMModeIntf(const char* panel_name) {
    return HDRHBMImpl::CreateHDRHBMImpl();
}

void PanelHBMModeFactoryImp::DestroyPanelHBMModeIntf(PanelHBMModeInterface* intf) {
    return HDRHBMImpl::DestroyHDRHBMImpl();
}
