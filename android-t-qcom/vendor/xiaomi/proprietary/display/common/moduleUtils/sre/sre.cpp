#include "display_property.h"
#include "DisplayEffectBase.h"
#include "sre.h"
#include "fpsmonitor.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "SRE"
#endif

using snapdragoncolor::IMiStcService;
namespace android {
int DFLOG::loglevel = 1;
static void SreSensorCallbackFunction(int event, const Event &info, void *user) {
    Sre *source = (Sre *) user;
    if (source) {
        source->sensorCallback(event, info);
    }
}

Sre::Sre(const sp<DisplayEffectBase>& display_effect)
      : mDisplayEffect(display_effect) {
    DF_LOGD("enter mHistControl = %d", mDisplayEffect->mHistControl);
    if (!mDisplayEffect.get()) {
        return;
    }

    displayId = mDisplayEffect->mDisplayId;
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
    mSensorCtrl.registerCallBack((void*)SreSensorCallbackFunction, this);

    mParseXml[displayId] = new MiParseXml();
    if (mParseXml[displayId].get()) {
        init();
    }

    if (mDisplayEffect->mHistControl) {
        mThreadHistExit = 0;
        mCPHistCond = PTHREAD_COND_INITIALIZER;
        mHistlock = PTHREAD_MUTEX_INITIALIZER;
        pthread_attr_t attr_hist;
        pthread_attr_init(&attr_hist);
        pthread_attr_setdetachstate(&attr_hist, PTHREAD_CREATE_JOINABLE);
        pthread_create(&mThreadSREHist, &attr_hist, ThreadWrapperSREHist, this);
        pthread_attr_destroy(&attr_hist);
        realIGCLevel = 0;
        mCPIGCCond = PTHREAD_COND_INITIALIZER;
        mIGClock = PTHREAD_MUTEX_INITIALIZER;
        pthread_attr_t attr_igc;
        pthread_attr_init(&attr_igc);
        pthread_attr_setdetachstate(&attr_igc, PTHREAD_CREATE_JOINABLE);
        pthread_create(&mThreadIGC, &attr_igc, ThreadWrapperSREIGC, this);
        pthread_attr_destroy(&attr_igc);
    }
}

void Sre::init()
{
    char temp[50];
    const unsigned int max_count = 1024;
    char *tokens[max_count] = {NULL};
    unsigned int count = 0;
    int ret = 0;
    char p[4096];

    string key("sreLevelNum");
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mSREConfigParams.sreLevelNum = atoi(p);
        DF_LOGV("mSREConfigParams.sreLevelNum: %d",atoi(p));
    }

