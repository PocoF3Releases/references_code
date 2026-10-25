#ifndef __HW_COLOR_DRM_MANAGER_H__
#define __HW_COLOR_DRM_MANAGER_H__

#include <xf86drmMode.h>
#include <utils/Singleton.h>
#include "../display_utils.h"
#include <pthread.h>
#include <sys/poll.h>

namespace android {

typedef struct {
    uint32_t *payload;
    uint32_t len;
    uint32_t object_id;
    uint32_t property_id;
    uint32_t obj_type;
    uint64_t value;
} DrmPccInfo;

typedef struct {
    uint32_t *payload;
    uint32_t len;
    uint32_t object_id;
    uint32_t property_id;
    uint32_t property_2_id;
    uint32_t obj_type;
    uint32_t cmd;
    uint64_t value;
} DrmHistInfo;

typedef struct {
    uint32_t object_id;//out
    uint32_t property_id;//out
    uint32_t obj_type;//in
    char prop_name[DRM_PROP_NAME_LEN];//in
} DrmCrtcProp;

typedef struct {
    struct PanelInfo panel;
    uint32_t connector_type;
    uint32_t connectorId;//in
    char prop_name[DRM_PROP_NAME_LEN];
} DrmConnectorProp;

typedef struct {
    uint64_t flag;
    uint32_t blob_data[256];
    uint32_t object_id;
} DrmHistBlobValue;

enum HANDLE_HIST_CMD {
    HIST_CMD_STOP,
    HIST_CMD_START,
    HIST_CMD_GET,
    HIST_CMD_MAX,
};

enum CONTROL_FEATURE_FLAG{
    HBM_FEATURE,
    SRE_FEATURE,
    BCBC_FEATURE,
    VSDR2HDR_FEATURE,
    IGC_DITHER,
    FEATURA_MAX,
    GAME_SRE_FEATURE,
};

#define MAX_CRTC 6

class HWColorDrmMgr: public Singleton<HWColorDrmMgr> {
public:
    HWColorDrmMgr();
    ~HWColorDrmMgr();
    int Init();
    int Deinit();
    int handleHistogramCmd(int displayId, int user,unsigned int cmd, DrmHistBlobValue *data);
    int GetCrtcProperties(DrmCrtcProp *pCrtc, int crtc_index);
    int GetConnectorProperties(DrmConnectorProp *pConnector);
    int CreatePropertyBlob(const void *data, size_t length, uint32_t *id);
    int DestroyPropertyBlob(struct drm_mode_create_blob *create_blob);
    int SetObjectProperty(DrmPccInfo *pConfig, struct drm_mode_create_blob *pBlob);

protected:
    int getObjectProperties(uint32_t id, uint32_t type, DrmCrtcProp *pCrtc, DrmConnectorProp *pConnector, bool *find);
    int getBlob(uint32_t blob_id, DrmCrtcProp *pCrtc, DrmConnectorProp *pConnector);
    int parseDtsInfo(unsigned char *blob_data, DrmConnectorProp *pConnector);

private:
    static void *histogramEventThread(void *context);
    void *histogramEventHandler();
    int registerHistogram(int displayId, int user, int enable);
    int initHistogram();

    int mDevFd;
    drmModeResPtr mRes;
    bool mInited;
    Mutex mDrmLock;
    DrmCrtcProp mCrtcProp;
    DrmConnectorProp mConnectorProp;

    pthread_t mThread;
    pollfd mPollFd;
    bool mExitThread;
    DrmHistInfo mHistConfig[MAX_CRTC];
    DrmHistBlobValue mHistData[MAX_CRTC];
};

};

#endif
