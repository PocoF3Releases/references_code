/*
 * Copyright (C) 2013 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/Singleton.h>
#include <utils/SortedVector.h>
#include <utils/String8.h>
#include <sys/prctl.h>
#include <errno.h>
#include <assert.h>
#include <sys/poll.h>

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <cutils/properties.h>
#include <fstream>

#include <display_property.h>
#include <DisplayFeatureHal.h>
#include <DisplayPostprocessing.h>
#include <DisplayColorManager_hidl.h>
#include <ParamManager2.h>
#include <media/stagefright/foundation/ADebug.h>
//#include <gui/ISurfaceComposer.h>
//#include <private/gui/ComposerService.h>

#include "DisplayEffectBase.h"
#include "cust_define.h"
#include "XmlParser.h"
#include "timer.hpp"
#ifdef HAS_QDCM
#include <QServiceUtils.h>
#endif

#define LOG_TAG "DisplayFeatureHal"

#define MIN(a,b) (((a)<(b))?(a):(b))

using std::fstream;

namespace android {

char * printFeatureMode(int fmode)
{
    switch (fmode) {
        case SET_AD:
            return "SET_AD";
        case SET_COLOR_MODE:
            return "SET_COLOR_MODE";
        case SET_PCC_MODE:
            return "SET_PCC_MODE";
        case SET_PANEL_MODE:
            return "SET_PANEL_MODE";
        case SET_SVI:
            return "SET_SVI";
        case SET_FOSS:
            return "SET_FOSS";
        case SET_DSPP_MODE:
            return "SET_DSPP_MODE";
        case SET_LTM:
            return "SET_LTM";
        case SET_MTK_MODE:
            return "SET_MTK_MODE";
    }
    return NULL;
}

// ---------------------------------------------------------------------------

void DEtimerCallbackFunc(void *user, int caseId ,int generation, int value)
{
    DisplayEffectBase *source = (DisplayEffectBase *) user;
    source->timerCallback(caseId, generation, value);
}

static void DFSensorCallbackFunction(int event, const Event &info, void *user) {
    DisplayEffectBase *source = (DisplayEffectBase *) user;
    if (source) {
        source->sensorCallback(event, info);
    }
}

DisplayEffectBase::DisplayEffectBase(unsigned int displayId)
    :mDisplayId(displayId)
{
    DF_LOGV("DisplayEffectBase");

    mStates[0] = NULL;

    mStates[SET_PANEL_MODE] = new PanelMode(displayId);
    mStates[SET_PANEL_MODE]->init(true);

    mStates[SET_COLOR_MODE] = new ColorMode(displayId);
    mStates[SET_COLOR_MODE]->init(true);

    mStates[SET_PCC_MODE] = new PccMode(displayId);
    mStates[SET_PCC_MODE]->init(true);

    mStates[SET_DSPP_MODE]= new DSPPMode(displayId);
    mStates[SET_DSPP_MODE]->init(true);

    mStates[SET_LTM] = new LtmMode(displayId);
    mStates[SET_LTM]->init(true);

    mStates[SET_MTK_MODE] = new MTKMode(displayId);
    mStates[SET_MTK_MODE]->init(true);

    mEffectStatus = 0;
    memset(&mCurParam, 0x0, sizeof(mCurParam));
    mCurParam.expertParam[1] = 255;
    mCurParam.expertParam[2] = 255;
    mCurParam.expertParam[3] = 255;
    mCurParam.expertParam[8] = 220;

    mHistControl = property_get_bool(RO_DISPLAYFEATURE_HISTOGRAM_ENABLE, false);
    mBacklightPoll = property_get_bool(RO_XIAOMI_BL_POLL, false);
    mSRESupported = property_get_bool(RO_VENDOR_SRE_ENABLE, false);
    mLocalHBMEnabled = property_get_bool(RO_VENDOR_LOCALHBM_ENABLE, false);
    mFod110nitLux = property_get_int32(RO_VENDOR_FOD_110NIT_LUX_LEVEL, 3);
    DF_LOGD("DisplayEffectBase, mFod110nitLux:%d", mFod110nitLux);

    mCTDimming = true;
    mWCGState = false;

    mInteracted_bl = 0;
    maxBLLevel = MAX_BL_LEVEL;

    mSuspendFlag = 0;
    mCancelRestore = 0;
    mDisplayState = kStateOn;
    mSuspendFlag = 0;

    mPccParams.uctParam.strength = COLOR_STRENGTH;
    for (int i = 0; i < 3; i++) {
        mPccParams.uctParam.def[i] = COLOR_DEFAULT >> (8 * (2 - i)) & 0xFF;
        mPccParams.uctParam.cold[i] = COLOR_COLD >> (8 * (2 - i)) & 0xFF;
        mPccParams.uctParam.warm[i] = COLOR_WARM >> (8 * (2 - i)) & 0xFF;
    }
    mPccParams.uctParam.uiParamMin = UIPARAM_MIN;
    mPccParams.uctParam.paramMax = PARAM_MAX;
    mPccParams.uctParam.dimmingTime = DIMMING_TIME;
    mPccParams.uctParam.dimmingStep = DIMMING_STEP;

    mPccParams.ccbbParam.xCoeff[0] = A1_COEFF;
    mPccParams.ccbbParam.xCoeff[1] = B1_COEFF;
    mPccParams.ccbbParam.xCoeff[2] = C1_COEFF;
    mPccParams.ccbbParam.yCoeff[0] = A2_COEFF;
    mPccParams.ccbbParam.yCoeff[1] = B2_COEFF;
    mPccParams.ccbbParam.yCoeff[2] = C2_COEFF;
    mPccParams.ccbbParam.deltaX = DELTA_X;
    mPccParams.ccbbParam.deltaY = DELTA_Y;
    mPccParams.ccbbParam.blThreshold[0] = BL_THRESHOLD_L;
    mPccParams.ccbbParam.blThreshold[1] = BL_THRESHOLD_H;

    mPccParams.wpCalibParam.targetWP[0] = TARGET_WP_x;
    mPccParams.wpCalibParam.targetWP[1] = TARGET_WP_y;
    mPccParams.wpCalibParam.targetXY[0] = TARGET_WP_X;
    mPccParams.wpCalibParam.targetXY[1] = TARGET_WP_Y;
    mPccParams.wpCalibParam.targetXY[2] = TARGET_WP_Z;
    mPccParams.wpCalibParam.rgbCoodinatexyl[0][0] = Rx;
    mPccParams.wpCalibParam.rgbCoodinatexyl[0][1] = Gx;
    mPccParams.wpCalibParam.rgbCoodinatexyl[0][2] = Bx;
    mPccParams.wpCalibParam.rgbCoodinatexyl[1][0] = Ry;
    mPccParams.wpCalibParam.rgbCoodinatexyl[1][1] = Gy;
    mPccParams.wpCalibParam.rgbCoodinatexyl[1][2] = By;
    mPccParams.wpCalibParam.rgbCoodinatexyl[2][0] = 0;
    mPccParams.wpCalibParam.rgbCoodinatexyl[2][1] = 0;
    mPccParams.wpCalibParam.rgbCoodinatexyl[2][2] = 0;
    mPccParams.wpCalibParam.xRange[0] = X_RANGE_MIN;
    mPccParams.wpCalibParam.xRange[1] = X_RANGE_MAX;
    mPccParams.wpCalibParam.yRange[0] = Y_RANGE_MIN;
    mPccParams.wpCalibParam.yRange[1] = Y_RANGE_MAX;
    mPccParams.wpCalibParam.xyComp[0] = X_COMP;
    mPccParams.wpCalibParam.xyComp[1] = Y_COMP;
    mPccParams.wpCalibParam.xyOffset[0] = X_OFFSET;
    mPccParams.wpCalibParam.xyOffset[1] = Y_OFFSET;
    mPccParams.slsParams.level = SUNLIGHT_SCREEN_LEVEL_MAX;
    mPccParams.slsParams.delay = SUNLIGHT_ADJUST_DELAY;
    memcpy(mPccParams.slsParams.coeff ,mSLScreenParams, sizeof(mPccParams.slsParams.coeff));
    memcpy(mPccParams.slsParams.luxThreshold, mSunlightLuxLevel, sizeof(mPccParams.slsParams.luxThreshold));
    mPccParams.DCBLParams.minBL = MIN_BL;
    mPccParams.DCBLParams.threshold = DC_BL_THRESHOLD;
    mPccParams.DCBLParams.startPcc = DC_BL_START_PCC;
    mPccParams.DCBLParams.step = DC_BL_PCC_STEP;
    mDCBLCoeff[0] = DC_BL_COEFF0;
    mDCBLCoeff[1] = DC_BL_COEFF1;
    memcpy(mPccParams.softBlParams.pccLut, mBlPccLut, sizeof(mPccParams.softBlParams.pccLut));
    memcpy(mPccParams.softBlParams.pccLutDC, mBlPccLutDC, sizeof(mPccParams.softBlParams.pccLutDC));

    //memcpy(mGABBParams.luxTable, mGABBLuxTable, sizeof(mGABBParams.luxTable));
    //memcpy(mGABBParams.regTable, mGABBRegTable, sizeof(mGABBParams.regTable));
    //memcpy(mGABBParams.cmd, DCBLCrcCmd, sizeof(mGABBParams.cmd));
    memcpy(mPccParams.GABBParams.GABBCoeff, mPccGABBParams, sizeof(mPccParams.GABBParams.GABBCoeff));
    mPccParams.GABBParams.threshold = PCC_GABB_THRESHOLD;

    setParams(SET_PCC_MODE,(void *)&mPccParams);

    mTypBrightness = TYP_BRIGHTNESS;
    mStartBrightness = START_BRIGHTNESS;
    mBrightnessStep = BRIGHTNESS_STEP;
    mReadMaxBrightnessEnable = READ_MAX_BRIGHTNESS_ENABLE;
    DF_LOGD("DisplayEffectBase, hist:%d", mHistControl);
    mSoftBlMaxLevel = SOFT_BL_MAX;
    mSoftBlThreshold = SOFT_BL_THRESH;
    mNativeBlMaxLevel = MAX_BL_LEVEL;
    mNativeBlThreshold = NATIVE_BL_THRESH;
#ifdef HAS_QDCM
    int ret = ClientInterface::Create("displayfeature_client", NULL, &mDisplayConfigIntf);
    if (ret) {
      DF_LOGE("DisplayConfig HIDL not present\n");
      mDisplayConfigIntf = NULL;
    }
#endif
    mDisplayDFPS = new DisplayFramerateSwitchThread("df_dfps", mDisplayId);
    mTrueTone = new TrueTone(this);
    mVideoMode= new VideoMode(this);
    mPaperMode = new PaperMode(this);
    mBrightness = new MiBrightness(this);
    mColorTempMode = new ColorTempMode(this);
    mFlatMode = new FlatMode(this);
#ifdef HAS_QDCM
    mDisplaySRE = new Sre(this);
    mDisplayBCBC = new DisplayBCBC(this);
    mDisplayVSDR2HDR = new DisplayVSDR2HDR(this);
    mDisplayIGCDither = new Dither(this);
    mDisplayCount = new DisplayCountThread(mDisplayId);
    if (mDisplayIGCDither.get()) {
        mDitherStrength = mDisplayIGCDither->getDefaultStrength();
    }
#endif

#ifdef CONFIG_FOSS_FEATURE_ENABLE
    mDisplayFoss = new Foss(this);
    if(!mDisplayFoss->getFossSupported()) {
        mDisplayFoss = NULL;
    }
#endif
}

DisplayEffectBase::~DisplayEffectBase()
{
    DF_LOGV("DisplayEffectBase destroy");
    if (mBacklightPoll) {
        pthread_join(mThreadBacklight, NULL);
        mThreadBacklight = 0;
        for (int i = 0; i < POLL_FD_NUM; i++) {
            close(mPollBlFd[i].fd);
            mPollBlFd[i].fd = -1;
        }
        mBacklightPoll = 0;
    }

    if (mLooper != NULL) {
        mLooper->unregisterHandler(id());
        mLooper->stop();
    }
#ifdef HAS_QDCM
    if (mDisplayConfigIntf) {
        ClientInterface::Destroy(mDisplayConfigIntf);
    }
#endif
    if (!mDisplayId && mFoldSensorStarted) {
        SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
        mSensorCtrl.sensorControlOP(mDisplayId, SENSOR_STOP, SENSOR_FOLD, FOLD);
        mFoldSensorStarted = false;
    }
}

void DisplayEffectBase::onCallback(int cmd, int value)
{
    DisplayUtil::onCallback(cmd, value);
}

int DisplayEffectBase::registerPollEvent()
{
    int ret = 0;
#ifdef MTK_PLATFORM
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
    event_req.base.disp_id = DISPLAY_PRIMARY;
    event_req.type = MI_DISP_EVENT_PANEL_DEAD;

    ALOGD("event type name: %s\n", getDispEventTypeName(event_req.type));
    ret = ioctl(mDFPollFd.fd, MI_DISP_IOCTL_REGISTER_EVENT, &event_req);
    if(ret) {
        ALOGE("ioctl MI_DISP_IOCTL_REGISTER_EVENT fail\n");
        return -1;
    }
#endif
    return 0;
}

void DisplayEffectBase::start()
{
    if (mLooper == NULL) {
        mLooper = new ALooper;
        mLooper->setName("displayeffectBase");
        mLooper->start();

        mLooper->registerHandler(this);
    }

    mFodSensorMonitor = new FodSensorMonitor();
    mFodSensorMonitor->start(this);
    mFodSensorMonitor->switchState(false);
    if (mBrightness.get())
        mBrightness->init();
#ifdef HAS_QDCM
    FpsMonitor & FM = FpsMonitor::getInstance();
    FM.registerCallback((void*)lockFpsRequest);
#endif
#ifdef MTK_PLATFORM
    registerPollEvent();
    pthread_create(&mThreadPanelMonitor, NULL, ThreadWrapperPanel, this);
#endif
}

void *DisplayEffectBase::ThreadWrapperPanel(void *context) {
    if (context) {
        return (void *)(uintptr_t)static_cast<DisplayEffectBase *>(context)->threadFuncPanelMonitor();
    }
    return NULL;
}

int DisplayEffectBase::threadFuncPanelMonitor() {
#ifdef MTK_PLATFORM
    char event_data[1024] = {0};
    int ret = 0;
    struct disp_event_resp *event_resp = NULL;
    //int *event_payload;
    int use_count = 0;
    int size;
    int i;
    unsigned long brightness = 0;
    unsigned long virtualBrightness = 0;
    int generation = 0;
    char BLValue[10];
    struct OutputParam sParams;
    struct DispParam Param;
    int DCEnable = 0;

    while (1) {
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
                case MI_DISP_EVENT_PANEL_DEAD: {
                    mPanelDead = *(event_resp->data);
                    DF_LOGD("recive panel dead event:%d mDisplayState %d\n", mPanelDead, mDisplayState);
                    if (mPanelDead == 0 && mDisplayState == kStateOn) {
                        if (mBrightness.get()) {
                            DCEnable = mBrightness->getDCMode();
                        }
                        restoreQcomMode();
                        if (DCEnable) {
                            sParams.function_id = DISP_FEATURE_DC;
                            sParams.param1 = FEATURE_ON;
                            ret = setDisplayEffect(SET_PANEL_MODE, &sParams);
                        }
                    }
                } break;
                default:
                    break;
                }
            i += event_resp->base.length;
            }
        }
    }
#endif
    pthread_exit(0);
    return 0;
}

void DisplayEffectBase::sensorCallback(int event, const Event &info)
{
    if (event != SENSOR_TYPE_FOLD_STATUS)
        return;
#ifdef HAS_QDCM
    ALOGD("fold status:%9.4f," "time=%ld, sensor=%d", info.u.data[0], info.timestamp, info.sensorType);
    DisplayUtil::onCallback(SET_FOLD_STATE, (int)info.u.data[0]);
    notifyQservice(qService::IQService::SET_FOLD_STATUS_UPDATE, (int)info.u.data[0]);
#endif
}

void DisplayEffectBase::lockFpsRequest(int value)
{
    DisplayUtil::onCallback(LOCK_FPS_REQUEST, value);
}

int DisplayEffectBase::timerCallback(int caseId, int generation, int value)
{
    DF_LOGV("displayId %d, caseId %d, generation %d, mTimerGeneration %d value %d",
         mDisplayId,  caseId, generation, mTimerGeneration[caseId], value);
    int ret = 0;
    struct OutputParam sParams;
    int correctedBl = 0, correctedVirtualBl = 0;

    if (generation != mTimerGeneration[caseId])
        return 0;
    switch (caseId) {
        case TIMER_IGC_DITHER: {
#ifdef HAS_QDCM
            if (mDisplayIGCDither.get()) {
                mDisplayIGCDither->setIGCDither(value, mDitherStrength);
            }
#endif
        } break;
        default:
            break;
    }

    return ret;
}

int DisplayEffectBase::featureEnablePolicy(int featureId, struct OutputParam *pSParams)
{
    int ret = -1;
    sp<DisplayFeatureState> pState;

    if (featureId >= SET_MAX_FEATURE_CNT || !pSParams) {
        DF_LOGE("featureId(%d) is illegal, invalid param", featureId);
        return ret;
    }

    pState = mStates[featureId];
    if (pState != NULL && pState->isSupport()) {
        pState->setValue(pSParams);
        ret = pState->output();
        if (DISPLAY_FEATURE_HIST == pSParams->function_id
            && (SRE_FEATURE == pSParams->param2 || GAME_SRE_FEATURE == pSParams->param2 ||
                    VSDR2HDR_FEATURE == pSParams->param2 || IGC_DITHER == pSParams->param2)) {
            pSParams->len = ret;
            ret = 0;
        }
    } else {
        DF_LOGE("---error not support this feature current!");
    }

    return ret;
}

int DisplayEffectBase::setParams(int featureId, void* params)
{
    sp<DisplayFeatureState> pState;
    if (featureId >= SET_MAX_FEATURE_CNT) {
        DF_LOGE("featureId(%d) is illegal", featureId);
        return -1;
    }

    pState = mStates[featureId];

    if (pState != NULL && params != NULL) {
        pState->setParam(params);
    }
    return 0;
}

int DisplayEffectBase::setBreakCTDimming(int value)
{
    int ret = -1;
    struct OutputParam sParams;

    sParams.function_id = DISPLAY_BREAK_CT_DIMMING;
    sParams.param1 = value;
    ret = setDisplayEffect(SET_PCC_MODE, &sParams);

    return ret;
}

int DisplayEffectBase::setBreakSLPCCDimming(int value)
{
    int ret = -1;
    struct OutputParam sParams;

    sParams.function_id = DISPLAY_BREAK_SLPCC_DIMMING;
    sParams.param1 = value;
    ret = setDisplayEffect(SET_PCC_MODE, &sParams);

    return ret;
}

int DisplayEffectBase::setDisplayEffect(int fMode, struct OutputParam *pSParams)
{
    int ret = -1;
    Mutex::Autolock autoLock(mSetEffectLock);
    if (!pSParams) {
        DF_LOGE("invalid param");
        return ret;
    }

    DF_LOGV("displayId = %d", mDisplayId);

    if (SET_LTM == fMode && pSParams->function_id == LTM_ON
        && mDisplayState != kStateOn && mDisplayState != -1) {
        DF_LOGW("skip setting LTM/SRE on");
        pSParams->function_id = 0;
        if (mSRESupported)
            return 0;
    }

    if (SET_COLOR_MODE == fMode) {
        if (mDisplayState == kStateOn) {
            mFlatMode->setFlatMode(fMode, pSParams);
#ifdef HAS_QDCM
            if (mDisplaySRE.get())
                mDisplaySRE->setDisplayEffect(pSParams->function_id);
#endif
        }

        setDisplayEffectCustom(pSParams);
        struct OutputParam sParams;
        if (pSParams->function_id != DISPLAY_COLOR_MODE_sRGB &&
            pSParams->function_id != DISPLAY_COLOR_MODE_EXPERT_WCG &&
            pSParams->function_id != DISPLAY_COLOR_MODE_DV_FLAT_OFF &&
            pSParams->function_id != DISPLAY_COLOR_MODE_DV_FLAT_ON &&
            GetVideoModeId() == 0 && mSceneId == -1) {
            sParams.function_id = COLOR_LUT;
            sParams.param1 = 0;
            sParams.param2 = 0;
            sParams.vec_payload.push_back(1);
            ret = featureEnablePolicy(SET_DSPP_MODE, &sParams);
        }
    }

    if (mSRESupported && SET_LTM == fMode) {
        sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_SRE, this);
        msg->setInt32("modeId", pSParams->function_id);
        msg->setInt32("value", pSParams->param1);
        msg->post();
        return ret;
    }

    ret = featureEnablePolicy(fMode, pSParams);
    if (ret != 0) {
        DF_LOGE("%s fail value: 0x%x", printFeatureMode(fMode), pSParams->function_id);
        setEffectStatus(0);
    }

    return ret;
}

int DisplayEffectBase::getEyecareParams(unsigned int level)
{
    int eyecare_params = -1;
    return eyecare_params;
}

int DisplayEffectBase::getEffectStatus()
{
    Mutex::Autolock autoLock(mStatusLock);
    return mEffectStatus;
}

int DisplayEffectBase::getEffectStatus(int index)
{
    Mutex::Autolock autoLock(mStatusLock);
    return (mEffectStatus>>index)&0x1;
}

void DisplayEffectBase::setEffectStatus(int status)
{
    Mutex::Autolock autoLock(mStatusLock);
    /* keep eyecase status when set other status
       as eyecare can stay with other modes now.*/
    if ((mEffectStatus >> DISPLAY_EYECARE) & 0x1)
        mEffectStatus = status | (0x1 << DISPLAY_EYECARE);
    else
        mEffectStatus = status;
    if (mTrueTone.get())
        mTrueTone->updateTrueToneStatus(mEffectStatus);
    DF_LOGV("0x%x", mEffectStatus);
}

