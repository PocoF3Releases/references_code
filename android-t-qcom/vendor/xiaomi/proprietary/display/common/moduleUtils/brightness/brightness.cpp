#include "brightness.h"
#include "timer.hpp"
#include <media/stagefright/foundation/ADebug.h>
#include "DisplayEffectBase.h"
#ifndef MTK_PLATFORM
#include "fpsmonitor.h"
#include "libqdutils/display_config.h"
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "MiBrightness"
#endif

using namespace std;

namespace android {
int DFLOG::loglevel = 1;
void BrightnessTimerCallbackFunction(void *user, int caseId ,int generation, int value)
{
    MiBrightness *source = (MiBrightness *) user;
    source->timerCallback(caseId, generation, value);
}

MiBrightness::MiBrightness(const sp<DisplayEffectBase>& display_effect)
      : mDisplayEffect(display_effect)
{
    if (mDisplayEffect.get()) {
        displayId = mDisplayEffect->mDisplayId;
    }
    mParseXml[displayId] = new MiParseXml();
    miDDM = new DisplayDeviceManager();
    mBacklightPoll = property_get_bool(RO_XIAOMI_BL_POLL, false);
    if (mBacklightPoll) {
        registerPollEvent();
        pthread_create(&mThreadBacklight, NULL, ThreadWrapperBacklight, this);
    }
}

MiBrightness::~MiBrightness()
{
    if (mBacklightPoll) {
        pthread_join(mThreadBacklight, NULL);
        mThreadBacklight = 0;
        close(mDFPollFd.fd);
        mDFPollFd.fd = -1;
        mBacklightPoll = 0;
    }
}

void MiBrightness::init()
{
    char temp[50];
    const unsigned int max_count = 1024;
    char *tokens[max_count] = {NULL};
    unsigned int count = 0;
    int ret = 0;
    char p[4096];
    if (!mParseXml[displayId].get()) {
        return;
    }
    param.hdrStatus = 0;
    param.hbmStatus = 0;
    param.brightness = 0;
    param.dbv = 0;
    param.pccVal = 1.0;
    param.DCBLEnable = property_get_bool(PERSIST_DISPLAYFEATURE_DC_BL_ENABLE, false);

    string key("Mi12bitEnable");
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.Mi12BitEnable = atoi(p);
        DF_LOGV("Mi12BitEnable: %d",atoi(p));
    }
    key = "HbmEnable";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.hbmEnable = atoi(p);
        DF_LOGV("hbmEnable: %d",atoi(p));
    }
    key = "DCBacklightDisable";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
    } else {
        if(atoi(p) == 1 && param.DCBLEnable) {
            param.DCBLEnable = 0;
            property_set(PERSIST_DISPLAYFEATURE_DC_BL_ENABLE, "false");
            DF_LOGV("DCBacklightDisable: %d",atoi(p));
        }
    }
    key = "DcDimmingEnable";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.dcDimmingEnable = atoi(p);
        DF_LOGV("dcDimmingEnable: %d",atoi(p));
    }
    key = "hbmtype";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.hbmType = atoi(p);
        DF_LOGV("hbmType: %d",atoi(p));
    }
    key = "PanelMaxDbv";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.panelMaxDbv = atoi(p);
        DF_LOGV("panelMaxDbv: %d",atoi(p));
    }
    key = "PanelVirtualMax";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        param.panelVirtualMax = 4095;
    } else {
        param.panelVirtualMax = atoi(p);
        DF_LOGV("Panel virtual max %d", atoi(p));
    }
    key = "HBMMaxDbv";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        param.hbmMaxDbv = 0;
    } else {
        param.hbmMaxDbv = atoi(p);
        DF_LOGV("hbmMaxDbv: %d",atoi(p));
    }
    key = "ManualMaxBl";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.manualMaxBl = atoi(p);
        DF_LOGV("manualMaxBl: %d",atoi(p));
    }
    key = "autoMaxBl";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.autoMaxBl = atoi(p);
        DF_LOGV("autoMaxBl: %d",atoi(p));
    }
    key = "ManualminBl";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.minBl = atoi(p);
        DF_LOGV("minBl: %d",atoi(p));
    }
    key = "Mi12BitThr";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.Mi12BitThresh = atoi(p);
        DF_LOGV("Mi12BitThresh: %d",atoi(p));
    }

    key = "Mi12BitpanelDBVThr";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.Mi12BitPanelDBVThresh = atoi(p);
        DF_LOGV("Mi12BitPanelDBVThresh: %d",atoi(p));
    }
    key = "DCType";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.dcType = atoi(p);
        DF_LOGV("dcType: %d",atoi(p));
    }
    key = "DCDbvThresh";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.dcDBVThresh = atoi(p);
        DF_LOGV("dcDbvThresh: %d",atoi(p));
    }
    key = "MinDBV";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        param.pwmMinDBV = atoi(p);
        DF_LOGV("pwmMinDBV: %d",atoi(p));
    }
    for (int i = 0; i < 4; i++) {
        key = "MiBlLut";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 100; j++) {
                BlLut.push_back(atoi(tokens[j]));
                DF_LOGV("BlLut %d %d %d ",i, j, atoi(tokens[j]));
            }
        }
    }
    for (int i = 0; i < 4; i++) {
        key = "MiBlLutDC";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 100; j++) {
                BlLutDC.push_back(atoi(tokens[j]));
                DF_LOGV("BlLutDC %d %d %d ",i, j, atoi(tokens[j]));
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        key = "Mi12BitPCC";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 100; j++) {
                pccLut.push_back(atof(tokens[j]));
                DF_LOGV("pccLut %d %d %f ",i, j, atof(tokens[j]));
            }
        }
    }

    for (int i = 0; i < 4; i++) {
        key = "Mi12BitPCCDC";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 100; j++) {
                DF_LOGV("pccLutDC %d %d %f ",i, j, atof(tokens[j]));
                pccLutDC.push_back(atof(tokens[j]));

            }
        }
    }
    key = "GabbEnable";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        param.GabbEnable = 0;
    } else {
        param.GabbEnable = atoi(p);
        DF_LOGV("GabbEnable: %d",atoi(p));
    }
    key = "PccGABBThreshold";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        param.GabbThresh = 0;
    } else {
        param.GabbThresh = atoi(p);
        DF_LOGV("GabbThresh: %d",atoi(p));
    }
    if (param.dcType == DC_TYPE_PCC) {
        key = "PccDCDivision";
        ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            param.pccDCDivision = 0;
        } else {
            param.pccDCDivision = atoi(p);
            DF_LOGV("pccDCDivision: %d",atoi(p));
        }

        key = "PccDCCoeff";
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int i = 0; i < count; i++) {
                pccDCCoeff.push_back(atof(tokens[i]));
                DF_LOGV("pccDCCoeff %d %f",i,atof(tokens[i]));
            }
        }
    }
    key = "MTKSilkyBrightnessEnable";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        param.silkyBrightnessEnable = 0;
    } else {
        param.silkyBrightnessEnable = atoi(p);
        DF_LOGV("silkyBrightnessEnable: %d",atoi(p));
    }
    key = "IcDimmingDisable";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        param.IcDimmingDisable = 0;
    } else {
        param.IcDimmingDisable = atoi(p);
        DF_LOGV("IcDimmingDisable: %d",atoi(p));
    }

    if (mLooper == NULL) {
        mLooper = new ALooper;
        mLooper->setName("MiBrightness");
        mLooper->start();

        mLooper->registerHandler(this);
    }
}