    key = "sreLevel";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < mSREConfigParams.sreLevelNum; i++) {
            mSREConfigParams.sreLevels.push_back(atoi(tokens[i]));
            DF_LOGV("SRE: sreLevel sreLevels[%d]:%d",i,mSREConfigParams.sreLevels[i]);
        }
    }

    key = "hdrSreLevel";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < mSREConfigParams.sreLevelNum; i++) {
            mSREConfigParams.hdrSreLevels.push_back(atoi(tokens[i]));
            DF_LOGV("SRE: hdr sreLevels[%d]:%d",i,mSREConfigParams.hdrSreLevels[i]);
        }
    }

    SreGammaMap_T map;
    for (int i = 0; i < mSREConfigParams.sreLevelNum; i++) {
        key = "sreIgcLevel";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 15; j++) {
                map.map[j] = atoi(tokens[j]);
                DF_LOGV("SRE: sreGammaMap[%d][%d]:%d", i, j, map.map[j]);
            }
            mSREConfigParams.sreGammaMap.push_back((SreGammaMap_T)map);
        }
    }

    key = "sreExitThreshold";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mSREConfigParams.sreExitThreshold = atoi(p);
        DF_LOGV("SRE: sreExitThreshold:%d", mSREConfigParams.sreExitThreshold);
    }

    key = "hdrSreExitThreshold";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mSREConfigParams.hdrSreExitThreshold = atoi(p);
        DF_LOGV("SRE: hdrSreExitThreshold:%d", mSREConfigParams.sreExitThreshold);
    }

    key = "sreDivisor";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < mSREConfigParams.sreLevelNum; i++) {
            mSREConfigParams.sreDivisor.push_back(atoi(tokens[i]));
            DF_LOGV("SRE: sre devisor[%d]:%d",i,mSREConfigParams.sreDivisor[i]);
        }
    }
    key = "hdrSreDivisor";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < mSREConfigParams.sreLevelNum; i++) {
            mSREConfigParams.hdrSreDivisor.push_back(atoi(tokens[i]));
            DF_LOGV("SRE: hdr sre devisor[%d]:%d",i,mSREConfigParams.sreDivisor[i]);
        }
    }
    key = "sreHistZoneNum";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mSREConfigParams.sreHistZoneNum = atoi(p);
        DF_LOGV("SRE: sreHistZoneNum:%d", mSREConfigParams.sreHistZoneNum);
    }
    histData_T data;
    for (int i = 0; i < mSREConfigParams.sreHistZoneNum; i++) {
        key = "histData0Zone";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 8; j++) {
                data.histData[j] = atoi(tokens[j]);
                DF_LOGV("SRE: hist data0[%d][%d]:%d", j, i, data.histData[j]);
            }
            mSREConfigParams.histData0Zone.push_back(data);
        }
    }
    for (int i = 0; i < mSREConfigParams.sreHistZoneNum; i++) {
        key = "histData1Zone";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 8; j++) {
                data.histData[j] = atoi(tokens[j]);
                DF_LOGV("SRE: hist data1[%d][%d]:%d", j, i, data.histData[j]);
            }
            mSREConfigParams.histData1Zone.push_back(data);
        }
    }
    key = "OriginalModeGCIndex";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mSREConfigParams.originalModeGcIndex = atoi(p);
        DF_LOGV("SRE: originalModeGcIndex:%d", mSREConfigParams.originalModeGcIndex);
    }
    mGCIndexMap[DISPLAY_COLOR_MODE_sRGB] = mSREConfigParams.originalModeGcIndex;
    mGCIndexMap[DISPLAY_COLOR_MODE_EXPERT_SRGB] = mSREConfigParams.originalModeGcIndex;
    mGCIndexMap[DISPLAY_COLOR_MODE_EXPERT_P3] = mSREConfigParams.originalModeGcIndex;
    mGCIndexMap[DISPLAY_COLOR_MODE_EXPERT_WCG] = mSREConfigParams.originalModeGcIndex;
    mInitDone = 1;
    return;
}

int Sre::getSunlightLevel(int lux)
{
    int sunlightLevel = 0;
    int i = 0;
    for (i = 0; i < SUNLIGHT_SCREEN_LEVEL_MAX - 1; i++) {
        if (lux < mDisplayEffect->mPccParams.slsParams.luxThreshold[i]) {
            sunlightLevel = i;
            break;
        }
    }
    if (i == SUNLIGHT_SCREEN_LEVEL_MAX - 1)
        sunlightLevel = SUNLIGHT_SCREEN_LEVEL_MAX - 1;
    return sunlightLevel;
}

void Sre::sensorCallback(int event, const Event &info)
{
    int SLlevel = 0;
    if (mDisplayState != kStateOn) {
        return;
    }
    if ((event == SENSOR_TYPE_LIGHT_PRIMARY_SCREEN && displayId != DISPLAY_PRIMARY) ||
        (event == SENSOR_TYPE_LIGHT_SECOND_SCREEN && displayId != DISPLAY_SECONDARY) ||
        (event != SENSOR_TYPE_LIGHT_PRIMARY_SCREEN && event != SENSOR_TYPE_LIGHT_SECOND_SCREEN)) {
        return;
    }
    DF_LOGD("mDisplayId = %d dataCallback value=<%9.4f,%9.4f,%9.4f>,"
       "time=%ld, sensor=%d, temperature(K)=%9.4f", mDisplayEffect->mDisplayId,
       info.u.data[0], info.u.data[1], info.u.data[2],
       info.timestamp, info.sensorType, info.u.data[1]);
    setSREStrength((int)info.u.data[0], false);
}