void DisplayEffectBase::_setEffectStatus(int status)
{
    Mutex::Autolock autoLock(mStatusLock);
    mEffectStatus |= status;
    DF_LOGV("0x%x", mEffectStatus);
}

void DisplayEffectBase::clearEyeCareStatus()
{
    Mutex::Autolock autoLock(mStatusLock);
    mEffectStatus &= ~(0x1 << DISPLAY_EYECARE);
    DF_LOGV("0x%x", mEffectStatus);
}

int DisplayEffectBase::getEffectFirstId(int status)
{
    int temp = 0;
    int bits = 0;

    temp = status;
    while(temp) {
        bits++;
        if(temp & 0x1) {
            break;
        } else {
            temp = temp>>1;
        }
    }
    return bits;
}

int DisplayEffectBase::restorePanelEffect()
{
    int ret = -1;
    return ret;
}

void DisplayEffectBase::getDispParam(struct DispParam *pParam)
{
    Mutex::Autolock autoLock(mStatusLock);
    *pParam = mCurParam;
}

void DisplayEffectBase::setDispParam(int displayId, int caseId, int modeId, int cookie)
{
    Mutex::Autolock autoLock(mStatusLock);
    if (caseId == EYECARE_STATE) {
        mCurParam.eyeCareLevel = modeId;
    } else if (caseId == COLOR_TEMP_STATE) {
        mCurParam.CTLevel = modeId;
    } else if (caseId == EXPERT_STATE) {
        if (!InputParamValidCheck(cookie, modeId)) {
            DF_LOGE("Invalid expert data");
            return;
        }
        if (cookie == EXPERT_CLEAR) {
            mCurParam.expertParam[0] = mDefaultGamut;
            mCurParam.expertParam[1] = 255;
            mCurParam.expertParam[2] = 255;
            mCurParam.expertParam[3] = 255;
            mCurParam.expertParam[4] = 0;
            mCurParam.expertParam[5] = 0;
            mCurParam.expertParam[6] = 0;
            mCurParam.expertParam[7] = 0;
            mCurParam.expertParam[8] = 220;
        } else {
            mCurParam.expertParam[cookie] = modeId;
            mCurParam.caseId = caseId;
        }
    } else {
        mCurParam.displayId = displayId;
        mCurParam.caseId = caseId;
        mCurParam.modeId = modeId;
        mCurParam.cookie = cookie;
    }

}

void DisplayEffectBase::setWCGState(bool enable)
{
    DF_LOGD("current wcg state %d, setting to %d", mWCGState, enable);
    mWCGState = enable;
}

void DisplayEffectBase::setBrightness(int value)
{
    mInteracted_bl = value;
}

void DisplayEffectBase::updateBacklightCoeff(float coeff)
{
    if (mBrightness.get()) {
        mBrightness->updateBlCoeff(coeff);
    }
}

int DisplayEffectBase::handleCCBB(int brightness, int fMode)
{
    int ret = -1;
    struct OutputParam sParams;

    DF_LOGV("enter");
    int enable = getEffectStatus(DISPLAY_ENV_ADAPTIVE_NORMAL);
    if(enable && brightness > 0) {
        sParams.function_id = DISPLAY_FEATURE_CCBB;
        sParams.param1 = brightness;
        ret = setDisplayEffect(fMode, &sParams);
    }

    DF_LOGV("exit ret %d enable %d", ret, enable);
    return ret;
}

int DisplayEffectBase::handleDispSwitchOption(int modeId, int cookie)
{
    int ret = 0;
    switch (modeId) {
        case COLORTEMPERATURE_ADJUST_STATE:
            SensorControl::sensor_log = cookie;
            DF_LOGD("sensor_log %d", SensorControl::sensor_log);
            break;
        case AUTOMATIC_CONTRAST_STATE: {
            DFLOG::loglevel = cookie;
            DF_LOGD("Setting Display Feature log level %d", DFLOG::loglevel);
        } break;
        case DC_BACKLIGHT_STATE: {
            if (cookie == 0) {
                DisplayUtil::onCallback(SET_DC_PARSE_STATE, 0);
                DisplayUtil::setDCParseCallbackEnable(false);
            }
            else if (cookie == 1) {
                DisplayUtil::setDCParseCallbackEnable(true);
                DisplayUtil::onCallback(SET_DC_PARSE_STATE, 1);
            }
        } break;
        default:
            ret = -1;
    }
    return ret;
}

int DisplayEffectBase::notifyQservice(int cmd, int value)
{
    int ret = 0;
#ifndef MTK_PLATFORM
    sp<qService::IQService> iqservice;
    android::Parcel data;
    data.writeInt32(value);

    iqservice = interface_cast<qService::IQService>(defaultServiceManager()->getService(String16("display.qservice")));
    if (iqservice.get()) {
        ret = iqservice->dispatch(cmd, &data, NULL);
    } else {
        DF_LOGW("Failed to acquire %s", "display.qservice");
        ret = -1;
    }
#endif
    DF_LOGD("cmd %d value %d ret %d", cmd, value, ret);
    return ret;
}

