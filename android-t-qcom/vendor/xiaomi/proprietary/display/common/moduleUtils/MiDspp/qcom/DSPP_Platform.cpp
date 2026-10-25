#include "DSPP_Platform.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "DisplayFeatureHal"
#endif

using snapdragoncolor::IMiStcService;

namespace android {
int DFLOG::loglevel = 1;
DSPP_Platform::DSPP_Platform(int displayId)
        :mDisplayId(displayId),
        DSPP_Common(displayId)
{

}

int DSPP_Platform::setPccConfig(double *rgbCoeff, int bl)
{
    int ret = 0;
    int enable = 1;
    sp<IMiStcService> imistcservice;
    android::Parcel data;
    double coeff[INDEX_SIZE] = {1.0, 0.0, 0.0, 0.0,
                                0.0, 1.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0,
                                0.0, 0.0, 0.0, 1.0};
    data.writeInt32(enable);
    data.writeInt32(mDisplayId);
    //multiply 3x3 and 4x4 matrix here. Now we only have 3x3 matrix, just put it in coeff.
    for (int i = 0; i < MATRIX_SIZE; i++)
        for (int j = 0; j < MATRIX_SIZE; j++) {
            if ((j != MATRIX_SIZE - 1) && (i < MATRIX_SIZE - 1))
                coeff[i * MATRIX_SIZE + j] = rgbCoeff[i * (MATRIX_SIZE - 1) + j];
            data.writeDouble(coeff[i * MATRIX_SIZE + j]);
        }

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_PCC, &data, NULL);
    } else {
        ALOGW("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }
    return ret;
}

int DSPP_Platform::setMode(int mode, int cookie)
{
    int ret = 0;
    android::sp<qService::IQService> iqservice;
    android::Parcel data;
    data.writeInt32(0);
    data.writeInt32(mode);
    iqservice = interface_cast<qService::IQService>(defaultServiceManager()->getService(android::String16("display.qservice")));
    if (iqservice.get()) {
        ret = iqservice->dispatch(qService::IQService::SET_COLOR_MODE_BY_ID, &data, NULL);
    } else {
        DF_LOGW("Failed to acquire %s", "display.qservice");
        ret = -1;
    }

    return ret;
}

int DSPP_Platform::setColorMode(int mode, int cookie, enum DISPLAY_PP_MODE * renderIntent)
{
    int ret = 0;
    enum DISPLAY_PP_MODE ppMode;
    static enum DISPLAY_PP_MODE currentMode = DISPLAY_PP_MODE_mode258;

    switch ((DISPLAY_COLOR_MODE)mode) {
        case DISPLAY_COLOR_MODE_NATURE: {
            ppMode = DISPLAY_PP_MODE_mode256;
        } break;
        case DISPLAY_COLOR_MODE_sRGB: {
            ppMode = DISPLAY_PP_MODE_mode257;
        } break;
        case DISPLAY_COLOR_MODE_P3: {
            ppMode = DISPLAY_PP_MODE_mode257;
        } break;
        case DISPLAY_COLOR_HDR: {
            ppMode = DISPLAY_PP_MODE_mode0;
        } break;
        case DISPLAY_COLOR_MODE_ACNORMAL: {
            ppMode = DISPLAY_PP_MODE_mode258;
        } break;
        case DISPLAY_COLOR_MODE_GAME1: {
            ppMode = DISPLAY_PP_MODE_mode263;
        } break;
        case DISPLAY_COLOR_MODE_GAME2: {
            ppMode = DISPLAY_PP_MODE_mode264;
        } break;
        case DISPLAY_COLOR_MODE_GAME3: {
            ppMode = DISPLAY_PP_MODE_mode265;
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_NONE: {
            ppMode = DISPLAY_PP_MODE_mode266;
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_SRGB: {
            ppMode = DISPLAY_PP_MODE_mode267;
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_P3: {
            ppMode = DISPLAY_PP_MODE_mode268;
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_WCG: {
            ppMode = DISPLAY_PP_MODE_mode269;
        } break;
        case DISPLAY_COLOR_MODE_VIDEO1: {
            ppMode = DISPLAY_PP_MODE_mode259;
        } break;
        case DISPLAY_COLOR_MODE_VIDEO2: {
            ppMode = DISPLAY_PP_MODE_mode260;
        } break;
        case DISPLAY_COLOR_MODE_VIDEO3: {
            ppMode = DISPLAY_PP_MODE_mode261;
        } break;
        case DISPLAY_COLOR_MODE_VIDEO4: {
            ppMode = DISPLAY_PP_MODE_mode262;
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_TRANSITION: {
            ppMode = DISPLAY_PP_MODE_mode270;
        } break;
        case DISPLAY_COLOR_MODE_ACNORMAL_FOD: {
            ppMode = DISPLAY_PP_MODE_mode271;
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_P3_FOD: {
            ppMode = DISPLAY_PP_MODE_mode272;
        } break;
        case DISPLAY_COLOR_MODE_SCENE_ACNORMAL_NONE: {
            ppMode = DISPLAY_PP_MODE_mode281;
        } break;
        case DISPLAY_COLOR_MODE_SCENE0: {
            ppMode = DISPLAY_PP_MODE_mode282;
        } break;
        case DISPLAY_COLOR_MODE_SCENE1: {
            ppMode = DISPLAY_PP_MODE_mode283;
        } break;
        case DISPLAY_COLOR_MODE_SCENE2: {
            ppMode = DISPLAY_PP_MODE_mode284;
        } break;
        case DISPLAY_COLOR_MODE_SCENE3: {
            ppMode = DISPLAY_PP_MODE_mode285;
        } break;
        case DISPLAY_COLOR_MODE_SCENE4: {
            ppMode = DISPLAY_PP_MODE_mode286;
        } break;
        case DISPLAY_COLOR_MODE_SCENE_NATIVE_NONE: {
            ppMode = DISPLAY_PP_MODE_mode287;
        } break;
        case DISPLAY_COLOR_MODE_SCENE0_NATIVE: {
            ppMode = DISPLAY_PP_MODE_mode288;
        } break;
        case DISPLAY_COLOR_MODE_DV_FLAT_OFF: {
            ppMode = DISPLAY_PP_MODE_mode273;
        } break;
        case DISPLAY_COLOR_MODE_DV_FLAT_ON: {
            ppMode = DISPLAY_PP_MODE_mode274;
        } break;
        default:
            DF_LOGE("error mode ...");
            ppMode = DISPLAY_PP_MODE_mode256;
    }
    //DisplayUtil::onCallback(30000, ppMode);

    forceScreenRefresh();
    currentMode = ppMode;
    *renderIntent = ppMode;
    DF_LOGD("set mode:%d ppMode:%d over done.... ret %d", mode, (int)ppMode, ret);
    return ret;
}

int DSPP_Platform::setGlobalPA(int caseId, int value, int dimming, GlobalPAParam param)
{
    int ret = 0;
    int enable = 1;
    disp_pa_config_data cfg;

    cfg.hue        = param.hue;
    cfg.saturation = param.saturation / 100.0;
    cfg.value      = (float)param.value;
    cfg.contrast   = param.contrast/100.0;
    cfg.sat_thresh = 0.0;

    switch (caseId) {
        case EXPERT_CLEAR:
            cfg.hue        = 0;
            cfg.saturation = 0.0;
            cfg.value      = 0.0;
            cfg.contrast   = 0.0;
            cfg.sat_thresh = 0.0;
            break;
        case EXPERT_SRE_H_AND_S:
            cfg.hue    = (value >> 8) & 0xFF;
            cfg.saturation = (value & 0xFF)/100.0;
            DF_LOGV("EXPERT_SRE_H_AND_S hue = %d, saturation = %f", cfg.hue, cfg.saturation);
        break;
        case NOT_EXPERT_SRE_H_AND_S:
            cfg.hue    = (value >> 8) & 0xFF;
            cfg.saturation = (value & 0xFF)/100.0;
            cfg.value      = 0.0;
            cfg.contrast   = 0.0;
            cfg.sat_thresh = 0.0;
            DF_LOGV("NOT_EXPERT_SRE_H_AND_S hue = %d, saturation = %f", cfg.hue, cfg.saturation);
        break;
        default:
            break;
    }

    sp<IMiStcService> imistcservice;
    android::Parcel data;
    int cmd;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(enable);
    data.writeInt32(mDisplayId);
    data.writeInt32(dimming);
    data.writeInt32(cfg.hue);
    data.writeFloat(cfg.saturation);
    data.writeFloat(cfg.value);
    data.writeFloat(cfg.contrast);
    data.writeFloat(cfg.sat_thresh);
    cmd = IMiStcService::SET_PA;
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(cmd, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }

    return ret;
}

int DSPP_Platform::setGammaCorrection(int targetGamma, int type, struct DispPPParam *pSDPPParams)
{
    int ret = 0;
    float temp;
    int iEntries[256];
    int entries[1024];
    int enable = 1;
    int dimming = 0;
    int fileId = -1;
    std::vector<int32_t> payload;
    sp<IMiStcService> imistcservice;
    android::Parcel data;
    int cmd;

    enable = pSDPPParams->vec_payload[0];
    fileId = pSDPPParams->vec_payload[1];
    dimming = pSDPPParams->vec_payload[2];

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(enable);
    data.writeInt32(mDisplayId);
    data.writeInt32(dimming);
    data.writeInt32(targetGamma);
    if (targetGamma == 0) {
        data.writeInt32(fileId);
    }
    if (GAMMA_IGC == type) {
        cmd = IMiStcService::SET_IGC;
    } else if (GAMMA_GC == type) {
        cmd = IMiStcService::SET_GC;
    }else if (GAMMA_SRE_IGC == type) {
        payload.assign(pSDPPParams->vec_payload.begin() + 3, pSDPPParams->vec_payload.end());
        data.writeInt32Vector(static_cast<std::vector<int32_t>>(payload));
        cmd = IMiStcService::SET_SRE_IGC;
    }

    if (imistcservice.get()) {
        ret = imistcservice->dispatch(cmd, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }

    return ret;
}

int DSPP_Platform::setIGCDither(int modeId, int strength)
{
    int ret = 0;
    sp<IMiStcService> imistcservice;
    android::Parcel data;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(modeId);  //Enable
    data.writeInt32(mDisplayId);  //DisplayId
    data.writeInt32(strength);  //strength
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_IGC_DITHER, &data, NULL);
    } else {
        ALOGE("Failed to acquire %s", "display.qservice");
        ret = -1;
    }
    return ret;
}

int DSPP_Platform::setLut(int lutId, int cookie, int dimming)
{
    int enable = cookie;
    int reInit = 0;
    int ret = 0;

    sp<IMiStcService> imistcservice;
    android::Parcel data;
    int cmd;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(enable);
    data.writeInt32(mDisplayId);
    data.writeInt32(dimming);
    data.writeInt32(reInit);
    data.writeInt32(lutId);
    cmd = IMiStcService::SET_LUT;
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(cmd, &data, NULL);
    } else {
        ALOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }
    return ret;
}

int DSPP_Platform::DisablePcc()
{
    int ret = 0;
    sp<IMiStcService> imistcservice;
    android::Parcel data;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(0);
    data.writeInt32(mDisplayId);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_PCC, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }
    return ret;
}

int DSPP_Platform::forceScreenRefresh()
{
    int ret = 0;
    sp<qService::IQService> iqservice;

    iqservice = interface_cast<qService::IQService>(defaultServiceManager()->getService(String16("display.qservice")));
    if (iqservice.get()) {
        iqservice->dispatch(qService::IQService::SCREEN_REFRESH, NULL, NULL);
    } else {
        DF_LOGW("Failed to acquire %s", "display.qservice");
        ret = -1;
    }
    return ret;
}

int DSPP_Platform::DisablePA()
{
    int ret = 0;
    sp<IMiStcService> imistcservice;
    android::Parcel data;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(0);
    data.writeInt32(mDisplayId);
    data.writeInt32(1);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_PA, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }
    return ret;
}

int DSPP_Platform::DisableIGC()
{
    int ret = 0;
    sp<IMiStcService> imistcservice;
    android::Parcel data;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(0);
    data.writeInt32(mDisplayId);
    data.writeInt32(1);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_IGC, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.qservice");
        ret = -1;
    }
    return ret;
}

int DSPP_Platform::DisableGC()
{
    int ret = 0;
    sp<IMiStcService> imistcservice;
    android::Parcel data;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(0);
    data.writeInt32(mDisplayId);
    data.writeInt32(1);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_GC, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }

    return ret;
}

int DSPP_Platform::DisableLut()
{
    int ret = 0;
    sp<IMiStcService> imistcservice;
    android::Parcel data;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(0);
    data.writeInt32(mDisplayId);
    data.writeInt32(1);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_LUT, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }

    return ret;
}
} // namespace android