int Sre::HandleSunlightScreen(int modeId, int cookie)
{
    int ret = -1;
    struct OutputParam sParams;
    if (!mSREEnabled)
        return 0;
    if (modeId != mLastSLPCCLevel || cookie > 0) {
        DF_LOGD("enter modeId %d, cookie %d", modeId, cookie);
        if (cookie == 0xEE || cookie == 0xFF) {
            mDisplayEffect->setBreakSLPCCDimming(true);
            usleep(50000);
        }
        mLastSLPCCLevel = modeId;
        sParams.function_id = DISPLAY_SUNLIGHT_SCREEN;
        sParams.param1 = modeId;
        if (kStateOn != mDisplayState)
            sParams.param2 = 0xFF;
        else
            sParams.param2 = cookie;
        ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
    }
    mLastSLPCCLevel = modeId;
    return ret;
}

int Sre::enableSRE(bool enable)
{
    int ret = 0;
    if (!mInitDone) {
        DF_LOGE("Failed to parse SRE param, cant not enable SRE");
        return -1;
    }
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
    if (enable) {
        DF_LOGD("enable SRE, displayId %d %d", displayId, mDisplayEffect->mDisplayId);
        mSREEnabled = true;
        mSensorCtrl.sensorControlOP(displayId, SENSOR_START, SENSOR_LIGHT, SRE);
    } else {
        DF_LOGD("disable SRE");
        mSensorCtrl.sensorControlOP(displayId, SENSOR_STOP, SENSOR_LIGHT, SRE);
        setSREStrength(0, true);
        mSREEnabled = false;
    }
    return ret;
}

int Sre::setSREFunction(int mode, int  cookie)
{
    int ret = 0;
    DF_LOGD("mDisplayId = %d", mDisplayEffect->mDisplayId);
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();

    switch (mode) {
        case LTM_OFF:
            DF_LOGD("LTM_OFF");
            ret = enableSRE(false);
            break;
        case LTM_ON:
             DF_LOGD("LTM_ON");
             ret = enableSRE(true);
             break;
        case LTM_STRENGTH:
        case LTM_VIDEO:
        case LTM_USERMODE:
            switch (cookie) {
                case LTM_USERMODE_DEFAULT:
                    DF_LOGW("setting LTM_USERMODE_DEFAULT= %d", cookie);
                    ret = setSREStrength(0, true);
                    enableSRE(true);
                    break;
                case LTM_USERMODE_GAME:
                case LTM_USERMODE_VIDEO:
                case 8000:
                case 19000:
                    mSensorCtrl.sensorControlOP(displayId, SENSOR_STOP, SENSOR_LIGHT, SRE);
                    mSREEnabled = true;
                    DF_LOGW("setting LTM_USERMODE= %d", cookie);
                    ret = setSREStrength(5000, true);
                    if (VIDEO_MODE_VIVID == mDisplayEffect->GetVideoModeId()) {
                        /* force sunlight pcc level to 5 */
                        HandleSunlightScreen(5, 0xEE);
                    } else if (GAME_MODE_BRIGHT == mDisplayEffect->getGameModeId()
                        || GAME_MODE_VIVID_BRIGHT == mDisplayEffect->getGameModeId()) {
                        HandleSunlightScreen(10, 0);
                    } else {
                        /* force sunlight pcc level to 5 */
                        HandleSunlightScreen(5, 0xEE);
                    }
                    break;
                default :
                    enableSRE(true);
                    ret = setSREStrength(mLastLux, false);
                    DF_LOGW("LTM_USERMODE_INDEX= %d, enable SRE.", cookie);
            }
            break;
        case LTM_HDR:
            mHDREnable = cookie;
            setSREStrength(mLastLux, false);
            break;
        default :
            return ret;
    }
    return ret;
}