bool DisplayEffectBase::GetHdrStatus()
{
    Mutex::Autolock autoLock(mHDRLock);
    return mHDRStatus;
}

int DisplayEffectBase::GetMessageGeneration(int index)
{
    return mMessageGeneration[index];
}

void DisplayEffectBase::SetMessageGeneration(int index, int gen)
{
    mMessageGeneration[index] = gen;
}

//GABB: Gamut Adjustment by backlight
int DisplayEffectBase::CalculateGABBParms(int brightness)
{
    int ret = -1;

    for (int i = 0; i < GABB_LUX_TABLE_LEN; i++) {
        if (brightness <= mGABBParams.luxTable[i])
            return i;
    }

    return GABB_LUX_TABLE_LEN - 1;
}

int DisplayEffectBase::HandleGABB(int brightness)
{
    int ret = -1;
    int GABBLevel = GABB_LUX_TABLE_LEN - 1;
    char writeString[100] = "";
    char cmd_head[32] = "00 00 39 00 00 00 00 00 16 B1";
    static char lastWriteString[100] = "";
    char gabb_cmd[300] = "";
    int x1, x2, y1, y2, x, y;
    int len = 0;
    int size = 0;

    DF_LOGV("enter brightness %d", brightness);
    GABBLevel = CalculateGABBParms(brightness);
    len = snprintf(writeString, strlen(cmd_head) + 1, "%s", cmd_head);
    if (len < 0) {
        DF_LOGE("error print!");
        return ret;
    }

    for (int i = 0; i < GABB_PARAM_LEN; i++) {
        if (GABBLevel == 0) {
            size = sizeof(writeString) - len;
            if (size > 0) {
                len += snprintf(writeString + len, size, " %02X", mGABBParams.regTable[GABBLevel][i]);
                if (len < 0) {
                    DF_LOGE("error print!");
                    return ret;
                }
            }
        } else {
            x1 = mGABBParams.luxTable[GABBLevel - 1];
            x2 = mGABBParams.luxTable[GABBLevel];
            y1 = mGABBParams.regTable[GABBLevel - 1][i];
            y2 = mGABBParams.regTable[GABBLevel][i];
            x = brightness;
            /*
                point (x, y) is in line ((x1, y1), (x2, y2)),
                so slope of((x1, y1), (x, y)) and ((x1, y1), (x2, y2)) are equal,
                (y - y1) / (x - x1) = (y2 - y1) / (x2 - x1)
            */
            y = y1 + (y2 - y1) * (x - x1) / (x2 - x1);

            size = sizeof(writeString) - len;
                if (size > 0) {
                len += snprintf(writeString + len, size, " %02X", y);
                if (len < 0) {
                    DF_LOGE("error print!");
                    return -1;
                }
            }
        }
    }
    if (strcmp(lastWriteString, writeString)) {
        memcpy(lastWriteString, writeString, sizeof(lastWriteString));

        strcpy(gabb_cmd, mGABBParams.cmd[0]);
        strcat(gabb_cmd, mGABBParams.cmd[1] + 5);
        strcat(gabb_cmd, mGABBParams.cmd[2] + 5);
        strcat(gabb_cmd, mGABBParams.cmd[3] + 5);
        strcat(gabb_cmd, writeString + 5);
        strcat(gabb_cmd, mGABBParams.cmd[4] + 5);
        ret = DisplayUtil::writePanelDsiCmds(gabb_cmd);
        DF_LOGV("gabb_cmd %s", gabb_cmd);
    }

    DF_LOGV("exit ret %d", ret);
    return ret;
}

int DisplayEffectBase::HandlePccGABB(int brightness)
{
    int ret = -1;
    struct OutputParam sParams;

    DF_LOGV("enter brightness %d", brightness);

    sParams.function_id = DISPLAY_GABB;
    sParams.param1 = brightness;
    ret = setDisplayEffect(SET_PCC_MODE, &sParams);

    DF_LOGV("exit ret %d", ret);
    return ret;
}

int DisplayEffectBase::HandleACNormal(int modeId, int cookie)
{
    int ret = -1;
    int luminous = -1, status = 0;
    struct OutputParam sParams;

    DF_LOGD("enter");

    sParams.function_id = DISPLAY_COLOR_MODE_ACNORMAL;
    ret = setDisplayEffect(SET_COLOR_MODE, &sParams);

    status = 0x1 << DISPLAY_ENV_ADAPTIVE_NORMAL;
    setEffectStatus(status);
    setDispParam(0, AUTOMATIC_CONTRAST_STATE, modeId, cookie);

    DF_LOGV("exit ret %d", ret);
    return ret;
}

int DisplayEffectBase::HandleACCold(int cookie)
{
    int ret = -1;
    return ret;
}

int DisplayEffectBase::HandleACWarm(int cookie)
{
    int ret = -1;
    return ret;
}

int DisplayEffectBase::HandleIncreasedContrast(int cookie)
{
    int ret = -1;
    struct OutputParam sParams;

    DF_LOGD("enter");
    sParams.function_id = DISPLAY_COLOR_MODE_NATURE;
    ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
    setEffectStatus(0x1 << DISPLAY_COLOR_ENHANCE);
    setDispParam(0, INCREASED_CONTRAST_STATE, 0, cookie);

    DF_LOGV("exit");
    return ret;
}

int DisplayEffectBase::HandleStandard(int cookie)
{
    int ret = -1;
    struct OutputParam sParams;
    DF_LOGD("enter");

    sParams.function_id = DISPLAY_COLOR_MODE_sRGB;
#ifdef MTK_PLATFORM
    if (mWCGState && mWCGCorlorGamut == DATASPACE_P3)
        sParams.function_id = DISPLAY_COLOR_MODE_P3;
#endif
    ret = setDisplayEffect(SET_COLOR_MODE, &sParams);

    setEffectStatus(0x1 << DISPLAY_STANDARD);
    setDispParam(0, STANDARD_STATE, 0, cookie);

    DF_LOGV("exit");
    return ret;
}

void DisplayEffectBase::setDisplayEffectCustom(struct OutputParam *pSParams)
{
    return ;
}

void DisplayEffectBase::SetCancelRestore(int value)
{
    mCancelRestore = value;
}

int DisplayEffectBase::SetEyeCareToStc(int modeId, int paperModeId)
{
    int ret = 0;
#ifdef HAS_QDCM
    sp<IMiStcService> imistcservice;
    android::Parcel data;
    int enable = 0;

    enable = modeId > 0 ? 1 : 0;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(enable);
    data.writeInt32(mDisplayId);
    data.writeInt32(0); // dimming config. Do not need dimming now.
    data.writeInt32(modeId);
    data.writeInt32(paperModeId);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_EYECARE, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }
#endif
    return ret;
}

int DisplayEffectBase::HandleEyeCare(int modeId, int cookie)
{
    int ret = -1;
    struct OutputParam sParams;

    DF_LOGD("enter modeId %d", modeId);
    Mutex::Autolock autoLock(mEffectLock);

    if (modeId < 0 || modeId > 255) {
        DF_LOGE("Invalid modeId %d", modeId);
        modeId = modeId < 0 ? 0 : 255;
    }

    struct DispParam param;
    getDispParam(&param);
#ifdef HAS_QDCM
    if (param.eyeCareLevel == 0 && modeId != 0) {
        FpsMonitor & FM = FpsMonitor::getInstance();
        FM.checkFps(1);
    }
#endif
    if (mPaperMode.get())
        mPaperMode->setEyeCare(modeId);

    SetEyeCareToStc(modeId, mPaperModeId);

    if (getEffectStatus(DISPLAY_EYECARE)) {
        setDispParam(0, EYECARE_STATE, modeId, cookie);
        if (modeId == 0) {
            clearEyeCareStatus();
            if (!GetHdrStatus()) {
                //Start for MIUI-1570094
                if (!GetVideoModeId() && !mGameModeId && mSceneId == -1) {
                    struct DispParam param;
                    getDispParam(&param);
                    switch (param.caseId) {
                        case AUTOMATIC_CONTRAST_STATE: {
                            HandleACNormal(param.modeId, param.cookie);
                        } break;
                        case INCREASED_CONTRAST_STATE: {
                            HandleIncreasedContrast(0);
                        } break;
                        case STANDARD_STATE: {
                            HandleStandard(0);
                        } break;
                        case EXPERT_STATE: {
                            HandleExpert(EXPERT_RESTORE, 0);
                        } break;
                        default: {
                            DF_LOGE("param error caseId: %d modeId: %d cookie: %d",
                                param.caseId, param.modeId, param.cookie);
                        } break;
                    }
                } else if (GetVideoModeId()) {
                    mVideoMode->enableVideoMode(GetVideoModeId(), 0);
                }
                //end
            }
        }
        DF_LOGV("exit ret %d", ret);
        return ret;
    }

    if (modeId > 0) {
        if (modeId)
            _setEffectStatus(0x1 << DISPLAY_EYECARE);
        setDispParam(0, EYECARE_STATE, modeId, cookie);
    }
    DF_LOGV("exit ret %d", ret);

    return ret;
}

int DisplayEffectBase::SetPicHDRToStc(int modeId, int cookie)
{
    int ret = 0;
#ifdef HAS_QDCM
    DF_LOGD("%s enter modeId %d,cookie %d", __func__, modeId, cookie);
    sp<IMiStcService> imistcservice;
     android::Parcel data;
    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(mDisplayId);
    //data.writeInt32(0); // dimming config. Do not need dimming now.
    data.writeInt32(modeId);
    data.writeInt32(cookie);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_EXIF, &data, NULL);
    } else {
        DF_LOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }
#endif
    return ret;
}

int DisplayEffectBase::HandlePicHDR(int modeId, int cookie)
{
    int ret = 0;
    DF_LOGD("%s enter modeId %d,cookie %d", __func__, modeId, cookie);
    struct OutputParam sParams;
     Mutex::Autolock autoLock(mEffectLock);

    if (modeId < 0 ) {
        DF_LOGE("Invalid modeId %d", modeId);
        ret = -1;
        return ret;
    }
    mFlatMode->HandlePicHDRFlatMode(modeId,cookie);
    SetPicHDRToStc(modeId, cookie);
    DF_LOGD("%s exit ret %d", __func__, ret);
    return ret;
}

int DisplayEffectBase::HandleCabcModeCustom(int modeId, int cookie)
{
    int ret = -1;
    return ret;
}

int DisplayEffectBase::HandleAD(int modeId, int cookie)
{
    int ret = -1;
    struct OutputParam sParams;

    sParams.function_id = modeId;
    sParams.param1 = cookie;
    ret = setDisplayEffect(SET_LTM, &sParams);
    return ret;
}

int DisplayEffectBase::HandleSunlightScreen(int modeId, int cookie)
{
    int ret = -1;
    static int last_level = 0;
    struct OutputParam sParams;
    if (modeId < 0 ||modeId >= SUNLIGHT_SCREEN_LEVEL_MAX) {
        DF_LOGE("Invalid Sunlight screen level %d", modeId);
        return ret;
    }

    if (modeId != last_level) {
        DF_LOGD("enter modeId %d", modeId);
        sParams.function_id = DISPLAY_SUNLIGHT_SCREEN;
        sParams.param1 = modeId;
        sParams.param2 = cookie;
        ret = setDisplayEffect(SET_PCC_MODE, &sParams);
    }

    last_level = modeId;

    DF_LOGV("exit ret %d", ret);
    return ret;
}

int DisplayEffectBase::HandleNightMode(int modeId, int cookie)
{
    int ret = 0;
    return ret;
}

void DisplayEffectBase::restoreQcomMode()
{
    struct DispParam param;
    getDispParam(&param);
    struct OutputParam sParams;
    // Should clear eyecare value if eyecare closed.
    if (param.eyeCareLevel == 0 && getEffectStatus(DISPLAY_EYECARE)) {
        mMessageGeneration[EYECARE_STATE]++;
        sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_EYECARE, this);
        msg->setInt32("Generation", mMessageGeneration[EYECARE_STATE]);
        msg->setInt32("Value", -1);
        msg->setInt32("Cookie", 1);
        msg->post();
    }
    if (GetVideoModeId()) {
        sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_VIDEO, this);
        msg->setInt32("Value", -1);
        msg->setInt32("Cookie", 0);
        msg->post();
    } else {
        switch (param.caseId) {
            case AUTOMATIC_CONTRAST_STATE: {
                sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_ENV_ADAPTIVE_NORMAL, this);
                msg->setInt32("Value", param.modeId);
                msg->setInt32("Cookie", param.cookie);
                msg->post();
            } break;
            case INCREASED_CONTRAST_STATE: {
                sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_COLOR_ENHANCE, this);
                msg->setInt32("Value", param.modeId);
                msg->setInt32("Cookie", param.cookie);
                msg->post();
            } break;
            case STANDARD_STATE: {
                sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_STANDARD, this);
                msg->setInt32("Value", param.modeId);
                msg->setInt32("Cookie", param.cookie);
                msg->post();
            } break;
            case GAME_STATE: {
                sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_GAME, this);
                msg->setInt32("Value", param.modeId);
                msg->setInt32("Cookie", param.cookie);
                msg->post();
            } break;
            case EXPERT_STATE: {
                sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_EXPERT, this);
                msg->setInt32("Case", EXPERT_RESTORE);
                msg->setInt32("Value", 0);
                msg->post();
            } break;
            default: {
                DF_LOGE("param error caseId: %d modeId: %d cookie: %d",
                    param.caseId, param.modeId, param.cookie);
            } break;
        }
    }

    if (param.eyeCareLevel != 0) {
        mMessageGeneration[EYECARE_STATE]++;
        sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_EYECARE, this);
        msg->setInt32("Generation", mMessageGeneration[EYECARE_STATE]);
        msg->setInt32("Value", -1);
        msg->setInt32("Cookie", 1);
        msg->post();
    } else {
        sParams.function_id = DISPLAY_RESTORE_PCC;
        setDisplayEffect(SET_PCC_MODE, &sParams);
    }
}

