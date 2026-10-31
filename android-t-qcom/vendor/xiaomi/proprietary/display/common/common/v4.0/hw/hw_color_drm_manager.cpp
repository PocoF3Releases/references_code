
#define LOG_TAG "ColorManager"

#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <binder/Parcel.h>

#include "cust_define.h"
#include "hw_color_drm_manager.h"
#include "../display_property.h"
#ifdef HAS_QDCM
#include "mi_stc_service.h"
#include <drm/msm_drm.h>
#include <display/drm/msm_drm_pp.h>
#include <display/drm/sde_drm.h>
#include <QServiceUtils.h>
using snapdragoncolor::IMiStcService;
#endif
namespace android {

ANDROID_SINGLETON_STATIC_INSTANCE(HWColorDrmMgr);

static uint32_t pcc_cfg_template[36] = { 0x0,0x0,0x0,0x8000,0x0,0x0,0x0,0x0,
                                 0x0,0x0,0x0,0x0,0x8000,0x0,0x0,0x0,
                                 0x0,0x0,0x0,0x0,0x0,0x8000,0x0,0x0,
                                 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
                                 0x0,0x0,0x0,0x0};



HWColorDrmMgr::HWColorDrmMgr()
{
    mDevFd = -1;
    mRes = NULL;
    mInited = false;
    memset(&mCrtcProp, 0, sizeof(mCrtcProp));
    memset(&mConnectorProp, 0, sizeof(mConnectorProp));
    memset(&mHistData[0], 0, MAX_CRTC*sizeof(mHistData[0]));
    mExitThread = false;

    Init();
}

HWColorDrmMgr::~HWColorDrmMgr()
{

}

int HWColorDrmMgr::Init()
{
    int ret = -1;

    if (mInited)
        return 0;

    mDevFd = drmOpen("msm_drm", nullptr);
    if (mDevFd < 0) {
        ALOGE("drmOpen failed with error %d", mDevFd);
        return -ENODEV;
    }
    drmDropMaster(mDevFd);
    ret = drmSetClientCap(mDevFd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret)
        ALOGE("drmSetClientCap DRM_CLIENT_CAP_UNIVERSAL_PLANES failed with error %d", ret);
    ret = drmSetClientCap(mDevFd, DRM_CLIENT_CAP_ATOMIC, 1);
    if(ret)
        ALOGE("drmSetClientCap DRM_CLIENT_CAP_ATOMIC failed with error %d", ret);

    mRes = drmModeGetResources(mDevFd);
    if (!mRes) {
        ALOGE("Failed to get resources\n");
        return -1;
    }
    if (property_get_bool(RO_DISPLAYFEATURE_HISTOGRAM_ENABLE, true)) {
        ret = initHistogram();
        if (ret) {
            ALOGE("Failed to init Histogram");
            return -1;
        }

        mPollFd.fd = drmOpen("msm_drm", nullptr);
        if (mPollFd.fd < 0) {
            ALOGE("drmOpen failed with error %d", mPollFd.fd);
            return -ENODEV;
        }
        drmDropMaster(mPollFd.fd);
        mPollFd.events = POLLIN | POLLPRI | POLLERR;

        if (pthread_create(&mThread, NULL, &histogramEventThread, this) < 0) {
            ALOGE("Failed to start histogramEventThread");
            return -1;
        }
    }
    mInited = true;
    ALOGD("%s success! fd:0x%x", __func__, mDevFd);
    return 0;
}

int HWColorDrmMgr::Deinit()
{
    int ret = -1;
    Mutex::Autolock autoLock(mDrmLock);
    if(!mInited)
        return -1;
    if(mDevFd < 0)
        return -1;

    mExitThread = true;
    if (property_get_bool(RO_DISPLAYFEATURE_HISTOGRAM_ENABLE, true)) {
        registerHistogram(0, 0, 0);
        pthread_join(mThread, NULL);
        close(mPollFd.fd);
        mPollFd.fd = -1;
    }
    ret = drmClose(mDevFd);
    if (ret < 0) {
        printf("drmClose failed with error %d", mDevFd);
        return -ENODEV;
    }
    if(mRes)
        drmModeFreeResources(mRes);
    mRes = NULL;
    mDevFd = -1;
    return ret;
}

int HWColorDrmMgr::parseDtsInfo(unsigned char *blob_data, DrmConnectorProp *pConnector)
{
    if(!blob_data) {
        ALOGW("blob_data is null!");
        return -1;
    }
    ALOGD("%s in",__func__);
    unsigned int token_count = 0,i = 0;
    const unsigned int max_count = 20;
    char *tokens[max_count] = {NULL};
    char *p;
    int ret = DisplayUtil::parseLine((char *)blob_data, "=\n", tokens, max_count, &token_count);
    ALOGD("%s %d token_count:%d",__func__, ret, token_count);
    if(!ret && token_count > 1) {
        while(i < token_count-1) {
            if (!strncmp(tokens[i], "panel name", strlen("panel name"))) {
                snprintf(pConnector->panel.panel_name, sizeof(pConnector->panel.panel_name), "%s", tokens[i+1]);
            } else if (!strncmp(tokens[i], "panel mode", strlen("panel mode"))) {
                snprintf(pConnector->panel.panel_mode, sizeof(pConnector->panel.panel_mode), "%s", tokens[i+1]);
            } else if (!strncmp(tokens[i], "is_ccbb_enabled", strlen("is_ccbb_enabled"))) {
                pConnector->panel.ccbb_enabled = atoi(tokens[i+1]);
            } else if (!strncmp(tokens[i], "is_blnotify_enabled", strlen("is_blnotify_enabled"))) {
                pConnector->panel.bl_notify_enabled = atoi(tokens[i+1]);
            } else if (!strncmp(tokens[i], "dfps support", strlen("dfps support"))) {
                if (!strncmp(tokens[i+1], "true", strlen("true"))) {
                    pConnector->panel.dfps_support = true;
                } else {
                    pConnector->panel.dfps_support = false;
                }
            } else if (!strncmp(tokens[i], "backlight type", strlen("backlight type"))) {
                snprintf(pConnector->panel.bl_type, sizeof(pConnector->panel.bl_type), "%s", tokens[i+1]);
            } else if (!strncmp(tokens[i], "mdp_transfer_time_us", strlen("mdp_transfer_time_us"))) {
                pConnector->panel.mdp_transfer_time_us = atoi(tokens[i+1]);
            }
            i++;
        }
    }else
        ret = -1;
    ALOGD("%s out",__func__);
    return ret;
}

int HWColorDrmMgr::getBlob(uint32_t blob_id, DrmCrtcProp *pCrtc, DrmConnectorProp *pConnector)
{
    uint32_t i;
    unsigned char *blob_data;
    drmModePropertyBlobPtr blob;
    ALOGD("%s in", __func__);
    if(!mInited)
        return -1;
    blob = drmModeGetPropertyBlob(mDevFd, blob_id);
    if (!blob) {
        ALOGE("%s fail !\n",__func__);
        return -1;
    }

    blob_data = (unsigned char *)blob->data;
    ALOGD("blob data len:%d\n", blob->length);

    if(pConnector) {
        parseDtsInfo(blob_data, pConnector);
    } else if(pCrtc) {

    }
    drmModeFreePropertyBlob(blob);
    ALOGD("%s out", __func__);
    return 0;
}

int HWColorDrmMgr::getObjectProperties(uint32_t id, uint32_t type,
    DrmCrtcProp *pCrtc, DrmConnectorProp *pConnector, bool *find)
{
    unsigned int i;
    drmModeObjectPropertiesPtr props;

    props = drmModeObjectGetProperties(mDevFd, id, type);

    if (!props) {
        ALOGE(" No properties: %s.", strerror(errno));
        return -1;
    }
    ALOGD("id:%d type:0x%x prop count %d",id, type , props->count_props);
    drmModePropertyPtr prop;
    for (i = 0; i < props->count_props; i++) {
        prop = drmModeGetProperty(mDevFd, props->props[i]);
        if(type == DRM_MODE_OBJECT_CRTC && pCrtc && prop) {
            if(!strcmp(prop->name, pCrtc->prop_name)) {
                pCrtc->object_id = id;
                pCrtc->property_id = prop->prop_id;
                pCrtc->obj_type = type;
                *find = true;
            }
        } else if(type == DRM_MODE_OBJECT_CONNECTOR && pConnector && prop) {
            if(id == pConnector->connectorId) {
                if (drm_property_type_is(prop, DRM_MODE_PROP_BLOB)) {
                    if(!strcmp(prop->name, pConnector->prop_name)) {
                        getBlob(props->prop_values[i],NULL, pConnector);
                        *find = true;
                    }
                }
            }
        }
        drmModeFreeProperty(prop);
    }
    drmModeFreeObjectProperties(props);

    return 0;
}

int HWColorDrmMgr::GetCrtcProperties(DrmCrtcProp *pCrtc, int crtc_index)
{
    int i;
    drmModeCrtcPtr c;

    c = drmModeGetCrtc(mDevFd, mRes->crtcs[crtc_index]);
    if (!c) {
        ALOGE("Could not get crtc %u: %s", mRes->crtcs[crtc_index], strerror(errno));
    }
    ALOGD("CRTC %u\n", c->crtc_id);
    bool find = false;
    getObjectProperties(c->crtc_id, DRM_MODE_OBJECT_CRTC, pCrtc, NULL, &find);
    drmModeFreeCrtc(c);

    return find;
}

int HWColorDrmMgr::GetConnectorProperties(DrmConnectorProp *pConnector)
{
    Mutex::Autolock autoLock(mDrmLock);

    drmModeConnectorPtr con;

    for (int i = 0; i < mRes->count_connectors; i++) {
        con = drmModeGetConnector(mDevFd, mRes->connectors[i]);
        if (!con) {
            ALOGE("Could not get connector %u: %s\n",
                mRes->connectors[i], strerror(errno));
            continue;
        }
        ALOGD("Connector %u (%s-%u) type 0x%x\n", con->connector_id,
               "name", con->connector_type_id, con->connector_type);
        bool find = false;
        if(con->connector_type == DRM_MODE_CONNECTOR_DSI) {
            getObjectProperties(con->connector_id,
                     DRM_MODE_OBJECT_CONNECTOR,NULL, pConnector, &find);
        }
        drmModeFreeConnector(con);
        if(find)
            break;
    }

    return 0;
}

int HWColorDrmMgr::CreatePropertyBlob(const void *data, size_t length, uint32_t *id)
{
    Mutex::Autolock autoLock(mDrmLock);
    if(!mInited)
        return -1;
    return drmModeCreatePropertyBlob(mDevFd, data, length, id);
}

int HWColorDrmMgr::DestroyPropertyBlob(struct drm_mode_create_blob *create_blob)
{
    Mutex::Autolock autoLock(mDrmLock);
    if(!mInited)
        return -1;

    return drmModeDestroyPropertyBlob(mDevFd, create_blob->blob_id);
}

int HWColorDrmMgr::SetObjectProperty(DrmPccInfo *pConfig, struct drm_mode_create_blob *pBlob)
{
    Mutex::Autolock autoLock(mDrmLock);
    if(!mInited || pConfig == NULL || pBlob == NULL)
        return -1;
    int ret = -1;
    uint32_t obj_id = pConfig->object_id;
    uint32_t prop_id = pConfig->property_id;
    uint32_t obj_type = pConfig->obj_type;

    ret = drmModeObjectSetProperty(mDevFd, obj_id, obj_type,
              prop_id, pBlob->blob_id);
    if (ret) {
       ALOGE("%s failed to set property %d", __func__, ret);
       return ret;
    }
    return ret;
}

int HWColorDrmMgr::handleHistogramCmd(int displayId, int user,unsigned int cmd, DrmHistBlobValue *data)
{
    Mutex::Autolock autoLock(mDrmLock);
    int ret = 0;

    //ALOGD("%s: enter with displayId = %d, user = %d", __func__, displayId, user);

    if (!mInited)
        return ret;

        switch (cmd) {
        case HIST_CMD_STOP: {
            ALOGD("%s disable histogram", __func__);
            ret = registerHistogram(displayId, user, 0);
            break;
        }
        case HIST_CMD_START: {
            ALOGD("%s enable histogram", __func__);
            ret = registerHistogram(displayId, user, 1);
            break;
        }
        case HIST_CMD_GET: {
            memcpy((void *)data, &mHistData[displayId], sizeof(mHistData[displayId]));
            break;
        }
        default:
            ALOGW("%s does not support command type", __func__);
        }

    return ret;
}

void *HWColorDrmMgr::histogramEventThread(void *context) {
    if (context) {
        return reinterpret_cast<HWColorDrmMgr *>(context)->histogramEventHandler();
    }

    return NULL;
}

void *HWColorDrmMgr::histogramEventHandler() {
    char event_data[1024];
    struct drm_msm_event_resp *event_resp = NULL;
    drmModePropertyBlobPtr blob;
    uint32_t hist_blob_id;

    while (!mExitThread) {
        int error = poll(&mPollFd, 1, -1);
        if (error <= 0) {
            ALOGW("poll failed. error = %s", strerror(errno));
            continue;
        }

        if (mPollFd.fd < 0) {
            continue;
        }

        if (mPollFd.revents & (POLLIN | POLLPRI | POLLERR)) {
            int32_t size = pread(mPollFd.fd, event_data, 1024, 0);
            if (size < 0) {
                ALOGE("pread failed");
                continue;
            }

            if (size < (int32_t)sizeof(*event_resp)) {
                ALOGE("event size is %d, expected %zd\n", size, sizeof(*event_resp));
                continue;
            }

            int32_t i = 0;
            while (i < size) {
                event_resp = (struct drm_msm_event_resp *)&event_data[i];
                if (event_resp->base.type == DRM_EVENT_HISTOGRAM) {
                    hist_blob_id = (*(uint32_t *)event_resp->data);
                    /* histogram blobId hardcode, can not read from driver */
                    blob = drmModeGetPropertyBlob(mPollFd.fd, hist_blob_id);
                    if (!blob) {
                        ALOGE("drmModeGetPropertyBlob fail !\n");
                        continue;
                    }

                    {
                        Mutex::Autolock autoLock(mDrmLock);
                        for (int m = 0; m < mRes->count_crtcs; m++) {
                            if (event_resp->info.object_id == mHistConfig[m].object_id) {
                                if (blob->length <= sizeof(DrmHistBlobValue)) {
                                    memcpy((void *)&mHistData[m], blob->data, blob->length);
                                     mHistData[m].object_id = event_resp->info.object_id;
/*
                                     ALOGE("display = %d, ctrc id:%d, hist blob data len:%d\n",
                                        m, mHistData[m].object_id, blob->length);
                                        for (int i = 0; i < 32; i++) {
                                            ALOGE("histogram value, %d~%d: %d, %d, %d, %d, %d, %d, %d, %d", 8*i, 8*i+7,
                                            mHistData[m].blob_data[8*i], mHistData[m].blob_data[8*i+1],
                                            mHistData[m].blob_data[8*i+2], mHistData[m].blob_data[8*i+3],
                                            mHistData[m].blob_data[8*i+4], mHistData[m].blob_data[8*i+5],
                                            mHistData[m].blob_data[8*i+6],mHistData[m].blob_data[8*i+7]);
                                       }
*/
                                }
#ifndef HIST_INT_PROP_SKIP
                           drmModeObjectSetProperty(mPollFd.fd, mHistConfig[m].object_id,
                               mHistConfig[m].obj_type, mHistConfig[m].property_2_id, 1);
#endif
                                break;
                            } else {
                                continue;
                            }
                        }
                    }
                        drmModeFreePropertyBlob(blob);
                    }
                i += event_resp->base.length;
            }
        }
    }

    pthread_exit(0);

    return NULL;
}

int HWColorDrmMgr::registerHistogram(int displayId, int user, int enable) {
    static int hist_status[MAX_CRTC] = {0};
    static int hist_users[MAX_CRTC] = {0};
    struct drm_msm_event_req req = {};
    int ret = 0;
    int i =0;

    for (i = 0; i < mRes->count_crtcs -1; i++) {
        if (i == displayId) {
            if (enable)
                hist_users[i] |= (0x1 << (displayId == 0 ? user : user + FEATURA_MAX));
            else
                hist_users[i] &= ~(0x1 << (displayId == 0 ? user : user + FEATURA_MAX));

           ALOGD("registerHistogram enter i= %d, hist_users[i] = %d, enable  = %d",
               i, hist_users[i], enable);

          if (hist_users[i] > 0 && !enable) {
              continue;
          }
            req.object_id = mHistConfig[i].object_id;
            req.object_type = mHistConfig[i].obj_type;
            req.event = DRM_EVENT_HISTOGRAM;
            if (enable) {
                ret = drmIoctl(mPollFd.fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
            } else {
                ret = drmIoctl(mPollFd.fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
            }

            if (ret) {
                ALOGE("crtc%d register Histogram enable:%d ->failed", i, enable);
                ret = 0;
                continue;
            } else {
                ALOGD("crtc%d register Histogram enable:%d ->success", i, enable);
            }

#ifdef HIST_INT_PROP_SKIP
            sp<IMiStcService> imistcservice;
            android::Parcel data;
            data.writeInt32(i);  /* display index */
            data.writeInt32(mHistConfig[i].property_id);
            data.writeInt32(mHistConfig[i].object_id);
            data.writeInt32(enable);

            imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
            if (imistcservice.get()) {
                ret = imistcservice->dispatch(IMiStcService::SET_DPPS_PROP, &data, NULL);
            } else {
                ALOGE("Failed to acquire %s", "display.qservice");
                ret = -1;
            }
#else
            ret |= drmModeObjectSetProperty(mPollFd.fd, mHistConfig[i].object_id,
                  mHistConfig[i].obj_type, mHistConfig[i].property_id, enable);

            ret |= drmModeObjectSetProperty(mPollFd.fd, mHistConfig[i].object_id,
                  mHistConfig[i].obj_type, mHistConfig[i].property_2_id, enable);
#endif

            hist_status[i] = enable;
        }
    }

    return ret;
}

int HWColorDrmMgr::initHistogram()
{
    memset(&mHistConfig[0], 0, sizeof(mHistConfig));

    ALOGD("mRes->count_crtcs =%d", mRes->count_crtcs);

    for (int i = 0; i < mRes->count_crtcs; i++) {
        memset(&mCrtcProp, 0, sizeof(mCrtcProp));
        snprintf(mCrtcProp.prop_name, sizeof(mCrtcProp.prop_name), "%s", "SDE_DSPP_HIST_CTRL_V1");
        mCrtcProp.obj_type = DRM_MODE_OBJECT_CRTC;
        GetCrtcProperties(&mCrtcProp, i);
        ALOGD("object_id %d property_id %d crtc_prop:%s",
            mCrtcProp.object_id, mCrtcProp.property_id, mCrtcProp.prop_name);
        if(mCrtcProp.object_id > 0 && mCrtcProp.property_id > 0) {
            mHistConfig[i].property_id = mCrtcProp.property_id;
            mHistConfig[i].object_id = mCrtcProp.object_id;
            mHistConfig[i].obj_type = DRM_MODE_OBJECT_CRTC;
        } else {
            return -1;
        }

        memset(&mCrtcProp, 0 ,sizeof(mCrtcProp));
        snprintf(mCrtcProp.prop_name, sizeof(mCrtcProp.prop_name), "%s", "SDE_DSPP_HIST_IRQ_V1");
        mCrtcProp.obj_type = DRM_MODE_OBJECT_CRTC;
        GetCrtcProperties(&mCrtcProp, i);
        ALOGD("object_id %d property_id %d crtc_prop:%s",
            mCrtcProp.object_id, mCrtcProp.property_id, mCrtcProp.prop_name);
        if(mCrtcProp.object_id > 0 && mCrtcProp.property_id > 0) {
            mHistConfig[i].property_2_id = mCrtcProp.property_id;
        } else {
            return -1;
        }
    }

    ALOGD("%s success !\n", __func__);
    return 0;
}

}