int Sre::setSREStrength(int lux, bool freeze)
{
   int ret = -1;
    int value = 0;
    std::vector<int> sreLevels;
    int sreExitThreshold;
    std::vector<int> lastSreLevels;
    int lastExitThreshold = mLastExitThreshold;
    if (mLastSreLevels.size() != 0) {
        lastSreLevels.assign(mLastSreLevels.begin(), mLastSreLevels.end());
    } else {
        lastSreLevels.push_back(0);
        lastSreLevels.push_back(0);
    }
    if (mLastFreezeStatus && !freeze)
        return 0;
    else if (lux == 0)
        mLastFreezeStatus = false;
    else
        mLastFreezeStatus = freeze;
    DF_LOGD("mDisplayId = %d, mLastLux = %d, lux = %d, mSREEnabled = %d, mLastFreezeStatus = %d, freeze = %d",
         mDisplayEffect->mDisplayId, mLastLux, lux, mSREEnabled, mLastFreezeStatus, freeze);
    if (!mSREEnabled && !freeze && !mLastFreezeStatus) {
        DF_LOGV("SRE is closed");
        return ret;
    }
    if (mHDREnable) {
        sreLevels.assign(mSREConfigParams.hdrSreLevels.begin(), mSREConfigParams.hdrSreLevels.end());
        sreExitThreshold = mSREConfigParams.hdrSreExitThreshold;
    } else {
        sreLevels.assign(mSREConfigParams.sreLevels.begin(), mSREConfigParams.sreLevels.end());
        sreExitThreshold = mSREConfigParams.sreExitThreshold;
    }
    mLastSreLevels.assign(sreLevels.begin(), sreLevels.end());
    mLastExitThreshold = sreExitThreshold;
    if ((mLastLux >= lastExitThreshold && lux < sreExitThreshold && !freeze) || (lux == 0 && freeze)) {
        mLastLux = lux;
        DF_LOGD("close SRE");
        if (mHistStatus == HIST_STARTED)
            histogram_operation(HIST_CMD_STOP);
        luxLevelIndex = LUX_LEVEL_BELOW_5000;
        realIGCLevel = 0;
        HandleSunlightScreen(0, 0);
        pthread_cond_signal(&mCPIGCCond);
        return ret;
    } else if (mLastLux > lastSreLevels[1] && lux < sreLevels[1] && lux >= sreExitThreshold) {
        mLastLux = lux;
        luxLevelIndex = LUX_LEVEL_ABOVE_10000;
        if (mHistStatus == HIST_STOPPED)
            histogram_operation(HIST_CMD_START);
        pthread_cond_signal(&mCPHistCond);
        return ret;
    } else if (!freeze && lux < sreLevels[1]) {
        mLastLux = lux;
        return ret;
    }
    for (int i = mSREConfigParams.sreLevelNum - 1; i >= 0; i--) {
        if (lux >= sreLevels[i]) {
            luxLevelIndex =  (enum LUX_LEVEL_INDEX)i;
            break;
        }
    }
    if ( freeze)
        luxLevelIndex = LUX_LEVEL_ABOVE_5000;
    if (mHistStatus == HIST_STOPPED)
        histogram_operation(HIST_CMD_START);
    pthread_cond_signal(&mCPHistCond);
    mLastLux = lux;
    return ret;
}

void Sre::setDisplayState(int display_state)
{
    mDisplayState = display_state;
    if (mDisplayState != kStateOn && true == mSREEnabled) {
        DF_LOGV("Display isn't ON, close sre function");
        enableSRE(false);
        if (mLastSLPCCLevel > 0)
            HandleSunlightScreen(0, 0xFF);
    }
}