int DisplayEffectBase::Handle10Bit(int modeId, int cookie)
{
    int ret = 0;
    struct OutputParam sParams;
    int brightness = 0;
    char BLValue[10];
    DF_LOGD("%s enter %d, the mActiveAppId: %d", __func__, modeId, curApp);
    if (modeId) {
        m10BitLayer = 1;
        if (curApp == IQIYI) {
            mHDRStatus |= 1 << CUVA_HDR;
            sParams.function_id = DISPLAY_COLOR_MODE_sRGB;
            ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
        }
    } else {
        m10BitLayer = 0;
        if ((mHDRStatus >> CUVA_HDR) & 0x1) {
            mHDRStatus &= ~(1 << CUVA_HDR);
            if (!mHDRStatus)
                restoreQcomMode();
        }
    }
    return ret;
}

int DisplayEffectBase::HandleHDR(int modeId, int cookie)
{
    int ret = 0, status = 0, value = 0;
    static int last_status;
    static int last_modeId;
    int brightness = 0;
    struct OutputParam sParams;
    int expertEnable = 0;
    ParamManager pmMgr;
    char BLValue[10];
    int DCEnable = 0;
    int dvFlat = property_get_int32("persist.vendor.dolbyvision.flat_on", 0);

    Mutex::Autolock autoLock(mHDRLock);
    DF_LOGD("enter %d", modeId);

    if (!mHDRSwitch) {
        DF_LOGW("HDR does not open");
        return 0;
    }
    expertEnable = pmMgr.getParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(),
                   pmMgr.ExpertNode[EXPERT_NODE_ENABLE].c_str());

    if (mBrightness.get()) {
        DCEnable = mBrightness->getDCMode();
    }
    DisplayUtil::onCallback(NOTIFY_HDR_STATE, modeId);
    if (modeId) {
        if (DCEnable) {
            setDCGamma(0);
        }
        if (modeId == 2) {
            if (dvFlat)
                sParams.function_id = DISPLAY_COLOR_MODE_DV_FLAT_ON;
            else
                sParams.function_id = DISPLAY_COLOR_MODE_DV_FLAT_OFF;
            ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
        }
#ifdef HAS_QDCM
        if (mAdaptiveHDR) {
            if (mDisplaySRE.get()) {
                mDisplaySRE->setSREFunction(LTM_HDR, 1);
                mDisplaySRE->setSREFunction(LTM_ON, 0);
            }
            DisplayUtil::onCallback(NOTIFY_HDR_STATE, 1);
        } else {
            if (mDisplaySRE.get()) {
                mDisplaySRE->setSREFunction(LTM_OFF, 0);
            }
        }
#endif
        if (last_modeId != modeId && !dvFlat && modeId != 2) {
            sParams.function_id = DISP_FEATURE_FLAT_MODE;
            sParams.param1 = FEATURE_OFF;
            mFlatMode->mFlatModeStatus = sParams.param1;
            ret |= setDisplayEffect(SET_PANEL_MODE, &sParams);
        }

        if (GetVideoModeId() != VIDEO_MODE_ORIGIN) {
            sParams.function_id = COLOR_LUT;
            sParams.param1 = 0;
            sParams.param2 = 0;
            sParams.vec_payload.push_back(1);
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);
            mLutId = -1;
        }
        if (expertEnable)
            HandleExpert(EXPERT_DISABLE, 0);
        mHDRStatus |= 1 << HDR10;
        status = 0x1 << DISPLAY_HDR;
        setEffectStatus(status);
    } else {
        mHDRStatus &= ~(1 << HDR10);
        if (DCEnable) {
            setDCGamma(1);
        }
        if (last_modeId != modeId && !mHDRStatus) {
            restorePanelEffect();
            restoreQcomMode();
        }
#ifdef HAS_QDCM
        if (mAdaptiveHDR) {
            if (mDisplaySRE.get()) {
                mDisplaySRE->setSREFunction(LTM_HDR, 0);
            }
            DisplayUtil::onCallback(NOTIFY_HDR_STATE, 0);
        } else {
            if (mDisplaySRE.get()) {
                mDisplaySRE->setSREFunction(LTM_ON, 0);
            }
        }
#endif
    }
    last_modeId = modeId;
    mCancelRestore = 0;

    DF_LOGV("exit ret:%d", ret);
    return ret;
}

int DisplayEffectBase::HandleEXTColorProc(int modeId, int cookie)
{
    int ret = 0;
    static int mExtCount = 0;
    return 0;
    //we add pcc mix of df and google pcc, do not need to handle this.
    if (modeId) {
        mExtCount += 1;
    } else if (modeId == 0 && mExtCount == 0) {
        return ret;
    } else if (modeId == 0) {
        mExtCount -= 1;
    }
    if (mExtCount)
        DisplayUtil::onCallback(SET_DC_PARSE_STATE, 0);
    else
        DisplayUtil::onCallback(SET_DC_PARSE_STATE, 1);
    return ret;
}

void DisplayEffectBase::HandleRegisterCallback(void * callback)
{
    if (callback) {
        DisplayUtil::registerCallback(callback);
        DF_LOGD("callback:0x%x", callback);
    }
}

int DisplayEffectBase::HandleCameraRequest(int modeId, int cookie)
{
    int ret = -1, status = 0;
    struct OutputParam sParams;

    DF_LOGD("enter");
    if (modeId) {
        if (getEffectStatus(DISPLAY_EXPERT)) {
            HandleExpert(EXPERT_DISABLE, 0);
            sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_P3;
            ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
        } else {
            sParams.function_id = DISPLAY_COLOR_MODE_ACNORMAL;
            ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
        }

        sParams.function_id = DISPLAY_CLEAR_PCC;
        ret |= setDisplayEffect(SET_PCC_MODE, &sParams);

        sParams.function_id = DISP_FEATURE_DIMMING;
        sParams.param1 = FEATURE_OFF;
        ret |= setDisplayEffect(SET_PANEL_MODE, &sParams);

        sParams.function_id = DISP_FEATURE_FLAT_MODE;
        sParams.param1 = FEATURE_OFF;
        mFlatMode->mFlatModeStatus = sParams.param1;
        ret |= setDisplayEffect(SET_PANEL_MODE, &sParams);
        status = 0x1 << DISPLAY_CAMERA;
        setEffectStatus(status);
    } else {
        restoreQcomMode();
	    if (mDisplayState == kStateOn) {
		     sParams.function_id = DISP_FEATURE_DIMMING;
		     sParams.param1 = FEATURE_ON;
		     ret = setDisplayEffect(SET_PANEL_MODE, &sParams);
	    }
    }
    DF_LOGV("exit ret %d", ret);

    return ret;
}

int DisplayEffectBase::HandleGameMode(int modeId, int cookie)
{
    int ret = 0;
    int status;
    ParamManager pmMgr;
    int expertEnable = 0;
    int lastModeId = 0;
    struct OutputParam sParams;

    if (modeId < GAME_MODE_ORIGIN ||modeId > GAME_MODE_VIVID_BRIGHT) {
        DF_LOGE("%s invalid mode id %d", __func__, modeId);
        return ret;
    }
    DF_LOGD("enter, displayId:%d, modeId %d", mDisplayId, modeId);
    if (mDisplayState != kStateOn && modeId) {
        lastModeId = mGameModeId;
        mGameModeId = modeId;
        DF_LOGD("return, displayId:%d, modeId %d", mDisplayId, modeId);
        return ret;
    }

    if (mPaperModeId) {
        if (modeId && !getGameModeId()) {
            if (mPaperMode.get()) {
                mPaperMode->clearPaperMode();
            }
        } else if (!modeId && getGameModeId()) {
            if (mPaperMode.get())
                mPaperMode->setPaperMode(mPaperModeId);
        }
    }
    lastModeId = mGameModeId;
    mGameModeId = modeId;

    expertEnable = pmMgr.getParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(),
                   pmMgr.ExpertNode[EXPERT_NODE_ENABLE].c_str());

    if (modeId == 0) {
        if (expertEnable && !mHDRStatus) {
            sParams.function_id = DISPLAY_EXPERT_MODE;
            sParams.param1 = mCurParam.expertParam[EXPERT_R] << 16
                           | mCurParam.expertParam[EXPERT_G] << 8
                           | mCurParam.expertParam[EXPERT_B];
            ret = setDisplayEffect(SET_PCC_MODE, &sParams);

            sParams.function_id = DISPLAY_FEATURE_REFRESH;
            ret = setDisplayEffect(SET_PCC_MODE, &sParams);

            sParams.function_id = COLOR_RESTORE;
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);

            status = 0x1 << DISPLAY_EXPERT;
            setEffectStatus(status);
        }
         if (lastModeId > 0){
             //restore
             sParams.function_id = LTM_OFF;
             ret = setDisplayEffect(SET_LTM, &sParams);
             sParams.function_id = LTM_ON;
             ret = setDisplayEffect(SET_LTM, &sParams);
             restoreQcomMode();
         }
    } else {
        if (expertEnable)
            HandleExpert(EXPERT_DISABLE, 0);

        if (modeId > 1 && lastModeId <= 1) {
            //set sre
            sParams.function_id = LTM_STRENGTH;
            sParams.param1 = 19000;
            ret = setDisplayEffect(SET_LTM, &sParams);
        }
        if (lastModeId == 0) {
            sParams.function_id = COLOR_LUT;
            sParams.param1 = 0;
            sParams.param2 = 0;
            sParams.vec_payload.push_back(1);
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);
            mLutId = -1;
        }
        status = 0x1 << DISPLAY_GAME;
        setEffectStatus(status);
    }

    DF_LOGV("exit");
    return ret;
}

int DisplayEffectBase::HandleColorTemp(int modeId, int cookie)
{
    int ret = -1;

    DF_LOGD("enter");

    if(mColorTempMode.get())
        ret = mColorTempMode->enableColorTempMode(modeId, cookie);

    DF_LOGD("%s exit", __func__);
    return ret;
}

int DisplayEffectBase::setDCGamma(int enable)
{
    return 0;
}

int DisplayEffectBase::getSceneId()
{
    return mSceneId;
}

int DisplayEffectBase::HandleDCBacklight(int value, int delay)
{
    int ret = -1;
    struct OutputParam sParams;

    if (mBrightness.get())
        mBrightness->setDCMode(value);

    ret = setDCGamma(value);
    if (ret) {
        DF_LOGE("Failed to set dc gamma");
    }

    sParams.function_id = DISP_FEATURE_DC;
    if (value) {
        sParams.param1 = FEATURE_ON;
        ret = setDisplayEffect(SET_PANEL_MODE, &sParams);
    } else {
        sParams.param1 = FEATURE_OFF;
        ret |= setDisplayEffect(SET_PANEL_MODE, &sParams);
    }
    DF_LOGD("%s ret %d", __func__, ret);
    return ret;
}

int DisplayEffectBase::processDCBacklight(int brightness, int cookie)
{
    return 0;
}

void DisplayEffectBase::updateBrightness(int brightness)
{
    mVirtualBl = brightness;
    if (brightness == 0) {
#ifdef HAS_QDCM
        if (mDisplayIGCDither.get()) {
            mDisplayIGCDither->setIGCDither(0, 0);
        }
#endif
    }
}

void DisplayEffectBase::updateAodDitherStatus(int status)
{
    DF_LOGD("%s status is %d", __func__, status);
#ifdef HAS_QDCM
    if (status== 0) {
        if (mDisplayIGCDither.get()) {
            mDisplayIGCDither->setIGCDither(0, 0);
        }
    }
    else {
        if (mDisplayIGCDither.get()) {
            mDisplayIGCDither->setIGCDither(1, mDitherStrength);
        }
    }
#endif
}

int DisplayEffectBase::handleDozeBrightnessMode(int modeId)
{
    int ret = 0;
    struct OutputParam sParams;

    if (modeId < DOZE_TO_NORMAL ||modeId > DOZE_BRIGHTNESS_LBM) {
        DF_LOGE("%s invalid input %d", __func__, modeId);
        return ret;
    }

    if (mDozeBrightnessMode != modeId) {
        sParams.function_id = DISP_FEATURE_DOZE_BRIGHTNESS;
        sParams.param1 = modeId;
        ret = setDisplayEffect(SET_PANEL_MODE, &sParams);
        mDozeBrightnessMode = modeId;
        DF_LOGD("DozeBrightnessMode:%d, displayId:%d", modeId, mDisplayId);
    }

    return ret;
}