void MiBrightness::updateBlCoeff(float coeff)
{
    mCoeff = coeff;
    sendBrightness(UPDATE_NORMAL);
}

void MiBrightness::sendBrightness(int brightness)
{
    sp<AMessage> msg = new AMessage(kBRIGHTNESS, this);
    msg->setInt32("ModeId", brightness);
    msg->post();
}

void MiBrightness::HandleHDR(int enable)
{
    param.hdrStatus = enable;
    sendBrightness(UPDATE_NORMAL);
}

int MiBrightness::timerCallback(int caseId, int generation, int value)
{
    int ret = 0;
    DF_LOGV("enter caseId %d, value %d, generation %d, mGeneration %d",
             caseId, value, generation, mTimerGeneration[caseId]);

    switch (caseId) {
        case TIMER_BACKLIGHT: {
            HandleBrightness(value);
        } break;
        case TIMER_HBM_OFF: {
            if (!param.hdrStatus) {
                HandleHBM(HBM_CMD_OFF);
            }
        } break;
        default: {
            DF_LOGE("invalid message");
        } break;
    }
    return ret;

}

void MiBrightness::onMessageReceived(const sp<AMessage> &msg)
{
    int32_t value, cookie;
    switch (msg->what()) {
        case kBRIGHTNESS:
        {
            CHECK(msg->findInt32("ModeId", &value));
            HandleBrightness(value);
            break;
        }
        default:
            DF_LOGE("Invalid message %d", msg->what());
            break;
    }
}