int Sre::getSREStatus()
{
    return mSREEnabled;
}

int Sre::histogram_operation(int cmd)
{
    int value = 0;
    struct OutputParam sParams;
    sParams.function_id = DISPLAY_FEATURE_HIST;
    if (GAME_MODE_BRIGHT == mDisplayEffect->getGameModeId()
            || GAME_MODE_VIVID_BRIGHT == mDisplayEffect->getGameModeId()) {
	sParams.param2 = GAME_SRE_FEATURE;
    } else {
        sParams.param2 = SRE_FEATURE;
    }
    Mutex::Autolock autoLock(mHistLock);
    switch ((enum HANDLE_HIST_CMD)cmd) {
        case HIST_CMD_START: {
            if (mHistStatus != HIST_CMD_STOP) {
                DF_LOGD("SRE histogram_operation-Start, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }
            sParams.param1 = HIST_CMD_START;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mHistStatus = HIST_STARTED;
        } break;
        case HIST_CMD_GET: {
            if (mHistStatus != HIST_STARTED) {
                DF_LOGD("SRE histogram_operation-Get, histgram status is wrong, status:%d!", mHistStatus);
                /* make sure SRE is closed to level 0 */
                realIGCLevel = 0;
                pthread_cond_signal(&mCPIGCCond);
                break;
            }
            sParams.param1 = HIST_CMD_GET;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            sre_hist_info = sParams.len;
        } break;
        case HIST_CMD_STOP: {
            if (mHistStatus != HIST_STARTED) {
                DF_LOGD("SRE histogram_operation-Stop, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }
            sParams.param1 = HIST_CMD_STOP;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mHistStatus = HIST_STOPPED;
        } break;
        default:
            DF_LOGD("SRE histogram_operation, cmd don't support!");
    }
    return mHistStatus;
}

void *Sre::ThreadWrapperSREHist(void *me) {
    return (void *)(uintptr_t)static_cast<Sre *>(me)->threadFuncSREHist();
}
int Sre::threadFuncSREHist() {
    int generation = 0;
    int ret = 0, value = 0;
    int pcc_diff = 0;
    int last_sre_hist_level = 0;
    int last_caseid_state = 0;
    int lastVideoModeId = 0;
    int lastGameModeId = 0;
    int lastLuxLevelIndex = LUX_LEVEL_BELOW_5000;
    int sre_hist_avr = 0, sre_hist_level = 0;
    struct DispParam param;
    int percent = 0, h1_percent =0, h2_percent =0, h3_percent =0;
    int histDataIndex = 0;
    std::vector<histData_T> data;
    while (mThreadHistExit == 0) {
        pthread_mutex_lock(&mHistlock);
        pthread_cond_wait(&mCPHistCond, &mHistlock);
        pthread_mutex_unlock(&mHistlock);
        lastLuxLevelIndex = LUX_LEVEL_BELOW_5000;
        usleep(20000);
        /*get histogram value*/
        while (histogram_operation(HIST_CMD_GET)) {
            /* SRE combine lux level and hist level then get the opt igc level */
            sre_hist_avr = sre_hist_info >> 24 & 0xFF;
            h1_percent =  sre_hist_info >> 16 & 0xFF;
            h2_percent =  sre_hist_info >> 8 & 0xFF;
            h3_percent =  sre_hist_info & 0xFF;
            DF_LOGD("SRE: hist avr %d, h1 %d, h2 %d, h3 %d, luxLevel %d",
                sre_hist_avr, h1_percent, h2_percent, h3_percent, (int)luxLevelIndex);
            if (luxLevelIndex <= LUX_LEVEL_ABOVE_10000) {
                data.assign(mSREConfigParams.histData0Zone.begin(), mSREConfigParams.histData0Zone.end());
                mDataZoneIndex = 0;
            } else {
                data.assign(mSREConfigParams.histData1Zone.begin(), mSREConfigParams.histData1Zone.end());
                mDataZoneIndex = 1;
            }
            if (sre_hist_avr == 0) {
                sre_hist_level = 0;
            } else if (sre_hist_avr <= data[0].histData[0]) {
                sre_hist_level = data[0].histData[4];
            } else if (sre_hist_avr <= data[1].histData[0]) {
                if (h1_percent > data[1].histData[1]) {
                    sre_hist_level = data[1].histData[4];
                } else if (h3_percent > data[1].histData[2]) {
                    sre_hist_level = data[1].histData[5];
                } else if (last_sre_hist_level == data[1].histData[5]
                        && h3_percent > data[1].histData[3]) {
                    sre_hist_level = data[1].histData[5];
                } else {
                    sre_hist_level = data[1].histData[6];
                }
            } else if (sre_hist_avr <= data[2].histData[0]) {
                if (h1_percent > data[2].histData[1]) {
                    sre_hist_level = data[2].histData[4];
                } else if (h1_percent < data[2].histData[2]) {
                    sre_hist_level = data[2].histData[5];
                } else if (last_sre_hist_level == data[2].histData[5]
                        && h1_percent < data[2].histData[3]) {
                    sre_hist_level = data[2].histData[5];
                } else {
                    sre_hist_level = data[2].histData[6];
                }
            } else if (sre_hist_avr <= data[3].histData[0]) {
                if (h1_percent > data[3].histData[1]) {
                    sre_hist_level = data[3].histData[4];
                } else if (h1_percent > data[3].histData[2]) {
                    if (last_sre_hist_level == data[3].histData[6]
                      && h1_percent < data[3].histData[3]) {
                        sre_hist_level = data[3].histData[5];
                    } else {
                        sre_hist_level = data[3].histData[6];
                    }
                } else {
                    sre_hist_level = data[3].histData[6];
                }
            } else if (sre_hist_avr <= data[4].histData[0]) {
                if (h1_percent > data[4].histData[1]) {
                    sre_hist_level = data[4].histData[4];
                } else if ((h1_percent + h2_percent) > data[4].histData[2]) {
                    sre_hist_level = data[4].histData[5];
                } else if (sre_hist_avr < data[4].histData[3]) {
                    sre_hist_level = data[4].histData[6];
                } else {
                    sre_hist_level = data[4].histData[7];
                }
            }
            mDisplayEffect->getDispParam(&param);
            if (lastLuxLevelIndex !=luxLevelIndex || last_sre_hist_level != sre_hist_level ||last_caseid_state != param.caseId
                || lastVideoModeId != mDisplayEffect->GetVideoModeId() || lastGameModeId != mDisplayEffect->getGameModeId()) {
                if (last_caseid_state != param.caseId)
                    usleep(20000);
                lastLuxLevelIndex = luxLevelIndex;
                last_sre_hist_level = sre_hist_level;
                last_caseid_state = param.caseId;
                lastVideoModeId = mDisplayEffect->GetVideoModeId();
                lastGameModeId = mDisplayEffect->getGameModeId();
                if (LUX_LEVEL_BELOW_5000 == luxLevelIndex)
                    realIGCLevel = 0;
                else
                    realIGCLevel = mSREConfigParams.sreGammaMap[lastLuxLevelIndex].map[last_sre_hist_level];
                pthread_cond_signal(&mCPIGCCond);
            }
            usleep(200000);
        }
    }
    return 0;
}

void *Sre::ThreadWrapperSREIGC(void *me) {
    return (void *)(uintptr_t)static_cast<Sre *>(me)->threadFuncSREIGC();
}
int Sre::threadFuncSREIGC() {
    int diff = 0;
    int tempIgc = 0;
    int mLastLevel = 0;
    int mNeedHueSatSet = 0;
    int lastVideoModeId = 0;
    struct OutputParam sParams;
    while (mThreadHistExit == 0) {
        pthread_mutex_lock(&mIGClock);
        pthread_cond_wait(&mCPIGCCond, &mIGClock);
        pthread_mutex_unlock(&mIGClock);
        DF_LOGW("mDisplayId = %d, mLastLevel=%d, realIGCLevel=%d",
               mDisplayEffect->mDisplayId, mLastLevel, realIGCLevel);

        mSreIgcOrigin = false;
        if (mDataZoneIndex == 1) {
            if (mLastLevel != mDataZoneIndex)
                mDimmingStep = 80;
            else
                mDimmingStep = 60;
        } else {
            if (mLastLevel != mDataZoneIndex)
                mDimmingStep = 60;
            else
                mDimmingStep = 40;
        }
        mLastLevel = mDataZoneIndex;
        if (0 == (sre_hist_info >> 24 & 0xFF) || kStateOn != mDisplayState ||realIGCLevel ==0) {
            /* set to default gamma */
            SetSREToSTC(0);
            /* restore vsdr2hdr status */
            mSreIgcOrigin = true;
            continue;
        }
        /* SRE set stc logical */
        SetSREToSTC(1);
    }
    return 0;
}

void Sre::setSreGCIndex(int index)
{
    sp<IMiStcService> imistcservice;
    android::Parcel data;

    data.writeInt32(mDisplayEffect->mDisplayId);
    data.writeInt32(index);
    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    if (imistcservice.get()) {
        imistcservice->dispatch(IMiStcService::SET_SRE_GC, &data, NULL);
    } else {
        DF_LOGW("Failed to acquire %s", "display.mistcservice");
    }
    mGcIndex = index;
}

int Sre::getGCIndex(int modeId)
{
    auto Index = mGCIndexMap.find(modeId);
    if (Index != mGCIndexMap.end()) {
        return Index->second;
    } else {
        return -1;
    }
}

void Sre::setDisplayEffect(int modeId)
{
    int index = getGCIndex(modeId);
    setSreGCIndex(index);
    mCurModeId = modeId;
}

void Sre::SetSREToSTC(int enable)
{
    int ret = 0;
    sp<IMiStcService> imistcservice;
    android::Parcel data;
    int divisor = 0;

    if (!enable && !mSreStatus)
        return;

    FpsMonitor & FM = FpsMonitor::getInstance();
    FM.checkFps(1);

    if (enable) {
        if (mHDREnable) {
            divisor = mSREConfigParams.hdrSreDivisor[(int)luxLevelIndex];
        } else {
            divisor = mSREConfigParams.sreDivisor[(int)luxLevelIndex];
        }
    }
    DF_LOGD("SRE: enable %d, hdr %d, lut level %d, divisor %d", enable, mHDREnable, realIGCLevel, divisor);
    data.writeInt32(enable);
    data.writeInt32(mDisplayEffect->mDisplayId);
    data.writeInt32(mDimmingStep);
    data.writeInt32(realIGCLevel);
    data.writeInt32(divisor);
    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    if (imistcservice.get()) {
        imistcservice->dispatch(IMiStcService::SET_SRE, &data, NULL);
    } else {
        DF_LOGW("Failed to acquire %s", "display.mistcservice");
    }
    mSreStatus = enable;
    mDivisor = divisor;
}

void Sre::dump(std::string& result)
{
    char res_ch[1024] = {0};
    result.append("\nSRE:\n");
    snprintf(res_ch, sizeof(res_ch), "Disp ID: %d, Enable: %d, Status: %d, Divisor: %d, Lux: %d, GCIndex: %d\n",
                             displayId, mSREEnabled, mSreStatus, mDivisor, mLastLux, mGcIndex);
    result.append(std::string(res_ch));
}

} //namespace android