void DisplayEffectBase::restoreExpertParam()
{
    ParamManager pmMgr;

    for (int i = EXPERT_NODE_GAMUT; i < EXPERT_NODE_MAX; i++)
        mCurParam.expertParam[i - EXPERT_NODE_GAMUT] =
            pmMgr.getParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(), pmMgr.ExpertNode[i].c_str());
    if (mCurParam.expertParam[0] == 0)
        mCurParam.expertParam[0] = mDefaultGamut;
    if (pmMgr.getParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(),
        pmMgr.ExpertNode[EXPERT_NODE_ENABLE].c_str()) == 1) {
        sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_EXPERT, this);
        msg->setInt32("Case", EXPERT_RESTORE);
        msg->setInt32("Value", 0);
        msg->post();
    }
}

void DisplayEffectBase::saveExpertParam()
{
    ParamManager pmMgr;
    bool ret = false;
    static int retryCnt = 3;
RETRY:
    if (retryCnt == 0) {
        DF_LOGE("failed 3 times, return!");
        retryCnt = 3;
        return;
    }
    ret = pmMgr.setParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(), pmMgr.ExpertNode[EXPERT_NODE_ENABLE].c_str(), 1);
    if (ret) {
        retryCnt--;
        goto RETRY;
    }
    for (int i = EXPERT_NODE_GAMUT; i < EXPERT_NODE_MAX; i++) {
        ret = pmMgr.setParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(), pmMgr.ExpertNode[i].c_str(),
                        mCurParam.expertParam[i - EXPERT_NODE_GAMUT]);
        if (ret) {
            retryCnt--;
            goto RETRY;
        }
    }
    retryCnt = 3;
    pmMgr.BackupXMLFile(0);
}

void DisplayEffectBase::ExpertParamValidCheck()
{
    if (mPaParam.mode > 3 ||mPaParam.mode < 0)
        mPaParam.mode = 0;
    if (mPaParam.R > 255 ||mPaParam.R < 50)
        mPaParam.R = 255;
    if (mPaParam.G > 255 ||mPaParam.G < 50)
        mPaParam.G = 255;
    if (mPaParam.B > 255 ||mPaParam.B < 50)
        mPaParam.B = 255;
    if (abs(mPaParam.hue) > 180)
        mPaParam.hue = 0;
    if (abs(mPaParam.saturation) > 50)
        mPaParam.saturation = 0;
    if (abs(mPaParam.value) > 255)
        mPaParam.value = 0;
    if (abs(mPaParam.contrast) > 100)
        mPaParam.contrast = 0;
    if (mPaParam.gamma < 170 ||mPaParam.gamma > 270)
        mPaParam.gamma = 220;
}

bool DisplayEffectBase::InputParamValidCheck(int caseId, int value)
{
    bool ret = true;
    switch (caseId) {
        case EXPERT_GAMUT:
            if (value < 0 ||value > 3)
                ret = false;
            break;
        case EXPERT_R:
        case EXPERT_G:
        case EXPERT_B:
            if (value < 50 ||value > 255)
                ret = false;
            break;
        case EXPERT_H:
            if (abs(value) > 180)
                ret = false;
            break;
        case EXPERT_S:
            if (abs(value) > 50)
                ret = false;
            break;
        case EXPERT_V:
            if (abs(value) > 255)
                ret = false;
            break;
        case EXPERT_CONTRAST:
            if (abs(value) > 100)
                ret = false;
            break;
        case EXPERT_GAMMA:
            if (value < 170 ||value > 270)
                ret = false;
            break;
        default:
            ret = true;
    }
    return ret;
}

int DisplayEffectBase::HandleExpert(int caseId, int value)
{
    int ret = -1;
    int status = 0;
    struct OutputParam sParams;
    if(!mExpertEnable)
        return ret;

    DF_LOGD("enter case %d value %d", caseId, value);
    if (!InputParamValidCheck(caseId, value)) {
        DF_LOGE("%s invalid input, case %d, value %d", __func__, caseId, value);
        return ret;
    }
    if (caseId != EXPERT_CLEAR && caseId != EXPERT_DISABLE) {
        setDispParam(0, EXPERT_STATE, value, caseId);
        if (mGameModeId || GetVideoModeId() || mHDRStatus) {
            DF_LOGD("Game mode, Video box or HDR working, skip set expert mode");
            return ret;
        }
    }

    switch (caseId) {
        case EXPERT_GAMUT:
            if (value == 0) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_NONE;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            } else if (value == 1) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_WCG;
                sParams.param1 = mWCGCorlorGamut;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            } else if (value == 2) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_P3;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            } else if (value == 3) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_SRGB;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            }
            break;
        case EXPERT_R:
        case EXPERT_G:
        case EXPERT_B:
            sParams.function_id = COLOR_PCC;
            sParams.param1 = caseId;
            sParams.param2 = value;
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);
            sParams.function_id = DISPLAY_EXPERT_MODE;
            sParams.param1 = mCurParam.expertParam[EXPERT_R] << 16
                           | mCurParam.expertParam[EXPERT_G] << 8
                           | mCurParam.expertParam[EXPERT_B];
            ret = setDisplayEffect(SET_PCC_MODE, &sParams);
            break;
        case EXPERT_H:
        case EXPERT_S:
        case EXPERT_V:
        case EXPERT_CONTRAST:
            sParams.function_id = COLOR_PA;
            sParams.param1 = caseId;
            sParams.param2 = value;
            sParams.vec_payload.push_back(0);
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);
            break;
        case EXPERT_GAMMA:
            sParams.function_id = COLOR_GAMMA;
            sParams.param1 = value;
            sParams.param2 = GAMMA_GC;
            sParams.vec_payload.push_back(1); //Enable
            sParams.vec_payload.push_back(0); //Gamma File Id, only useful when param1 is 0
            sParams.vec_payload.push_back(0); //Dimming
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);
            break;
        case EXPERT_CLEAR:
            //EXPERT CLEAR will clear effects and params.
            mCurParam.expertParam[0] = mDefaultGamut;
            mCurParam.expertParam[1] = 255;
            mCurParam.expertParam[2] = 255;
            mCurParam.expertParam[3] = 255;
            mCurParam.expertParam[4] = 0;
            mCurParam.expertParam[5] = 0;
            mCurParam.expertParam[6] = 0;
            mCurParam.expertParam[7] = 0;
            mCurParam.expertParam[8] = 220;
            mPaParam.mode       = mCurParam.expertParam[0];
            mPaParam.R          = mCurParam.expertParam[1];
            mPaParam.G          = mCurParam.expertParam[2];
            mPaParam.B          = mCurParam.expertParam[3];
            mPaParam.hue        = mCurParam.expertParam[4];
            mPaParam.saturation = mCurParam.expertParam[5];
            mPaParam.value      = mCurParam.expertParam[6];
            mPaParam.contrast   = mCurParam.expertParam[7];
            mPaParam.gamma      = mCurParam.expertParam[8];
            setParams(SET_DSPP_MODE, (void*)&mPaParam);

            sParams.function_id = COLOR_CLEAR;
            sParams.param1 = 1;
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);

            sParams.function_id = DISPLAY_EXPERT_MODE;
            sParams.param1 = 0x00FFFFFF;
            ret = setDisplayEffect(SET_PCC_MODE, &sParams);
            break;
        case EXPERT_DISABLE:
            //EXPERT DISABLE just clear effects, params will not clear, for FOD.
            sParams.function_id = COLOR_CLEAR;
            sParams.param1 = 0;
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);

            sParams.function_id = DISPLAY_EXPERT_MODE;
            sParams.param1 = 0x00FFFFFF;
            ret = setDisplayEffect(SET_PCC_MODE, &sParams);
            break;
        case EXPERT_RESTORE:
            mPaParam.mode       = mCurParam.expertParam[0];
            mPaParam.R          = mCurParam.expertParam[1];
            mPaParam.G          = mCurParam.expertParam[2];
            mPaParam.B          = mCurParam.expertParam[3];
            mPaParam.hue        = mCurParam.expertParam[4];
            mPaParam.saturation = mCurParam.expertParam[5];
            mPaParam.value      = mCurParam.expertParam[6];
            mPaParam.contrast   = mCurParam.expertParam[7];
            mPaParam.gamma      = mCurParam.expertParam[8];
            ExpertParamValidCheck();
            if (mPaParam.mode == 0) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_NONE;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            } else if (mPaParam.mode == 1) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_WCG;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            } else if (mPaParam.mode == 2) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_P3;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            } else if (mPaParam.mode == 3) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_SRGB;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            }

            setParams(SET_DSPP_MODE, (void*)&mPaParam);

            sParams.function_id = COLOR_RESTORE;
            ret = setDisplayEffect(SET_DSPP_MODE, &sParams);

            sParams.function_id = DISPLAY_EXPERT_MODE;
            sParams.param1 = mPaParam.R << 16 | mPaParam.G << 8 | mPaParam.B;
            ret = setDisplayEffect(SET_PCC_MODE, &sParams);

            sParams.function_id = DISPLAY_FEATURE_REFRESH;
            ret = setDisplayEffect(SET_PCC_MODE, &sParams);
            break;
        default:
            DF_LOGE("err case id %d", caseId);
            break;
    }
    if (caseId != EXPERT_CLEAR && caseId != EXPERT_DISABLE) {
        status = 0x1 << DISPLAY_EXPERT;
        setEffectStatus(status);
    }
    saveExpertParam();

    if (caseId == EXPERT_DISABLE) {
        if (mBrightness.get()) {
            int DCEnable = 0;
            DCEnable = mBrightness->getDCMode();
            if (DCEnable) {
                setDCGamma(DCEnable);
            }
        }
    }

    return ret;
}

int DisplayEffectBase::HandleVideoMode(int modeId, int cookie)
{
    int ret = 0;
    int inputModeId = modeId;
    DF_LOGD("enter %d", modeId);

    if (modeId == -1) {
        modeId = GetVideoModeId();
        mVideoMode->mVideoModeId = VIDEO_MODE_ORIGIN;
    }
    if (modeId < VIDEO_MODE_ORIGIN ||modeId >= VIDEO_MODE_MAX) {
        DF_LOGE("%s invalid input %d", __func__, modeId);
        return ret;
    }
    if (mPaperModeId) {
        if (inputModeId > 0 && !GetVideoModeId()) {
            if (mPaperMode.get())
                mPaperMode->clearPaperMode();
        } else if (!modeId && GetVideoModeId()) {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_PAPER_MODE, this);
            msg->setInt32("Value", -1);
            msg->setInt32("Cookie", 255);
            msg->post();
        }
    }
    if (mVideoMode.get()) {
        ret = mVideoMode->enableVideoMode((-1 == inputModeId) ? VIDEO_MODE_ORIGIN : modeId, cookie);
        mVideoMode->mVideoModeId = modeId;
    }

    return ret;
}

int DisplayEffectBase::GetExpertMode()
{
    int ret;
    ParamManager pmMgr;
    ret = pmMgr.getParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(),
                   pmMgr.ExpertNode[EXPERT_NODE_ENABLE].c_str());
    return ret;
}

void DisplayEffectBase::RestoreAIDisplay()
{
    if (mSceneId != -1) {
        sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_SCENE, this);
        msg->setInt32("modeId", mSceneId);
        msg->setInt32("cookie", 0);
        msg->post();
    }

}

int DisplayEffectBase::GetVideoModeId()
{
    return mVideoMode->mVideoModeId;

}

int DisplayEffectBase::HandlePaperMode(int modeId, int cookie)
{
    int ret = -1;
    struct OutputParam sParams;

    DF_LOGD("enter mode %d", modeId);
    if (modeId < 0 ||modeId > 2) {
        DF_LOGE("%s invalid input value %d", __func__, modeId);
        return ret;
    }

    if (mPaperMode.get())
        mPaperMode->setPaperMode(modeId);
#ifndef MTK_PLATFORM
    struct DispParam param;
    getDispParam(&param);
    ret = SetEyeCareToStc(param.eyeCareLevel, modeId);
#endif
    mPaperModeId = modeId;
    return ret;
}

int DisplayEffectBase::onSuspend()
{
    int ret = -1;
    struct OutputParam sParams;

    DF_LOGD("enter");

    if (getEffectStatus() & 0x3F) {
#ifdef HAS_QDCM
        if (mDisplaySRE.get()) {
            ret = mDisplaySRE->HandleSunlightScreen(0, 0xFF); //clear sunlight pcc coeffs.
        }
#endif
    }

    mSuspendFlag = 1;
    current_mode_index = -1;

    return ret;
}

int DisplayEffectBase::onResume()
{
    int ret = 0;
    int generation = 0;
    struct OutputParam sParams;
    bool dc_state = 0;
    Timer t;

    if (!first_cycle) {
        DisplayUtil::onCallback(UPDATE_SECONDARY_FRAME_RATE, 0);
    }

    current_fps = (current_fps ? current_fps : 60);
    notifySurfaceflingerFpsSwitch(current_fps);

    sParams.function_id = LTM_ON;
    ret = setDisplayEffect(SET_LTM, &sParams);
    if (mSceneId != -1) {
        if (curApp == GALLERY) {
            sp<AMessage> msg = new AMessage(DisplayEffectBase::kDISPLAY_SCENE, this);
            msg->setInt32("modeId", mSceneId);
            msg->setInt32("cookie", 0);
            msg->post();
        } else {
            mSceneId = -1;
            restoreQcomMode();
        }
    } else {
        restoreQcomMode();
    }
    HandleColorTemp(mCurParam.CTLevel, 0);

    if (mDarkMode) {
        HandleDarkMode(mDarkMode, 0);
    } else {
        mTimerGeneration[TIMER_IGC_DITHER]++;
        t.AsyncWait(0, DEtimerCallbackFunc, this,
                 TIMER_IGC_DITHER, mTimerGeneration[TIMER_IGC_DITHER], 1);
        mDitherEn = 1;
    }

    return ret;
}