int MiBrightness::registerPollEvent()
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

    for (df_event_index = 0; df_event_index < DF_EVENT_MAX; df_event_index++) {
        event_req.base.flag = 0;
        event_req.base.disp_id = displayId;

        switch (df_event_index) {
        case DF_EVENT_BACKLIGHT_CLONE:
            event_req.type = MI_DISP_EVENT_BRIGHTNESS_CLONE;
            break;
        default:
            break;
        }

        DF_LOGD("event type name: %s\n", getDispEventTypeName(event_req.type));
        ret = ioctl(mDFPollFd.fd, MI_DISP_IOCTL_REGISTER_EVENT, &event_req);
        if(ret) {
            DF_LOGE("ioctl MI_DISP_IOCTL_REGISTER_EVENT fail\n");
            return -1;
        }
    }
    return 0;
}

void *MiBrightness::ThreadWrapperBacklight(void *context) {
    if (context) {
        return (void *)(uintptr_t)static_cast<MiBrightness *>(context)->threadFuncBacklight();
    }
    return NULL;
}

int MiBrightness::threadFuncBacklight() {
    char event_data[1024] = {0};
    int ret = 0;
    struct disp_event_resp *event_resp = NULL;
    int size;
    int i;
    int brightness = 0;
    struct OutputParam sParams;
    static int firstCycle = 1;

    while(mBacklightPoll) {
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
                case MI_DISP_EVENT_BRIGHTNESS_CLONE: {
                    if (event_resp->base.length - sizeof(struct disp_event_resp) < 2) {
                        DF_LOGE("Invalid Backlight value\n");
                        break;
                    }
                    brightness = (unsigned long)*((int *)(event_resp->data));
                    DF_LOGV("backlight %d", brightness);
                    if (firstCycle) {
                        enableDCDimming(param.dcDimmingEnable);
                        firstCycle = 0;
                    }
                    sendBrightness(brightness);
                    if (brightness != param.brightness) {
#ifndef MTK_PLATFORM
                        FpsMonitor & FM = FpsMonitor::getInstance();
                        FM.checkFps(1);
                        FM.notifyBrightnessUpdate();
#endif
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

int MiBrightness::HandleHDRHBM(int brightness)
{
    int dbv = 0;
    int ret = 0;

    if ((param.hbmStatus >> HBM_NORMAL & 0x1) == 0) {
        ret = HandleHBM(HBM_CMD_ON);
        if (ret != 0) {
            DF_LOGE("failed enable hdr hbm");
        } else {
            param.hbmStatus |= 0x1 << HBM_HDR;
        }
    }

    if (param.hbmType == HBM_TYPE_2047) {
        dbv = param.panelMaxDbv;
    } else {
        dbv = param.panelMaxDbv * 2 + 1;
    }

    if (param.hbmMaxDbv) {
       dbv = dbv > param.hbmMaxDbv ? param.hbmMaxDbv : dbv;
    }

    ret = writeDBVCmd(dbv);
    if (ret != 0) {
        DF_LOGE("failed writeDBVCmd value %d", dbv);
    }
    return ret;
}

int MiBrightness::HandleDCDimming(int brightness)
{
    int ret = 0;
    int dbv = 0;
    Timer t;

    dbv = getDBV(brightness);
    if (brightness <= param.manualMaxBl) {
        if (brightness == param.manualMaxBl && param.hdrStatus) {
            HandleHDRHBM(brightness);
        } else {
            if (brightness == param.manualMaxBl && (param.hbmStatus >> HBM_HDR & 0x1) == 1) {
                mTimerGeneration[TIMER_HBM_OFF]++;
                t.AsyncWait(500, BrightnessTimerCallbackFunction, this,
                         TIMER_HBM_OFF, mTimerGeneration[TIMER_HBM_OFF], 0);
            } else {
                if ((param.hbmStatus >> HBM_NORMAL & 0x1) == 1) {
                    HandleHBM(HBM_CMD_OFF);
                }
            }
            ret = writeDBV(dbv);
        }
    } else if (brightness > param.manualMaxBl) {
        if (!param.hbmEnable) {
            DF_LOGE("Do not support hbm");
            return ret;
        }
        if (param.brightness <= param.manualMaxBl) {
            writeDBV(param.manualMaxBl); // Make sure Qcom dc dimming params clear.
        }
        if ((param.hbmStatus >> HBM_NORMAL & 0x1) == 0) {
            HandleHBM(HBM_CMD_ON);
        }
        if (param.hdrStatus) {
            ret = HandleHDRHBM(brightness);
        } else {
            ret = writeDBVCmd(dbv);
        }
    }
    param.dbv = dbv;
    param.brightness = brightness;

    return ret;
}

int MiBrightness::Set12BitPCCConfig(int brightness)
{
    int i, ret = 0;
    float pccVal = 1.0;
    struct OutputParam sParams;
    int dbv = 0;
    if (param.Mi12BitEnable && brightness < param.Mi12BitThresh) {
        pccVal = getPcc(brightness);
        sParams.function_id = DISPLAY_BL;
        sParams.pcc_payload.push_back(pccVal);
        dbv = getDBV(brightness);
        sParams.param1 = dbv;
        ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
        param.pccVal = pccVal;
    }
    return ret;
}

int MiBrightness::SetPCCConfigDC(int brightness, int dbv)
{
    int i, ret = 0;
    struct OutputParam sParams;
    if (pccDCCoeff.size() != 4) {
        DF_LOGE("pccDCCoeff init failed");
        return -1;
    }
    sParams.function_id = DISPLAY_DC;
    if (getDCMode()) {
        if (dbv >= param.dcDBVThresh) {
            param.dcPccVal = 1;
            sParams.pcc_payload.push_back(param.dcPccVal);
            ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            return ret;
        }

        if (param.Mi12BitEnable) {
            if (brightness == 0) {
                param.dcPccVal = 1.0;
            } else if (brightness < param.pccDCDivision) {
                param.dcPccVal = pccDCCoeff[0] * dbv / param.panelVirtualMax + pccDCCoeff[1];
            } else {
                param.dcPccVal = pccDCCoeff[2] * brightness / param.autoMaxBl + pccDCCoeff[3];
            }
        } else {
            if (brightness > 0 && brightness < param.dcDBVThresh * param.autoMaxBl / param.panelVirtualMax) {
                param.dcPccVal = pccDCCoeff[0] * brightness / param.autoMaxBl + pccDCCoeff[1];
            } else {
                param.dcPccVal = 1.0;
            }
        }
        param.dcPccVal = param.dcPccVal > 1 ? 1 : param.dcPccVal;
    } else {
        param.dcPccVal = 1.0;
    }
    sParams.pcc_payload.push_back(param.dcPccVal);
    ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
    DF_LOGV("brightness %d, pcc coeff %f, dbv = %d, division %d", brightness, param.dcPccVal, dbv, param.pccDCDivision);

    return ret;
}

int MiBrightness::HandleDCBacklight(int brightness, int dbv, int delay)
{
    int ret = 0;
    int hbm = brightness > param.manualMaxBl ? 1 : 0;
    if (param.dcType == DC_TYPE_CRC){
        if (restoreAfterResume)
            return ret;
        if (dbv <= param.dcDBVThresh) {
            if(!hbm){
                mDisplayEffect->processDCBacklight(dbv, 0);
            } else {
                if (param.lastNoZeroBrightness < param.manualMaxBl
                 && param.dbv != 0 && getDBV(param.lastNoZeroBrightness) < param.dcDBVThresh)
                    mDisplayEffect->processDCBacklight(param.dcDBVThresh, 0);
            }
        } else {
            if (param.dbv <= param.dcDBVThresh && param.lastNoZeroBrightness < param.manualMaxBl && mDisplayState != kStateOff) {
                ret = mDisplayEffect->processDCBacklight(dbv, 0);
            }
        }
        if (dbv > param.dcDBVThresh||hbm)
            writeDBV(dbv);
        else
            writeDBV(param.dcDBVThresh);
    } else if (param.dcType == DC_TYPE_PCC) {
        if (!hbm) {
            if ((dbv < param.dcDBVThresh) ||
                (dbv >= param.dcDBVThresh && param.dbv < param.dcDBVThresh && (param.hbmStatus >> HBM_NORMAL & 0x1) == 0)) {
                ret = SetPCCConfigDC(brightness, dbv);
            }
            if (getDCMode() && dbv < param.dcDBVThresh)
                dbv = param.dcDBVThresh;
        } else {
            if ((int)param.dcPccVal != 1) {
                ret = SetPCCConfigDC(brightness, param.dcDBVThresh);
            }
        }
        if (delay)
            usleep(33000);
        writeDBV(dbv);
    } else if (param.dcType == DC_TYPE_PANEL) {
        writeDBV(dbv);
    }
    return ret;
}

int MiBrightness::HandleBrightness(int value)
{
    int ret = 0;
    int dbv = 0;
    float pccVal = 1.0;
    int brightness = 0;
    struct OutputParam sParams;
    Mutex::Autolock autoLock(mBLLock);

    if (value < 0) {
        if (value == UPDATE_AFTER_RESUME) {
            if (param.lastNoZeroBrightness > param.manualMaxBl)
                return 0;
            restoreAfterResume = true;
        }
        brightness = param.lastNoZeroBrightness;
        value = brightness;
    }
    if (value <= param.manualMaxBl) {
        brightness = (int)(value / mCoeff);
        if (brightness > param.manualMaxBl)
            brightness = param.manualMaxBl;
    } else {
        brightness = value;
    }

    if (mDisplayEffect.get()) {
        mDisplayEffect->updateBrightness(brightness);
    } else {
        DF_LOGE("can not get displayEffect!");
        restoreAfterResume = false;
        return ret;
    }
    if (brightness == 0) {
        param.brightness = brightness;
        param.dbv = 0;
        if (param.hbmStatus >> HBM_NORMAL & 0x1)
            HandleHBM(HBM_CMD_OFF);
        restoreAfterResume = false;
        return ret;
    }

    dbv = getDBV(brightness);

    if (param.GabbEnable)  {
        if (value <= param.GabbThresh)
            mDisplayEffect->HandlePccGABB(value);
        else if (param.brightness <= param.GabbThresh && value > param.GabbThresh)
            mDisplayEffect->HandlePccGABB(0);
    }

    if (param.dcDimmingEnable) {
        ret = HandleDCDimming(brightness);
    } else {
        if (brightness <= param.manualMaxBl) {
            if (param.Mi12BitEnable) {
                pccVal = getPcc(brightness);
                if (brightness < param.Mi12BitThresh) {
                    sParams.function_id = DISPLAY_BL;
                    sParams.pcc_payload.push_back(pccVal);
                    sParams.param1 = dbv;
                    ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
                    param.pccVal = pccVal;
                } else if (brightness >= param.Mi12BitThresh &&
                            param.brightness < param.Mi12BitThresh) {
                    sParams.function_id = DISPLAY_BL;
                    sParams.pcc_payload.push_back(pccVal);
                    sParams.param1 = 0xFFFF;
                    ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
                    param.pccVal = pccVal;
                }
            }

            if (brightness == param.manualMaxBl && param.hdrStatus) {
                HandleHDRHBM(brightness);
            } else {
                if ((param.hbmStatus >> HBM_NORMAL & 0x1) == 1) {
                    HandleHBM(HBM_CMD_OFF);
                }
                if (getDCMode()) {
                    ret = HandleDCBacklight(brightness, dbv, 0);
                } else {
                    if (param.silkyBrightnessEnable) {
                        if (brightness >= param.Mi12BitThresh || param.brightness == 0) {
                            writeDBV(dbv);
                        } else {
                            ret = mDisplayEffect->CustomBrightnessProcess(dbv);
                            if (ret) {
                                DF_LOGW("Failed CustomBrightnessProcess");
                            }
                        }
                    } else {
                        writeDBV(dbv);
                    }
                }
            }
        } else if (brightness > param.manualMaxBl) {
            if (!param.hbmEnable) {
                DF_LOGE("Do not support hbm");
                param.lastNoZeroBrightness = param.manualMaxBl;
				restoreAfterResume = false;
                return ret;
            }
            // Clear 12bit pcc value if brightnes change from 12bit zone to hbm.
            if (param.Mi12BitEnable) {
                if (param.brightness < param.Mi12BitThresh) {
                    sParams.function_id = DISPLAY_BL;
                    sParams.param1 = 0xFFFF;
                    sParams.pcc_payload.push_back(pccVal);
                    ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
                    param.pccVal = pccVal;
                }
            }
            if ((param.hbmStatus >> HBM_NORMAL & 0x1) == 0) {
                HandleHBM(HBM_CMD_ON);
            }
            // Clear DC pcc or crc.
            if (getDCMode()) {
                HandleDCBacklight(brightness, dbv, 0);
            }
            if (param.hdrStatus) {
                ret = HandleHDRHBM(brightness);
            } else {
#ifdef MTK_PLATFORM
                ret = writeDBV(dbv);
#else
                ret = writeDBVCmd(dbv);
#endif
            }
        }
    }
    param.dbv = dbv;
    if (value != -1) {
        param.brightness = value;
        if (value != 0)
            param.lastNoZeroBrightness = value;
    }
	restoreAfterResume = false;
    return ret;
}

int MiBrightness::HandleHBM(int enable)
{
    int ret = 0;
    struct OutputParam sParams;

    if (!param.silkyBrightnessEnable && !param.IcDimmingDisable)
        usleep(50000);

    if (enable) {
        param.hbmStatus |= 0x1 << HBM_NORMAL;
    } else {
        param.hbmStatus &= ~(0x1 << HBM_NORMAL);
    }

    if (mDisplayEffect.get()) {
        sParams.function_id = DISP_FEATURE_HBM;
        sParams.param1 = enable;
        ret = mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
        if (mDisplayEffect->getFlatModeStatus() == FEATURE_ON) {
            uint32_t flatMode = enable ? FEATURE_OFF : FEATURE_ON;
            sParams.function_id = DISP_FEATURE_FLAT_MODE;
            sParams.param1 = flatMode;
            ret = mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
        }
    }

    if (!param.silkyBrightnessEnable && !param.IcDimmingDisable)
        usleep(50000);
    return ret;
}

int MiBrightness::getDCMode()
{
    return param.DCBLEnable;
}

void MiBrightness::enableDCDimming(int enable)
{
#ifndef MTK_PLATFORM
    int dispId = qdutils::DISPLAY_PRIMARY;
    if (DISPLAY_SECONDARY == displayId)
        dispId = qdutils::DISPLAY_BUILTIN_2;
    android::sp<qService::IQService> iqservice;
    android::Parcel data;
    data.writeInt32(dispId);
    data.writeInt32(enable);
    DF_LOGD("enable %d", enable);
    iqservice = interface_cast<qService::IQService>(defaultServiceManager()->getService(android::String16("display.qservice")));
    if (iqservice.get()) {
        iqservice->dispatch(qService::IQService::SET_DIMMING_ENABLE, &data, NULL);
    } else {
        DF_LOGW("Failed to acquire %s", "display.qservice");
    }
#endif
}

int MiBrightness::setDimming(int enable)
{
    int ret = 0;
    struct OutputParam sParams;
    sParams.function_id = DISP_FEATURE_DIMMING;
    sParams.param1 = enable ? FEATURE_ON : FEATURE_OFF;
    ret = mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
    return ret;
}

void MiBrightness::setDCMode(int enable)
{
    param.DCBLEnable = enable;
    struct OutputParam sParams;

    sParams.function_id = DISP_FEATURE_DC;
    if (enable) {
        property_set(PERSIST_DISPLAYFEATURE_DC_BL_ENABLE, "true");
        sParams.param1 = FEATURE_ON;
        mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
    } else {
        property_set(PERSIST_DISPLAYFEATURE_DC_BL_ENABLE, "false");
        sParams.param1 = FEATURE_OFF;
        mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);
    }

    if (param.dcType == DC_TYPE_QCOM_DC_DIMMING) {
#ifndef MTK_PLATFORM
        int dispId = qdutils::DISPLAY_PRIMARY;
        int dbv = param.pwmMinDBV;

        if (!enable) {
            setDimming(0);
            usleep(100000);
        }
        if (DISPLAY_SECONDARY == displayId)
            dispId = qdutils::DISPLAY_BUILTIN_2;

        if (enable)
            dbv = param.dcDBVThresh;

        android::sp<qService::IQService> iqservice;
        android::Parcel data;
        data.writeInt32(dispId);
        data.writeInt32(dbv);
        iqservice = interface_cast<qService::IQService>(defaultServiceManager()->getService(android::String16("display.qservice")));
        if (iqservice.get()) {
            iqservice->dispatch(qService::IQService::SET_DIMMING_MIN_BL, &data, NULL);
        } else {
            DF_LOGW("Failed to acquire %s", "display.qservice");
        }
        if (!enable) {
            usleep(100000); // make sure delay enougth time so that qservice finished min bl change.
            setDimming(1);
        }
#endif
    }

    if (param.dcType == DC_TYPE_PCC) {
        setDimming(0);
        usleep(30000);
        HandleDCBacklight(param.brightness, param.dbv, enable);
        usleep(30000);
        setDimming(1);
        sendBrightness(UPDATE_NORMAL);
    }
}

int MiBrightness::getDBV(int brightness)
{
    int dbv = 0;
    int ratio = (param.autoMaxBl - param.manualMaxBl) / (param.panelMaxDbv + 1);
    if (param.dcDimmingEnable) {
        if (brightness > param.manualMaxBl) {
            if (param.hbmEnable) {
                if (param.hbmType == HBM_TYPE_4095)
                    dbv = param.panelMaxDbv + (brightness - param.manualMaxBl) / ratio;
                else if (param.hbmType == HBM_TYPE_2047) {
                    dbv = (brightness - param.manualMaxBl) / ratio;
                    dbv = dbv > param.panelMaxDbv ? param.panelMaxDbv : dbv;
                }
                if (param.hbmMaxDbv)
                    dbv = dbv > param.hbmMaxDbv ? param.hbmMaxDbv : dbv;
            } else {
                dbv = param.panelMaxDbv;
            }
        } else {
            dbv = brightness;
        }
    } else {
        if (brightness > param.manualMaxBl) {
            if (param.hbmEnable) {
                if (param.hbmType == HBM_TYPE_4095)
                    dbv = param.panelMaxDbv + (brightness - param.manualMaxBl) / ratio;
                else if (param.hbmType == HBM_TYPE_2047) {
                    dbv = (brightness - param.manualMaxBl) / ratio;
                    dbv = dbv > param.panelMaxDbv ? param.panelMaxDbv : dbv;
                }
                if (param.hbmMaxDbv)
                    dbv = dbv > param.hbmMaxDbv ? param.hbmMaxDbv : dbv;
            } else {
                dbv = param.panelMaxDbv;
            }
        } else {
            if (param.Mi12BitEnable) {
                if (brightness >= param.Mi12BitThresh && brightness <= param.manualMaxBl) {
                    dbv = param.Mi12BitPanelDBVThresh +
                    (param.panelMaxDbv - param.Mi12BitPanelDBVThresh) *
                    (brightness - param.Mi12BitThresh) / (param.manualMaxBl - param.Mi12BitThresh);
                } else if (brightness > 0 && brightness < param.Mi12BitThresh) {
                    if (!getDCMode()) {
                        dbv = BlLut[brightness];
                    } else {
                        dbv = BlLutDC[brightness];
                    }
                }
            } else {
                dbv = param.minBl + (brightness - param.minBl) * (param.panelMaxDbv - param.minBl) / (param.manualMaxBl - param.minBl);
            }
        }
    }
    DF_LOGV("DisplayId %d, input %d, output %d", displayId, brightness, dbv);
    return dbv;
}

double MiBrightness::getPcc(int brightness)
{
    double value = 1.0;
    if (!param.Mi12BitEnable) {
        return value;
    }
    if (brightness < param.Mi12BitThresh) {
        if (!getDCMode()) {
            value = pccLut[brightness];
        } else {
            value = pccLutDC[brightness];
        }
    }
    return value;
}

int MiBrightness::writeDBVCmd(int value)
{
    struct OutputParam sParams;

    if (restoreAfterResume) {
        DF_LOGD("restore bl after resume, do not write dbv");
        return 0;
    }

    DF_LOGV("setting hbm backlight to %d", value);
    /*set backlight must disable qsync in M2*/
    if(mDisplayEffect && mDisplayEffect->mDisplayConfigIntf) {
        mDisplayEffect->SetQsyncTimer(displayId);
    }

    sParams.function_id = DISP_FEATURE_HBM_BACKLIGHT;
    sParams.param1 = value;
    mDisplayEffect->setDisplayEffect(SET_PANEL_MODE, &sParams);

    return 0;
}

int MiBrightness::writeDBV(int value)
{
    int fd = -1;
    int ret = -1;
    string path;
    char BLValue[10];

    if (restoreAfterResume && (param.silkyBrightnessEnable == 0)) {
        DF_LOGD("restore bl after resume, do not write dbv");
        return 0;
    }
    ret = mDisplayEffect->CustomBrightnessProcess(value);
    if (ret) {
        DF_LOGW("Failed CustomBrightnessProcess");
    }
    snprintf(BLValue, sizeof(BLValue), "%d", value);
    path = displayId ? SECONDARY_BACKLIGHT_PATH : PRIMARY_BACKLIGHT_PATH;

    DF_LOGV("setting backlight to %d", value);

    /*set backlight must disable qsync in M2*/
    if(mDisplayEffect && mDisplayEffect->mDisplayConfigIntf) {
        mDisplayEffect->SetQsyncTimer(displayId);
    }

    fd = open(path.c_str(), O_RDWR);
    if (fd < 0) {
        DF_LOGE("open backlight node error: %s", path.c_str());
        return ret;
    }

    ret = write(fd, (void*)BLValue, strlen(BLValue));
    if (ret < 0) {
        DF_LOGE("write (%s) value %d failed ret=%d!", path.c_str(), value, ret);
        goto FAILED;
    }

    DF_LOGV("successfully write %s into %s", BLValue, path.c_str());

FAILED:
    close(fd);
    fd = -1;
    return ret;
}

MiBrightnessParam MiBrightness::getBrightnessParam()
{
    return param;
}

void MiBrightness::SetDisplayState(int State)
{
    int dbv = 0;
    float pccVal = 1.0;
    struct OutputParam sParams;

    if (mDisplayState != kStateOn && State == kStateOn) {
        if (param.silkyBrightnessEnable == 0)
            sendBrightness(UPDATE_AFTER_RESUME);
    } else if (mDisplayState == kStateOn && State == kStateOff) {
        // Clear 12bit pcc value after kStateOff.

        if (param.Mi12BitEnable) {
            if (param.brightness < param.Mi12BitThresh) {
                sParams.function_id = DISPLAY_BL;
                sParams.param1 = -1;
                sParams.pcc_payload.push_back(pccVal);
                mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
                param.pccVal = pccVal;
            }
        }
        if (getDCMode() && param.dcType == DC_TYPE_PCC)
            SetPCCConfigDC(0, 0);
    }
    mDisplayState = State;
}

int MiBrightness::getLastNonZeroBl()
{
    Mutex::Autolock autoLock(mBLLock);
    return param.lastNoZeroBrightness;
}

void MiBrightness::dump(std::string& result)
{
    char res_ch[1024] = {0};
    result.append("\nBrightness:\n");
    snprintf(res_ch, sizeof(res_ch), "Disp ID: %d, Bri: %d, Non0Bri: %d, DBV: %d, Coeff: %f, HBM: %d, HDR: %d, DC: %d, Pcc: %f, DCPcc: %f\n",
                             displayId, param.brightness, param.lastNoZeroBrightness, param.dbv, mCoeff,
                             param.hbmStatus, param.hdrStatus, param.DCBLEnable, param.pccVal, param.dcPccVal);
    result.append(std::string(res_ch));
}

} //namespace android