int DisplayEffectBase::getDisplayState()
{
    return mDisplayState;
}

int DisplayEffectBase::HandleDisplayState(int modeId, int cookie)
{
    int ret = 0;
    int lastState = 0;
    Timer t;
    struct OutputParam sParams;
    android::Parcel data;
    ParamManager pmMgr;

    if (modeId < kStateOff ||modeId > kStateStandby) {
        DF_LOGE("%s invalid display state %d", __func__, modeId);
        return ret;
    }
    DF_LOGD("enter state %s, last state %s",
                getDisplayStateName(modeId), getDisplayStateName(mDisplayState));

    lastState = mDisplayState;
    mDisplayState = modeId;

#ifdef HAS_QDCM
    sp<IMiStcService> imistcservice;
    if (mDisplaySRE.get()) {
        mDisplaySRE->setDisplayState(mDisplayState);
    }

    if (mDisplayVSDR2HDR.get()) {
        mDisplayVSDR2HDR->setDisplayState(mDisplayState);
    }

    if (mDisplayBCBC.get()) {
        mDisplayBCBC->setDisplayState(mDisplayState);
    }

    if (mDisplayIGCDither.get()) {
        mDisplayIGCDither->setDisplayState(mDisplayState);
    }

    FpsMonitor & FM = FpsMonitor::getInstance();
    FM.SetDisplayState(mDisplayId, mDisplayState);
#endif
    if (mTrueTone.get())
        mTrueTone->SetDisplayState(mDisplayState);

    if (mBrightness.get())
        mBrightness->SetDisplayState(mDisplayState);

    if (mVideoMode.get())
        mVideoMode->SetDisplayState(mDisplayState);

    if (first_cycle) {
        if (modeId == kStateOn) {

            /* first boot read fps property to restore */
            getDisplayAttributesInfo();
            settings_fps = (mDisplayId ? property_get_int32(PERSIST_VENDOR_DFPS_LEVEL, 60) : 60);
            power_limit_fps = (mDisplayId ? property_get_int32(PERSIST_VENDOR_POWER_DFPS_LEVEL, 0) : 0);
            DF_LOGD("settings_fps(%d), power_limit_fps(%d)",
                    settings_fps, power_limit_fps);
            if (power_limit_fps)
                current_fps = power_limit_fps;
            else
                current_fps = settings_fps;

            notifySurfaceflingerFpsSwitch(current_fps);
            mTimerGeneration[TIMER_IGC_DITHER]++;
            t.AsyncWait(10 * 1000, DEtimerCallbackFunc, this,
                TIMER_IGC_DITHER, mTimerGeneration[TIMER_IGC_DITHER], 1);
            mDitherEn = 1;
        }
        if (property_get_bool(PERSIST_DISPLAYFEATURE_DC_BL_ENABLE, false)) {
            HandleDCBacklight(1, 0);
        }
    }

    if (modeId == kStateOn) {
        if (lastState != kStateOn) {
            ret = onResume();
        }
    } else if (modeId == kStateOff && lastState == kStateOn) {
        ret = onSuspend();
    }
#ifdef HAS_QDCM
    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(mDisplayId);
    data.writeInt32(modeId);
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_DISPLAY_STATE, &data, NULL);
    } else {
        ALOGE("Failed to acquire %s", "display.mistcservice");
        ret = -1;
    }
#endif
    if (first_cycle)
        first_cycle = false;

    if (!mDisplayId && !mFoldSensorStarted && modeId != lastState) {
        SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
        mSensorCtrl.registerCallBack((void*)DFSensorCallbackFunction, this);
        mSensorCtrl.sensorControlOP(mDisplayId, SENSOR_START, SENSOR_FOLD, FOLD);
        mFoldSensorStarted = true;
    }
    if(modeId == kStateOff) {
        pmMgr.BackupXMLFile(0);
    }

    DF_LOGV("exit");

    return ret;
}

int DisplayEffectBase::HandleDisplayMutualState(int displayId,int state)
{
    if (mDisplayId == DISPLAY_PRIMARY) {
        return 0;
    }

    if (displayId == DISPLAY_PRIMARY && state == kStateOff
        && mDisplayState == kStateOn) {
        notifySurfaceflingerFpsSwitch(current_fps);
    }

    return 0;
}

int DisplayEffectBase::getLimitFps()
{
    return power_limit_fps ? power_limit_fps : thermal_limit_fps;
}

int DisplayEffectBase::HandleFpsSwitch(int modeId, int cookie)
{
    int input_fps = modeId;
    int switch_module= cookie;
    int temp_fps = 0;
    int ret = 0;

    DF_LOGI("displayId:%d, %s app set input_fps(%d), current_fps(%d)",
            mDisplayId, getFpsSwitchAppName(switch_module), input_fps, current_fps);

    if (mDisplayId == DISPLAY_PRIMARY) {
        switch (switch_module) {
            case FPS_SWITCH_SETTINGS: {
                if (FPS_60HZ == input_fps || FPS_90HZ == input_fps) {
                    setSettingsFps(input_fps);
                    setPowerLimitFps(0);
                }
            } break;
            case FPS_SWITCH_POWER: {
                setPowerLimitFps(input_fps);
            } break;
            default:
                DF_LOGV("%s module is not supported, input_fps: %d",
                    getFpsSwitchAppName(switch_module), input_fps);
        }

        return ret;
    }

    switch (switch_module) {
        case FPS_SWITCH_VIDEO:
        case FPS_SWITCH_TOPAPP:
        case FPS_SWITCH_MULTITASK:
        case FPS_SWITCH_CAMERA:
        case FPS_SWITCH_DEFAULT:
        case FPS_SWITCH_GAME: {
            if (input_fps > settings_fps) {
                DF_LOGE("%s app set input_fps(%d) > user settings fps(%d),skip!",
                        getFpsSwitchAppName(switch_module), input_fps, settings_fps);
                return -1;
            }
            if ( (power_limit_fps && input_fps > power_limit_fps) ||
                 (thermal_limit_fps && input_fps > thermal_limit_fps)) {
                 DF_LOGE("%s can't set input_fps(%d) > power_limit_fps(%d) or thermal_limit_fps(%d), skip!",
                         getFpsSwitchAppName(switch_module), input_fps, power_limit_fps, thermal_limit_fps);
                 return -1;
            }
            else if (0 == input_fps) {
                if (power_limit_fps)
                    temp_fps = MIN(power_limit_fps, settings_fps);
                if (thermal_limit_fps)
                    temp_fps = MIN(thermal_limit_fps, settings_fps);
                temp_fps = temp_fps ? temp_fps : settings_fps;
                last_app_fps = 0;
            }
            else {
                temp_fps = last_app_fps = input_fps;
            }
        } break;
        case FPS_SWITCH_SETTINGS: {
            if (FPS_60HZ == input_fps || FPS_90HZ == input_fps) {
                temp_fps = input_fps;
                setSettingsFps(input_fps);
                setPowerLimitFps(0);
                thermal_limit_fps = 0;
                last_app_fps = input_fps;
            }
        } break;
        case FPS_SWITCH_POWER: {
            setPowerLimitFps(input_fps);
            if (power_limit_fps > 0) {
                temp_fps = power_limit_fps;
            } else {
                if (thermal_limit_fps)
                    temp_fps = thermal_limit_fps;
                else
                    temp_fps = last_app_fps ? last_app_fps : settings_fps;
            }
        } break;
        case FPS_SWITCH_THERMAL: {
            thermal_limit_fps = input_fps;
            if (thermal_limit_fps > 0) {
                temp_fps = power_limit_fps ? power_limit_fps : thermal_limit_fps;
            } else {
                if (power_limit_fps > 0)
                    temp_fps = power_limit_fps;
                else
                    temp_fps = last_app_fps ? last_app_fps : settings_fps;
            }
        } break;
        default: {
            DF_LOGE("%s module is not supported, input_fps: %d",
                    getFpsSwitchAppName(switch_module), input_fps);
            return -1;
        }
    }

    if (temp_fps != current_fps) {
        current_fps = temp_fps;
        ret = notifySurfaceflingerFpsSwitch(current_fps);
        if (!ret)
            DF_LOGI("really set fps(%d)", current_fps);
        else
            DF_LOGE("failed to set fps(%d)", current_fps);
    }

    return ret;
}

int DisplayEffectBase::getDisplayAttributesInfo(void)
{
    int error = 0;
#ifdef HAS_QDCM
    uint32_t index;
    uint32_t count = 0;
    Attributes dpyattr;
    DisplayType dpy = DisplayType::kInvalid;

    if (!mDisplayConfigIntf) {
        return 0;
    }

    dpy = (mDisplayId == DISPLAY_PRIMARY ? DisplayType::kPrimary : DisplayType::kBuiltIn2);

    error = mDisplayConfigIntf->GetConfigCount(dpy, &count);
    if (error){
        DF_LOGE("GetConfigCount failed error = %d !", error);
        return -1;
    }

    if (count > 0) {
        display_attributes_info.resize(count);
        for (index = 0; index < count; index++) {
            error = mDisplayConfigIntf->GetDisplayAttributes(index, dpy, &dpyattr);
            if (error){
                DF_LOGE("GetDisplayAttributes failed error = %d !", error);
                return -1;
            }

            display_attributes_info[index].mode_index = index;
            display_attributes_info[index].fps = uint32_t(1000000000L / dpyattr.vsync_period);
            display_attributes_info[index].vsync_period_ns = dpyattr.vsync_period;
            display_attributes_info[index].x_pixels = dpyattr.x_res;
            display_attributes_info[index].y_pixels = dpyattr.y_res;
            DF_LOGI("Display attributes[%d]: WxH: %dx%d, fps: %d, vsync_period: %d",
                index, display_attributes_info[index].x_pixels, display_attributes_info[index].y_pixels,
                display_attributes_info[index].fps, display_attributes_info[index].vsync_period_ns);
        }
        return 0;
    } else {
        return -1;
    }
#endif
    return 0;
}

int DisplayEffectBase::notifySurfaceflingerFpsSwitch(int fps)
{
    int index, count = 0;
    int mode_index = -1;
    int cmd_type = 0;
    bool found = false;

    DF_LOGI("displayId:%d, fps = %d", mDisplayId, fps);

    count = display_attributes_info.size();
    for (index = 0; index < count; index++) {
        if (display_attributes_info[index].fps == fps) {
            mode_index = display_attributes_info[index].mode_index;
            found = true;
            break;
        }
    }
    if (!found) {
        DF_LOGE("via fps(%d) did not find validated mode index", fps);
        return -1;
    }

    current_mode_index = mode_index;
    DF_LOGI("mode index = %d", mode_index);
    if (getDisplayState() != kStateOn) {
        DF_LOGI("fps(%d) only On mode can switch fps", fps);
        return 0;
    } else {
        if (!DisplayUtil::isRegisterCallbackDone()) {
            if (mDisplayDFPS.get()) {
                mDisplayDFPS->setFramerateIndex(mode_index);
                mDisplayDFPS->runThreadLoop();
            }
        } else {
            cmd_type = (mDisplayId ? 10036 : 10035);
            DisplayUtil::onCallback(cmd_type, mode_index);
        }
    }

    return 0;
}

void DisplayEffectBase::setSettingsFps(int fps)
{
    char property_value[PROPERTY_VALUE_MAX];

    settings_fps = fps;

    snprintf(property_value, 32, "%d", fps);
    property_set(PERSIST_VENDOR_DFPS_LEVEL, property_value);
}

void DisplayEffectBase::setPowerLimitFps(int fps)
{
    char property_value[PROPERTY_VALUE_MAX];

    power_limit_fps = fps;

    snprintf(property_value, 32, "%d", fps);
    property_set(PERSIST_VENDOR_POWER_DFPS_LEVEL, property_value);
}

int DisplayEffectBase::HandleDisplayScene(int modeId, int cookie)
{
    DF_LOGD("enter modeId %d", modeId);
    struct OutputParam sParams;
    int ret = 0;
    Timer t;
    int lutId;

    int lutDimId = 0;
    if (modeId < SCENE_NULL ||modeId > SCENE_MAX) {
        DF_LOGE("%s invalid input scene %d", __func__, modeId);
        return ret;
    }
    mSceneId = modeId;

    if (GetVideoModeId()) {
        DF_LOGW("skip set scene %d because video box mode %d working", modeId, GetVideoModeId());
        return ret;
    }
    if (curApp != GALLERY || mDisplayState != kStateOn) {
        DF_LOGE("Receive display scene %d in non-gallery app or Display State invalid %d.", modeId, mDisplayState);
        return ret;
    }

    switch (modeId) {
        case SCENE_PORTRAIT:
            if (getEffectStatus(DISPLAY_ENV_ADAPTIVE_NORMAL)) {
                ApplySceneLut(MI_DF_SCENE_0);
                sParams.function_id = DISPLAY_COLOR_MODE_SCENE0;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            } else if (getEffectStatus(DISPLAY_COLOR_ENHANCE)) {
                ret = DisableLut();
                if (ret)
                    DF_LOGE("Failed to disable lut");
                sParams.function_id = DISPLAY_COLOR_MODE_SCENE_NATIVE_NONE;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            }
            break;
        case SCENE_WATERSIDES:
        case SCENE_BLUESKY:
        case SCENE_MOUNTAIN:
        case SCENE_SUNRISE:
            ApplySceneLut(MI_DF_SCENE_1);
            sParams.function_id = DISPLAY_COLOR_MODE_SCENE1;
            ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            break;
        case SCENE_ARCHITECTURE:
        case SCENE_FLOWERS:
        case SCENE_PLANTS:
        case SCENE_INDOOR:
        case SCENE_TEXTURE:
            ApplySceneLut(MI_DF_SCENE_2);
            sParams.function_id = DISPLAY_COLOR_MODE_SCENE2;
            ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            break;
        case SCENE_ANIMAL:
            ApplySceneLut(MI_DF_SCENE_3);
            sParams.function_id = DISPLAY_COLOR_MODE_SCENE3;
            ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            break;
        case SCENE_DOCUMENT:
            ApplySceneLut(MI_DF_SCENE_4);
            sParams.function_id = DISPLAY_COLOR_MODE_SCENE4;
            ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            break;
        default:
            if (getEffectStatus(DISPLAY_ENV_ADAPTIVE_NORMAL)) {
                ApplySceneLut(MI_DF_SCENE_DEFAULT);
                sParams.function_id = DISPLAY_COLOR_MODE_SCENE_ACNORMAL_NONE;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            } else if (getEffectStatus(DISPLAY_COLOR_ENHANCE)) {
                ret = DisableLut();
                if (ret)
                    DF_LOGE("Failed to disable lut");
                sParams.function_id = DISPLAY_COLOR_MODE_SCENE0_NATIVE;
                ret = setDisplayEffect(SET_COLOR_MODE, &sParams);
            }
            break;
    }

    return ret;
}

int DisplayEffectBase::DisableLut()
{
    int ret = 0;
    struct OutputParam sParams;
    sParams.function_id = COLOR_LUT;
    sParams.param1 = 0;
    sParams.param2 = 0;
    sParams.vec_payload.push_back(1);
    ret = setDisplayEffect(SET_DSPP_MODE, &sParams);
    mLutId = -1;

    return ret;
}

void DisplayEffectBase::ApplySceneLut(int sceneId)
{
    struct OutputParam sParams;
    int lutId = AI_DISP_LUT_0;
    if (getEffectStatus(DISPLAY_ENV_ADAPTIVE_NORMAL)) {
        if (sceneId == MI_DF_SCENE_DEFAULT ||
            sceneId == MI_DF_SCENE_0 ||
            sceneId == MI_DF_SCENE_3 ||
            sceneId == MI_DF_SCENE_4) {
            lutId = AI_DISP_LUT_0;
        } else if (sceneId == MI_DF_SCENE_1 || sceneId == MI_DF_SCENE_2) {
            lutId = AI_DISP_LUT_1;
        }
    } else if (getEffectStatus(DISPLAY_COLOR_ENHANCE)) {
        lutId = AI_DISP_LUT_2;
    }
    sParams.function_id = COLOR_LUT;
    sParams.param1 = lutId;
    sParams.param2 = 1;
    sParams.vec_payload.push_back(1);
    setDisplayEffect(SET_DSPP_MODE, &sParams);
    mLutId = lutId;
}

int DisplayEffectBase::HandleActiveApp(int modeId, int cookie)
{
    int ret = 0;
    struct OutputParam sParams;
    DF_LOGD("current active app %d", modeId);
    if (modeId < NONE || modeId > TIKTOK) {
        DF_LOGE("%s invalid input %d", __func__, modeId);
        return ret;
    }
    mFlatMode->HandleAppFlatMode(modeId,cookie);
    if (mTrueTone.get())
        mTrueTone->setActiveApp(modeId);
    if (modeId != GALLERY && curApp == GALLERY) {
        mSceneId = -1;
        if (mDisplayState == kStateOn) {
            restoreQcomMode();
        }
    }

    curApp = modeId;

    if (m10BitLayer) {
        Handle10Bit(1, 0);
    }
    return ret;
}

int DisplayEffectBase::HandleDarkMode(int modeId, int cookie)
{
    struct OutputParam sParams;
    int ret = -1;
    Timer t;
    int value = 0;
    DF_LOGD("enter mode %d", modeId);
    value = modeId ? 0 : 1;
    if (value == mDitherEn)
        return ret;
    mTimerGeneration[TIMER_IGC_DITHER]++;
    if (mDisplayState == kStateOn)
        t.AsyncWait(0, DEtimerCallbackFunc, this,
                TIMER_IGC_DITHER, mTimerGeneration[TIMER_IGC_DITHER], value);
    mDitherEn = value;
    mDarkMode = modeId;
    return ret;
}

int DisplayEffectBase::HandleTrueTone(int modeId, int cookie)
{
    int ret = 0;
    DF_LOGD("enter %d", modeId);
    if (mTrueTone.get())
        ret = mTrueTone->enableTrueTone(modeId, cookie);
    return ret;
}

int DisplayEffectBase::HandleAdaptiveHDR(int modeId, int cookie)
{
    int ret = 0;
    if ((mHDRStatus >> HDR10) & 0x1) {
#ifdef HAS_QDCM
        if (mDisplaySRE.get()) {
            if (modeId) {
                ret = mDisplaySRE->setSREFunction(LTM_HDR, 1);
                ret |= mDisplaySRE->setSREFunction(LTM_ON, 0);
            } else {
                ret = mDisplaySRE->setSREFunction(LTM_HDR, 0);
                ret |= mDisplaySRE->setSREFunction(LTM_OFF, 0);
            }
        }
#endif
    }
    mAdaptiveHDR = modeId;
    DF_LOGD("set adaptive hdr %d", modeId);
    return ret;
}

void DisplayEffectBase::HandleLowBrightnessFOD(int state, int cookie) {
    int ret = 0;
    DF_LOGD("state = %d", state);

    struct OutputParam sParams;
    sParams.function_id = DISP_FEATURE_LOW_BRIGHTNESS_FOD;
    sParams.param1 = (uint32_t)state;
    ret = setDisplayEffect(SET_PANEL_MODE, &sParams);

    mFodSensorMonitor->switchState(state);
}

void DisplayEffectBase::HandleTouchState(int caseId, int value)
{

}

int DisplayEffectBase::getFlatModeStatus()
{
    return mFlatMode->mFlatModeStatus;
}

void DisplayEffectBase::HandleDVStatus(int enable, int cookie)
{
    DF_LOGD("%s enter enable %d", __func__, enable);
    DisplayUtil::onCallback(NOTIFY_DOLBY_STATE, enable);
    int value = enable ? 2 : 0;
    struct OutputParam sParams;
    if (enable) {
        sParams.function_id = COLOR_LUT;
        sParams.param1 = 20;
        sParams.param2 = 1;
        sParams.vec_payload.push_back(1);
        setDisplayEffect(SET_DSPP_MODE, &sParams);
        mLutId = sParams.param1;
    }
    HandleHDR(value, cookie);
}

void DisplayEffectBase::restoreBlPcc(int value)
{
#ifdef HAS_QDCM
    sp<IMiStcService> imistcservice;
    android::Parcel data;
    if (value == AUTH_STOP && mBrightness->param.dcDimmingEnable) {
        imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
        data.writeInt32(mDisplayId);
        if (imistcservice.get()) {
            imistcservice->dispatch(IMiStcService::RESTORE_BL_PCC, &data, NULL);
        } else {
            ALOGE("Failed to acquire %s", "display.mistcservice");
        }
    } else if (value == AUTH_STOP && mBrightness->param.Mi12BitEnable) {
  	    int lastBl = mBrightness->getLastNonZeroBl();
  	    mBrightness->Set12BitPCCConfig(lastBl);
    }
#endif
}

int DisplayEffectBase::HandleC3dLut(int modeId, int cookie)
{
    int ret = 0;
    struct OutputParam sParams;
    DF_LOGD("%s enter, modeId %d, cookie = %d", __func__, modeId, cookie);

    sParams.function_id = COLOR_LUT;
    sParams.param1 = modeId;
    sParams.param2 = cookie;
    sParams.vec_payload.push_back(0);
    setDisplayEffect(SET_DSPP_MODE, &sParams);
    mLutId = modeId;

    DF_LOGD("%s exit", __func__);
    return ret;
}

int DisplayEffectBase::HandleDataBase(int modeId, int cookie)
{
    return 0;
}

char* DisplayEffectBase::printfMessage(int id)
{
    switch (id) {
        case kLOW_BRIGHTNESS_FOD:              return "LOW_BRIGHTNESS_FOD";
        case kDISPLAY_SCENE:                   return "SCENE";
        case kDISPLAY_ENV_ADAPTIVE_COLD:       return "ENV_ADAPTIVE_COLD";
        case kDISPLAY_ENV_ADAPTIVE_NORMAL:     return "ENV_ADAPTIVE_NORMAL";
        case kDISPLAY_ENV_ADAPTIVE_WARM:       return "ENV_ADAPTIVE_WARM";
        case kDISPLAY_COLOR_ENHANCE:           return "COLOR_ENHANCE";
        case kDISPLAY_STANDARD:                return "STANDARD";
        case kDISPLAY_EYECARE:                 return "EYECARE";
        case kDISPLAY_COLOR_TEMPERATURE:       return "COLOR_TEMPERATURE";
        case kDISPLAY_KEEP_WP_SRGB:            return "KEEP_WP_SRGB";
        case kDISPLAY_CABC_MODE_SWITCH:        return "CABC_MODE_SWITCH";
        case kDISPLAY_BRIGHTNESS_NOTIFY:       return "BRIGHTNESS_NOTIFY";
        case kDISPLAY_NIGHT_MODE:              return "NIGHT_MODE";
        case kDISPLAY_CCBB:                    return "CCBB";
        case kDISPLAY_AD:                      return "AD";
        case kDISPLAY_HDR:                     return "HDR";
        case kDISPLAY_HBM:                     return "HBM";
        case kDISPLAY_STATE_NOTIFY:            return "DISPLAY_STATE_NOTIFY";
        case kDISPLAY_EXT_COLOR_PROC:          return "EXT_COLOR_PROC";
        case kDISPLAY_FOD_COLOR_MODE_P3:       return "FOD_COLOR_MODE_P3";
        case kDISPLAY_FOD_RESTORE_QCOM_MODE:   return "FOD_RESTORE_QCOM_MODE";
        case kDISPLAY_FOD_SCREEN_ON:           return "FOD_SCREEN_ON";
        case kDISPLAY_FOD_PRE_HBM:             return "FOD_PRE_HBM";
        case kDISPLAY_CAMERA:                  return "CAMERA";
        case kDISPLAY_SET_CALLBACK:            return "SET_CALLBACK";
        case kDISPLAY_BCBC_CUSTOM:             return "BCBC";
        case kDISPLAY_GAME:                    return "GAME";
        case kDISPLAY_SUNLIGHT_SCREEN:         return "SUNLIGHT_SCREEN";
        case kDISPLAY_DC_BACKLIGHT:            return "DC_BACKLIGHT";
        case kDISPLAY_DOZE_BRIGHTNESS_MODE:    return "DOZE_BRIGHTNESS";
        case kDISPLAY_EXPERT:                  return "EXPERT";
        case kDISPLAY_VIDEO:                   return "VIDEOBOX";
        case kDISPLAY_SENSOR_CONTROL:          return "SENSOR_CONTROL";
        case kDISPLAY_PAPER_MODE:              return "PAPER_MODE";
        case kDISPLAY_TRUE_TONE:               return "TRUETONE";
        case kDISPLAY_ACTIVE_APP:              return "ACTIVE_APP";
        case kDARK_MODE:                       return "DARK_MODE";
        case kDISPLAY_STATE_NOTIFY_MUTUAL:     return "DISPLAY_STATE_NOTIFY_MUTUAL";
        case kADAPTIVE_HDR:                    return "ADAPTIVE_HDR";
        case kDISPLAY_DITHER_MODE:             return "DITHER";
        case kDISPLAY_TOUCH_STATE:             return "TOUCH_STATE";
        case kDISPLAY_ORITATION_STATE:         return "ORITATION_STATE";
        case kDISPLAY_CUP_BLACK_COVERED_STATE: return "CUP_BLACK_COVERED";
        case kDISPLAY_10BIT:                   return "10BIT";
        case kDISPLAY_SRE:                     return "SRE";
        default:                               return NULL;
    }
}

void DisplayEffectBase::dumpInfo(std::string& result)
{
    char res_ch[1024] = {0};
    ParamManager pmMgr;

    result.append("\nDF:\n");
    snprintf(res_ch, sizeof(res_ch), "Disp ID: %d, mEffectStatus: %d, Paper: %d, LUT: %d, GAME: %d, HDR: %d\n",
             mDisplayId, getEffectStatus(), mPaperModeId, mLutId, mGameModeId, mHDRStatus);
    result.append(std::string(res_ch));
    snprintf(res_ch, sizeof(res_ch), "Flat: %d, Scene: %d, ActApp: %d, Dark: %d, Game: %d\n",
             mFlatMode->mFlatModeStatus, mSceneId, curApp, mDarkMode, mGameModeId);
    result.append(std::string(res_ch));
    snprintf(res_ch, sizeof(res_ch), "FPS: current_mode_index: %d, current_fps: %d, settings_fps: %d, last_app_fps: %d, power_limit_fps: %d, thermal_limit_fps: %d\n",
             current_mode_index, current_fps, settings_fps, last_app_fps, power_limit_fps, thermal_limit_fps);
    result.append(std::string(res_ch));
    struct DispParam param;
    getDispParam(&param);
    result.append("\nDispParam:\n");
    snprintf(res_ch, sizeof(res_ch), "Case: %d, Mode: %d, Cookie: %d, ECLevel: %d\n",
             param.caseId, param.modeId, param.cookie, param.eyeCareLevel);
    result.append(std::string(res_ch));
    result.append("\nExpert:\n");
    snprintf(res_ch, sizeof(res_ch), "%s: %d  ", pmMgr.ExpertNode[EXPERT_NODE_ENABLE].c_str(),
        pmMgr.getParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(), pmMgr.ExpertNode[EXPERT_NODE_ENABLE].c_str()));
    result.append(std::string(res_ch));
    for (int i = EXPERT_NODE_GAMUT; i < EXPERT_NODE_MAX; i++) {
        snprintf(res_ch, sizeof(res_ch), "%s: %d  ", pmMgr.ExpertNode[i].c_str(), mCurParam.expertParam[i - EXPERT_NODE_GAMUT]);
        result.append(std::string(res_ch));
    }
    result.append("\n");

    if (mBrightness.get())
        mBrightness->dump(result);

    if (mPaperMode.get())
        mPaperMode->dump(result);

    if (mTrueTone.get())
        mTrueTone->dump(result);
#ifdef HAS_QDCM
    if (mDisplaySRE.get())
        mDisplaySRE->dump(result);
#endif
    if (mVideoMode.get())
        mVideoMode->dump(result);

    sp<DisplayFeatureState> pState = mStates[SET_PCC_MODE];
    if(pState != NULL && pState->isSupport()) {
        pState->dump(result);
    } else {
        ALOGE("---error not support this feature current!");
    }

    dumpStc(result);
}

void DisplayEffectBase::dumpStc(std::string& result)
{
#ifdef HAS_QDCM
    sp<IMiStcService> imistcservice;
    android::Parcel data, out_data;

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    data.writeInt32(mDisplayId);
    if (imistcservice.get()) {
        imistcservice->dispatch(IMiStcService::STC_DUMP, &data, &out_data);
    } else {
        ALOGE("Failed to acquire %s", "display.mistcservice");
    }
    result.append(out_data.readCString());
#endif
}

int DisplayEffectBase::CustomBrightnessProcess(int dbv)
{
    return 0;
}

void DisplayEffectBase::onMessageReceived(const sp<AMessage> &msg)
{
    DF_LOGV("%s %d", printfMessage(msg->what()), msg->what());
    int32_t caseId, value, cookie, generation;
    char BLValue[10];
    int realBrightness;
    int status = 0;
    struct DispParam param;
    getDispParam(&param);
    HBMStatusParams hbm_status = {0};
    Timer t;
    ParamManager pmMgr;
    int CABCValue = -1;

    if (msg->what() == kDISPLAY_ENV_ADAPTIVE_NORMAL ||
        msg->what() == kDISPLAY_COLOR_ENHANCE ||
        msg->what() == kDISPLAY_STANDARD) {
        if (getEffectStatus(DISPLAY_EXPERT)) {
            HandleExpert(EXPERT_DISABLE, 0);
            pmMgr.setParam(pmMgr.ExpertNode[EXPERT_NODE_NAME].c_str(),
                           pmMgr.ExpertNode[EXPERT_NODE_ENABLE].c_str(), 0);
        }
    }

    switch (msg->what()) {
        case kDISPLAY_ENV_ADAPTIVE_NORMAL:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            if (mDisplayState != kStateOff && !mGameModeId && !GetVideoModeId() && mSceneId == -1) {
                HandleACNormal(value, cookie);
            } else {
                setDispParam(0, AUTOMATIC_CONTRAST_STATE, value, cookie);
                status = 0x1 << DISPLAY_ENV_ADAPTIVE_NORMAL;
                setEffectStatus(status);
            }
            CABCValue = CABC_UI_ON;
            break;
        }
        case kDISPLAY_ENV_ADAPTIVE_COLD:
        {
            HandleACCold(0);
            break;
        }
        case kDISPLAY_ENV_ADAPTIVE_WARM:
        {
            HandleACWarm(0);
            break;
        }
        case kDISPLAY_COLOR_ENHANCE:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));

            if (mDisplayState != kStateOff && !mGameModeId && !GetVideoModeId() && mSceneId == -1) {
                HandleIncreasedContrast(0);
            } else {
                setDispParam(0, INCREASED_CONTRAST_STATE, value, cookie);
                setEffectStatus(0x1 << DISPLAY_COLOR_ENHANCE);
            }
            CABCValue = CABC_OFF;
            break;
        }
        case kDISPLAY_STANDARD:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));

            if (mDisplayState != kStateOff && !mGameModeId && !GetVideoModeId() && mSceneId == -1) {
                HandleStandard(0);
            } else {
                setDispParam(0, STANDARD_STATE, value, cookie);
                setEffectStatus(0x1 << DISPLAY_STANDARD);
            }
            CABCValue = CABC_OFF;
            break;
        }
        case kDISPLAY_BRIGHTNESS_NOTIFY:
        {
            break;
        }
        case kDISPLAY_EYECARE:
        {
            CHECK(msg->findInt32("Generation", &generation));
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            if (value == -1) {
                value = param.eyeCareLevel;
                HandleEyeCare(value, cookie);
            } else {
                if (abs(generation - mMessageGeneration[EYECARE_STATE]) <= 3) {
                    HandleEyeCare(value, cookie);
                }
            }
            break;
        }
        case kDISPLAY_CABC_MODE_SWITCH:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleCabcModeCustom(value, cookie);
            break;
        }
        case kDISPLAY_AD:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleAD(value, cookie);
            break;
        }
        case kDISPLAY_NIGHT_MODE:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleNightMode(value, cookie);
            break;
        }
        case kDISPLAY_HDR:
        {
            CHECK(msg->findInt32("Generation",&generation));
            CHECK(msg->findInt32("CaseId",&caseId));
            if (generation != mMessageGeneration[caseId]) {
                DF_LOGV("receive old message");
                return;
            }

            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleHDR(value, cookie);
            break;
        }
        case kDISPLAY_STATE_NOTIFY:
        {
            int32_t state;
            CHECK(msg->findInt32("state", &state));
            HandleDisplayState(state,0);
            break;
        }
        case kDISPLAY_DITHER_MODE:
        {
            CHECK(msg->findInt32("Value", &value));
            updateAodDitherStatus(value);
            break;
        }
        case kDISPLAY_EXT_COLOR_PROC:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleEXTColorProc(value, cookie);
            break;
        }
        case kDISPLAY_FOD_COLOR_MODE_P3:
        {
            struct OutputParam sParams;

            if (getEffectStatus(DISPLAY_EXPERT)) {
                sParams.function_id = DISPLAY_COLOR_MODE_EXPERT_P3;
                setDisplayEffect(SET_COLOR_MODE, &sParams);

                sParams.function_id = COLOR_PA;
                sParams.param1 = EXPERT_V;
                sParams.vec_payload.push_back(0);
                setDisplayEffect(SET_DSPP_MODE, &sParams);
            } else {
                sParams.function_id = DISPLAY_COLOR_MODE_ACNORMAL;
                setDisplayEffect(SET_COLOR_MODE, &sParams);
            }

            sParams.function_id = DISPLAY_CLEAR_PCC;
            setDisplayEffect(SET_PCC_MODE, &sParams);
            break;
        }
        case kDISPLAY_FOD_RESTORE_QCOM_MODE:
        {
            if (getDisplayState() == kStateOn) {
                restoreQcomMode();
            }
            break;
        }
        case kDISPLAY_FOD_SCREEN_ON:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            if (mBrightness.get()) {
                if (mBrightness->param.lastNoZeroBrightness <
                    mBrightness->param.manualMaxBl)
                    restoreBlPcc(value);
            }
            break;
        }
        case kDISPLAY_FOD_PRE_HBM:
        {
            break;
        }
        case kDISPLAY_CAMERA:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleCameraRequest(value, cookie);
            break;
        }
        case kDISPLAY_SET_CALLBACK:
        {
            void *callback = NULL;
            CHECK(msg->findPointer("callback", &callback));
            HandleRegisterCallback(callback);
            break;
        }
        case kDISPLAY_BCBC_CUSTOM:
        {
            CHECK(msg->findInt32("Value", &value));
#ifdef HAS_QDCM
            if (mDisplayBCBC.get())
                mDisplayBCBC->setBCBCFunction(value, 0);
#endif
            break;
        }
        case kDISPLAY_GAME:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleGameMode(value, cookie);
            break;
        }
        case kDISPLAY_SUNLIGHT_SCREEN:
        {
            CHECK(msg->findInt32("Generation",&generation));
            CHECK(msg->findInt32("CaseId",&caseId));
            if (generation != mMessageGeneration[caseId]) {
                ALOGD("receive old message");
                return;
            }
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleSunlightScreen(value, cookie);
        } break;
        case kDISPLAY_DC_BACKLIGHT:
        {
            CHECK(msg->findInt32("Generation",&generation));
            CHECK(msg->findInt32("CaseId",&caseId));
            if (generation != mMessageGeneration[caseId]) {
                ALOGD("receive old message");
                return;
            }
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleDCBacklight(value, cookie);
        } break;
        case kDISPLAY_COLOR_TEMPERATURE:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleColorTemp(value, cookie);
            break;
        }
        case kDISPLAY_DOZE_BRIGHTNESS_MODE:
        {
            CHECK(msg->findInt32("Value", &value));
            handleDozeBrightnessMode(value);
            break;
        }
        case kDISPLAY_EXPERT:
        {
            CHECK(msg->findInt32("Case", &caseId));
            CHECK(msg->findInt32("Value", &value));
            if (!mGameModeId && !GetVideoModeId() && mSceneId == -1) {
                HandleExpert(caseId, value);
            } else {
                if (caseId != EXPERT_CLEAR && caseId != EXPERT_DISABLE)
                    setDispParam(0, EXPERT_STATE, value, caseId);
            }
            CABCValue = CABC_OFF;
            break;
        }
        case kDISPLAY_VIDEO:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleVideoMode(value, cookie);
            break;
        }
        case kDISPLAY_SENSOR_CONTROL:
        {
            break;
        }
        case kDISPLAY_PAPER_MODE:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            if(value==-1)
                value=mPaperModeId;
            HandlePaperMode(value, cookie);
            break;
        }
        case kDISPLAY_STATE_NOTIFY_MUTUAL:
        {
            int32_t displayId, state;
            CHECK(msg->findInt32("display", &displayId));
            CHECK(msg->findInt32("state", &state));
            HandleDisplayMutualState(displayId,state);
            break;
        }
        case kDISPLAY_SCENE:
        {
            CHECK(msg->findInt32("modeId", &value));
            CHECK(msg->findInt32("cookie", &cookie));
            HandleDisplayScene(value, cookie);
            break;
        }
        case kDISPLAY_ACTIVE_APP:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleActiveApp(value, cookie);
            break;
        }
        case kDARK_MODE:
        {
            CHECK(msg->findInt32("modeId", &value));
            CHECK(msg->findInt32("cookie", &cookie));
            HandleDarkMode(value, cookie);
            break;
        }
        case kDISPLAY_TRUE_TONE:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleTrueTone(value, cookie);
            break;
        }
        case kADAPTIVE_HDR:
        {
            CHECK(msg->findInt32("modeId", &value));
            CHECK(msg->findInt32("cookie", &cookie));
            HandleAdaptiveHDR(value, cookie);
            break;
        }
        case kLOW_BRIGHTNESS_FOD:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleLowBrightnessFOD(value, cookie);
            break;
        }
        case kDISPLAY_SRE:
        {
            CHECK(msg->findInt32("modeId", &value));
            CHECK(msg->findInt32("value", &cookie));
#ifdef HAS_QDCM
            if (mDisplaySRE.get()) {
                mDisplaySRE->setSREFunction(value, cookie);
            }
#endif
            break;
        }
        case kDISPLAY_TOUCH_STATE:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleTouchState(value, cookie);
            break;
        }
        case kDISPLAY_10BIT:
        {
            CHECK(msg->findInt32("Generation",&generation));
            CHECK(msg->findInt32("CaseId",&caseId));
            if (generation != mMessageGeneration[caseId]) {
                DF_LOGV("receive old message");
                return;
            }
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            Handle10Bit(value, cookie);
            break;
        }
        case DisplayEffectBase::kDISPLAY_DOLBY_VISION_STATE:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleDVStatus(value, cookie);
            break;
        }
        case kDISPLAY_DATABASE:
        {
            int32_t value, cookie;
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleDataBase(value, cookie);
        } break;
        case kDISPLAY_C3D_LUT:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandleC3dLut(value, cookie);
            break;
        }
        case kDISPLAY_PIC_HDR:
        {
            CHECK(msg->findInt32("Value", &value));
            CHECK(msg->findInt32("Cookie", &cookie));
            HandlePicHDR(value, cookie);
            break;
        }
        default:
            DF_LOGE("Invalid message %d", msg->what());
            break;
    }
    if (CABCValue != -1)
        HandleCabcModeCustom(CABCValue, 0);
}
}
