#include "truetone.h"
#include "timer.hpp"
#include "DisplayEffectBase.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "TRUETONE"
#endif

namespace android {
int DFLOG::loglevel = 1;
static void TrueToneSensorCallback(int event, const Event &info, void* user)
{
    TrueTone * source = (TrueTone *) user;
    if (source)
        source->sensorCallback(event, info);
}

void TimerCallbackFunction(void *user, int caseId ,int generation, int value)
{
    TrueTone *source = (TrueTone *) user;
    source->timerCallback(caseId, generation, value);
}

TrueTone::TrueTone(const sp<DisplayEffectBase>& display_effect)
      : mDisplayEffect(display_effect) {
    if (mDisplayEffect.get()) {
        displayId = mDisplayEffect->mDisplayId;
    }
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
    mSensorCtrl.registerCallBack((void*)TrueToneSensorCallback, this);

    mParseXml[displayId] = new MiParseXml();
    if (mParseXml[displayId].get()) {
        init();
    }
}

TrueTone::~TrueTone()
{

}

void TrueTone::init()
{
    char temp[50];
    const unsigned int max_count = 1024;
    char *tokens[max_count] = {NULL};
    unsigned int count = 0;
    int ret = 0;
    char p[4096];

    string key("appZoneNum");
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mAppZoneNum = atoi(p);
        DF_LOGV("mAppZoneNum: %d",atoi(p));
        if (mAppZoneNum > 0)
            pAppZones = (struct TroneToneZone*)malloc(mAppZoneNum * sizeof(struct TroneToneZone));
    }

    for (int i = 0; i < mAppZoneNum; i++) {
        key = "appZone";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            pAppZones[i].start = atoi(tokens[0]);
            pAppZones[i].end = atoi(tokens[1]);
            pAppZones[i].coeffA = atof(tokens[2]);
            pAppZones[i].coeffB = atof(tokens[3]);
            pAppZones[i].coeffC = atof(tokens[4]);
            pAppZones[i].coeffN = atoi(tokens[5]);
            DF_LOGV("i %d, apple zone %f %f %f %f %f %f ",
                     i, pAppZones[i].start, pAppZones[i].end, pAppZones[i].coeffA, pAppZones[i].coeffB, pAppZones[i].coeffC, pAppZones[i].coeffN);
        }
    }
    key = "cctZoneNum";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mCCTZoneNum = atoi(p);
        DF_LOGV("mCCTZoneNum: %d",atoi(p));
        if (mCCTZoneNum > 0)
            pCCTZones = (struct TroneToneZone*)malloc(mCCTZoneNum * sizeof(struct TroneToneZone));
    }
    for (int j = 0; j < mCCTZoneNum; j++) {
        key = "cctZone";
        sprintf(temp, "%d", j);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            pCCTZones[j].start = atoi(tokens[0]);
            pCCTZones[j].end = atoi(tokens[1]);
            pCCTZones[j].coeffA = atof(tokens[2]);
            pCCTZones[j].coeffN = atoi(tokens[3]);
            DF_LOGV("j %d, cct zone %f %f %f %f",j, pCCTZones[j].start,pCCTZones[j].end, pCCTZones[j].coeffA, pCCTZones[j].coeffN);
        }
    }
    key = "cctThresholdNum";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mCCTThresholdNum = atoi(p);
        DF_LOGV("mCCTThresholdNum: %d",atoi(p));
        if (mCCTThresholdNum > 0)
            pCCTThreshold = (struct TroneToneZone*)malloc(mCCTThresholdNum * sizeof(struct TroneToneZone));
    }
    for (int j = 0; j < mCCTThresholdNum; j++) {
        key = "cctThresholdZone";
        sprintf(temp, "%d", j);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            pCCTThreshold[j].start = atoi(tokens[0]);
            pCCTThreshold[j].end = atoi(tokens[1]);
            pCCTThreshold[j].coeffN = atoi(tokens[2]);
            DF_LOGV("j %d, pCCTThreshold %f %f %f",j, pCCTThreshold[j].start, pCCTThreshold[j].end, pCCTThreshold[j].coeffN);
        }
    }
    key = "CCTLuxThreshold";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mCCTLuxThreshold = atoi(p);
        DF_LOGV("mCCTLuxThreshold: %d",atoi(p));
    }
    for (int i = 0; i < 9; i++) {
        key = "D65Coeff";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 4; j++) {
                mPccParam.D65Coeffs[i][j] = atof(tokens[j]);
                DF_LOGV("pcc truetone D65Coeffs %d %d %f ",i, j, atof(tokens[j]));
            }
        }
    }
    for (int i = 0; i < 9; i++) {
        key = "D75Coeff";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 4; j++) {
                mPccParam.D75Coeffs[i][j] = atof(tokens[j]);
                DF_LOGV("pcc truetone D75Coeffs %d %d %f ",i, j, atof(tokens[j]));
            }
        }
    }
    key = "TrueTonedimmingTime";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mCctDimmingTime = atoi(p);
        DF_LOGV("dimmingTime: %d",atoi(p));
    }
    key = "cctStepMs";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mCctStepMs = atoi(p);
        DF_LOGV("dimmingTime: %d",atoi(p));
    }

    key = "nativeCCTZone";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < 2; i++) {
            mPccParam.NativeCCTZone[i] = atoi(tokens[i]);
            DF_LOGV("nativeCCTZone[%d]=%d ",i, mPccParam.NativeCCTZone[i]);
        }
    }

    key = "TrueToneBlCalibExpLvl";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        mBlCalibExpLvl = 0;
    } else {
        mBlCalibExpLvl = atoi(p);
        DF_LOGV("mBlCalibExpLvl: %d",atoi(p));
    }

    if (mBlCalibExpLvl > 0) {
        key = "TrueToneBlCalibCoeff";
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            if (count != mBlCalibExpLvl * 2) {
                DF_LOGE("parse %s err param count %d", key.c_str(), count);
                return;
            }
            for (int i = 0; i < mBlCalibExpLvl * 2; i++) {
                mBlCalibCoeff.push_back(atof(tokens[i]));
                DF_LOGV("mBlCalibCoeff[%d]=%f",i, mBlCalibCoeff[i]);
            }
        }
    }

    key = "TrueToneBlCalibThresh";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mBlCalibThresh = atoi(p);
        DF_LOGV("mBlCalibThresh: %d",atoi(p));
    }

    key = "TrueToneBlCalibCCTThresh";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mBlCalibCctThresh = atoi(p);
        DF_LOGV("mBlCalibCctThresh: %d",atoi(p));
    }
    mInitDone = 1;
    return;
}

void TrueTone::HandleTouchState(int caseId, int value)
{
    DF_LOGV("%d", caseId);
    Timer t;
    if (mEnableTrueTone) {
        mTouchState = caseId;
        if (caseId) {
            mSkipCCT = caseId;
        } else {
            mTimerGeneration[TIMER_CCT_ENABLE]++;
            t.AsyncWait(2000, TimerCallbackFunction, this, TIMER_CCT_ENABLE, mTimerGeneration[TIMER_CCT_ENABLE], 0);
        }
    }
}

int TrueTone::enableTrueTone(int modeId, int cookie)
{
    Timer t;
    int ret = 0;
    if (!mInitDone) {
        DF_LOGE("Failed to parse truetone param, cant not enable truetone");
        return -1;
    }
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
    switch (modeId) {
        case 1: {
            mEnableTrueTone = 1;
            mTruetoneDebug = 0;
            if (mTruetoneOnOffdimming <= TRUE_TONE_ON_OFF_DIMMING) {
                mTimerGeneration[TIMER_TRUE_TONE_ON]++;
                t.AsyncWait(32, TimerCallbackFunction, this,
                    TIMER_TRUE_TONE_ON, mTimerGeneration[TIMER_TRUE_TONE_ON], mTruetoneOnOffdimming);
                return ret;
            }
            mSensorCtrl.sensorControlOP(displayId, SENSOR_START, SENSOR_FRONT_CCT, TRUETONE);
            mSensorCtrl.sensorControlOP(displayId, SENSOR_START, SENSOR_LIGHT, TRUETONE);
            if (mlastlux < mCCTLuxThreshold) {
                Mutex::Autolock autoLock(mTruetoneLock);
                HandleCCT(6500);
            }
        } break;
        case 0: {
            mEnableTrueTone = 0;
            mTruetoneDebug = 0;
            mTouchState = 0;
            mSensorCtrl.sensorControlOP(displayId, SENSOR_STOP, SENSOR_FRONT_CCT, TRUETONE);
            mSensorCtrl.sensorControlOP(displayId, SENSOR_STOP, SENSOR_LIGHT, TRUETONE);
            if (mTruetoneOnOffdimming <= TRUE_TONE_ON_OFF_DIMMING) {
                mTimerGeneration[TIMER_TRUE_TONE_OFF]++;
                t.AsyncWait(32, TimerCallbackFunction, this,
                    TIMER_TRUE_TONE_OFF, mTimerGeneration[TIMER_TRUE_TONE_OFF], mTruetoneOnOffdimming);
                return ret;
            }
            trueToneClose();
        } break;
        case 2: {
            if (cookie == 1)
                mAppleLikeGallery = 1;
            else
                mAppleLikeGallery = 0;
        } break;
        case 3: {
            mTruetoneDebug = 1;
            mSensorX = (float)cookie / 10000;
        } break;
        case 4: {
            mTruetoneDebug = 1;
            mSensorY = (float)cookie / 10000;
        } break;
        case 5: {
            mTruetoneDebug = 1;
            mSensorCCT = cookie;
            HandleCCT(0);
        } break;
        default: {
            DF_LOGE("error modeId: %d cookie: %d", modeId, cookie);
        } break;
    }

    return ret;
}

int TrueTone::HandleCCT(int forcecct)
{
    int ret = 0;
    float cct = 0.0;
    Timer t;
    int cctThreshold = 0;

    cct = mTargetCCT;

    if (mSkipCCT) {
        DF_LOGD("touch state %d, skip cct %d", mTouchState, mSkipCCT);
        return ret;
    }

    if (forcecct) {
        mTargetCCT = forcecct;
        mTimeZones.coeffA = 11.018;
        mTimeZones.coeffB = (mTargetCCT - mLastCCT - 176.288) / 4;
        mTimeZones.coeffC = mLastCCT;
        DF_LOGD("force cct %f a %f b %f c %f", mTargetCCT, mTimeZones.coeffA, mTimeZones.coeffB, mTimeZones.coeffC);
        mTimerGeneration[TIMER_TRUE_TONE_ON]++;
        t.AsyncWait(mCctStepMs, TimerCallbackFunction, this,
            TIMER_TRUE_TONE_ON, mTimerGeneration[TIMER_TRUE_TONE_ON], 254);

        return ret;
    }

    if (mAppleLikeGallery) {
        for (int i = 0; i < mAppZoneNum; i++) {
            if (mSensorCCT >= pAppZones[i].start && mSensorCCT < pAppZones[i].end) {
                cct = pAppZones[i].coeffA * pow(mSensorCCT,3) + pAppZones[i].coeffB * pow(mSensorCCT,2) + pAppZones[i].coeffC * mSensorCCT + pAppZones[i].coeffN;
                break;
            }
        }
    }else {
        for (int i = 0; i < mCCTZoneNum; i++) {
            if (mSensorCCT >= pCCTZones[i].start && mSensorCCT < pCCTZones[i].end) {
                cct = pCCTZones[i].coeffA * mSensorCCT + pCCTZones[i].coeffN;
                break;
            }
        }
    }

    for (int i = 0; i < mCCTThresholdNum; i++) {
        if (cct >= pCCTThreshold[i].start && cct < pCCTThreshold[i].end) {
            cctThreshold = (int)pCCTThreshold[i].coeffN;
            break;
        }
    }
    if (abs(mTargetCCT - cct) >= cctThreshold) {
        mTargetCCT = cct;

        mTimeZones.coeffA = 11.018;
        mTimeZones.coeffB = (mTargetCCT - mLastCCT - 176.288) / 4;
        if (mLastCCT < 0) {
            if (mDisplayTruetoneStatus == TRUETONE_D65)
                mLastCCT = mPccParam.NativeCCTZone[0];
            else
                mLastCCT = mPccParam.NativeCCTZone[1];
        }
        mTimeZones.coeffC = mLastCCT;

        DF_LOGD("target cct %f a %f b %f c %f", mTargetCCT, mTimeZones.coeffA, mTimeZones.coeffB, mTimeZones.coeffC);
        mTimerGeneration[TIMER_TRUE_TONE_ON]++;
        t.AsyncWait(mCctStepMs, TimerCallbackFunction, this,
            TIMER_TRUE_TONE_ON, mTimerGeneration[TIMER_TRUE_TONE_ON], 254);
    }

    return ret;
}

int TrueTone::trueToneClose()
{
    int ret = 0;
    Timer t;

    if (mDisplayTruetoneStatus == TRUETONE_D65)
        mTargetCCT = mPccParam.NativeCCTZone[0];
    else
        mTargetCCT = mPccParam.NativeCCTZone[1];

    mTimeZones.coeffA = 11.018;
    mTimeZones.coeffB = (mTargetCCT - mLastCCT - 176.288) / 4;
    mTimeZones.coeffC = mLastCCT;
    DF_LOGD("target cct %f a %f b %f c %f", mTargetCCT, mTimeZones.coeffA, mTimeZones.coeffB, mTimeZones.coeffC);
    mTimerGeneration[TIMER_TRUE_TONE_OFF]++;
    t.AsyncWait(mCctStepMs, TimerCallbackFunction, this,
                TIMER_TRUE_TONE_OFF, mTimerGeneration[TIMER_TRUE_TONE_OFF], 254);

    return ret;
}

int TrueTone::timerCallback(int caseId, int generation, int value)
{
    DF_LOGD("enter caseId %d, value %d, generation %d, mGeneration %d",
             caseId, value, generation, mTimerGeneration[caseId]);
    int ret = -1;
    Timer t;
    static float ontime = 0.0;
    static float closetime = 0.0;
    struct OutputParam sParams;

    /* sensor trigger Timer, clear ontime here*/
    if (254 == value) {
        ontime = 0.0;
        closetime = 0.0;
    }

    if (generation != mTimerGeneration[caseId])
        return 0;
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();

    switch (caseId) {
        case TIMER_TRUE_TONE_ON: {
            if (value <= TRUE_TONE_ON_OFF_DIMMING + 1) {
                if (mEnableTrueTone) {
                    if (mTruetoneOnOffdimming <= TRUE_TONE_ON_OFF_DIMMING) {
                        setPCCConfigTrueTone(mTruetoneOnOffdimming, mDisplayTruetoneStatus);
                        mTruetoneOnOffdimming++;
                        mTimerGeneration[TIMER_TRUE_TONE_ON]++;
                        t.AsyncWait(32, TimerCallbackFunction, this, TIMER_TRUE_TONE_ON,
                            mTimerGeneration[TIMER_TRUE_TONE_ON], mTruetoneOnOffdimming);
                    } else {
                        mSensorCtrl.sensorControlOP(displayId, SENSOR_START, SENSOR_FRONT_CCT, TRUETONE);
                        mSensorCtrl.sensorControlOP(displayId, SENSOR_START, SENSOR_LIGHT, TRUETONE);
                        if (mDisplayTruetoneStatus == TRUETONE_D65)
                            mLastCCT = mPccParam.NativeCCTZone[0];
                        else
                            mLastCCT = mPccParam.NativeCCTZone[1];
                        if (mlastlux < mCCTLuxThreshold) {
                            Mutex::Autolock autoLock(mTruetoneLock);
                            HandleCCT(6500);
                        }
                        return ret;
                    }
                }
            } else {
                if (ontime > mCctDimmingTime || mDisplayState != kStateOn) {
                    ontime = 0.0;
                    DF_LOGD("truetone on target %f, now cct %f", mTargetCCT, mLastCCT);
                    return ret;
                }
                if (mEnableTrueTone) {
                    ontime += (float)mCctStepMs / 1000;
                    mLastCCT = mTimeZones.coeffA * pow(ontime,2) + mTimeZones.coeffB * ontime + mTimeZones.coeffC;
                    setPCCConfigTrueTone((int)mLastCCT, mDisplayTruetoneStatus);
                    HandleCCTBacklight(mLastCCT, 1);
                    mTimerGeneration[TIMER_TRUE_TONE_ON]++;
                    t.AsyncWait(mCctStepMs, TimerCallbackFunction, this, TIMER_TRUE_TONE_ON, mTimerGeneration[TIMER_TRUE_TONE_ON], 255);
                    return ret;
                }
                ontime = 0.0;
            }
        } break;
        case TIMER_TRUE_TONE_OFF: {
            if (value <= TRUE_TONE_ON_OFF_DIMMING + 1) {
                if (!mEnableTrueTone) {
                    if (mTruetoneOnOffdimming != 0) {
                        mTruetoneOnOffdimming--;
                        setPCCConfigTrueTone(mTruetoneOnOffdimming, mDisplayTruetoneStatus);
                        mTimerGeneration[TIMER_TRUE_TONE_OFF]++;
                        t.AsyncWait(32, TimerCallbackFunction, this, TIMER_TRUE_TONE_OFF, mTimerGeneration[TIMER_TRUE_TONE_OFF], mTruetoneOnOffdimming);
                    } else {
                        return ret;
                    }
                }
            } else {
                if (closetime > mCctDimmingTime || mDisplayState != kStateOn) {
                    closetime = 0.0;

                    HandleCCTBacklight(0, 0);

                    mTimerGeneration[TIMER_TRUE_TONE_OFF]++;
                    t.AsyncWait(32, TimerCallbackFunction, this, TIMER_TRUE_TONE_OFF, mTimerGeneration[TIMER_TRUE_TONE_OFF], mTruetoneOnOffdimming);
                    DF_LOGD("truetone off target %f, now cct %f", mTargetCCT, mLastCCT);
                    return ret;
                }
                if (!mEnableTrueTone) {
                    closetime += (float)mCctStepMs / 1000;
                    mLastCCT = mTimeZones.coeffA * pow(closetime,2) + mTimeZones.coeffB * closetime + mTimeZones.coeffC;
                    setPCCConfigTrueTone((int)mLastCCT, mDisplayTruetoneStatus);
                    HandleCCTBacklight(mLastCCT, 1);
                    mTimerGeneration[TIMER_TRUE_TONE_OFF]++;
                    t.AsyncWait(mCctStepMs, TimerCallbackFunction, this, TIMER_TRUE_TONE_OFF, mTimerGeneration[TIMER_TRUE_TONE_OFF], 255);
                    return ret;
                }
                closetime = 0.0;
            }
        } break;
        case TIMER_CCT_ENABLE: {
            if (mEnableTrueTone) {
                if (!mTouchState) {
                    mSkipCCT = 0;
                    HandleCCT(0);
                }
            } else {
                mSkipCCT = 0;
            }
        } break;
        default:
            break;
    }

    return ret;
}

int TrueTone::setPCCConfigTrueTone(int cct, int mode)
{
    double Coeffs[9][4];
    double dimmingStep[ARRAY_SIZE_EXP] = {0};
    int nativeCCT = 0;
    int ret = 0;
    struct OutputParam sParams;

    DF_LOGD("cct %d, mode %d", cct, mode);
#ifndef MTK_PLATFORM
    FpsMonitor & FM = FpsMonitor::getInstance();
    FM.checkFps(1);
#endif
    switch (( enum DISPLAY_TRUETONE_STATUS)mode) {
        case TRUETONE_NORMAL: {
            memcpy(Coeffs, mPccParam.D75Coeffs, sizeof(Coeffs));
            nativeCCT = mPccParam.NativeCCTZone[1];
        } break;
        case TRUETONE_D65: {
            memcpy(Coeffs, mPccParam.D65Coeffs, sizeof(Coeffs));
            nativeCCT = mPccParam.NativeCCTZone[0];
        } break;
        default: {
            DF_LOGE("error DISPLAY_TRUETONE_STATUS ...");
            return ret;
        }
    }

    if (cct <= TRUE_TONE_ON_OFF_DIMMING) {
        for (int i = 0; i < ARRAY_SIZE_EXP; i++) {
            dimmingStep[i] = Coeffs[i][0] * nativeCCT * nativeCCT * nativeCCT \
                         + Coeffs[i][1] * nativeCCT * nativeCCT \
                         + Coeffs[i][2] * nativeCCT \
                         + Coeffs[i][3];
            if (i%4 == 0)
                dimmingStep[i] = (1.0 - dimmingStep[i])/TRUE_TONE_ON_OFF_DIMMING;
            else
                dimmingStep[i] = dimmingStep[i]/TRUE_TONE_ON_OFF_DIMMING;
        }
        for (int i = 0; i < ARRAY_SIZE_EXP; i++) {
            if (i%4 == 0)
                mTrueToneCoeff[i] = 1.0 - dimmingStep[i] * cct;
            else
                mTrueToneCoeff[i] = dimmingStep[i] * cct;
        }
    } else {
        for (int j = 0; j < ARRAY_SIZE_EXP; j++) {
            mTrueToneCoeff[j] = Coeffs[j][0] * cct * cct * cct \
                         + Coeffs[j][1] * cct * cct \
                         + Coeffs[j][2] * cct \
                         + Coeffs[j][3];
            if (j % 4 == 0 && mTrueToneCoeff[j] > 1.0)
                mTrueToneCoeff[j] = 1.0;
        }
    }

    DF_LOGD("truetone cct %d mode %d mTrueToneCoeff:R[r %f, g %f, b %f], G[r %f, g %f, b %f], B[r %f, g %f, b %f]",
        cct, mode, mTrueToneCoeff[0],mTrueToneCoeff[1],mTrueToneCoeff[2],mTrueToneCoeff[3],mTrueToneCoeff[4], mTrueToneCoeff[5], mTrueToneCoeff[6],mTrueToneCoeff[7],mTrueToneCoeff[8]);
    sParams.function_id = DISPLAY_TRUE_TONE;
    for (int j = 0; j < ARRAY_SIZE_EXP; j++) {
        sParams.pcc_payload.push_back(mTrueToneCoeff[j]);
    }
    ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);

    return ret;
}

void TrueTone::sensorCallback(int event, const Event &info)
{
    if (event == SENSOR_TYPE_FRONT_CCT || event == SENSOR_TYPE_VIRTUAL_CCT) {
        DF_LOGD("Cct value=<%9.4f>", info.u.data[0]);
        if (!mTruetoneDebug) {
            if (mEnableTrueTone) {
                mSensorCCT = info.u.data[0];
                if (mlastlux > mCCTLuxThreshold) {
                    Mutex::Autolock autoLock(mTruetoneLock);
                    HandleCCT(0);
                }
            }
        }
    } else if ((event == SENSOR_TYPE_LIGHT_PRIMARY_SCREEN && displayId == DISPLAY_PRIMARY) ||
               (event == SENSOR_TYPE_LIGHT_SECOND_SCREEN && displayId == DISPLAY_SECONDARY)) {
        float lux;
        lux = info.u.data[0];

        DF_LOGV("sensor lux =<%9.4f>", info.u.data[0]);

        if (!mTruetoneDebug) {
            if (mEnableTrueTone && info.u.data[0] < mCCTLuxThreshold) {
                if((mlastlux > mCCTLuxThreshold && mTargetCCT > 6500) || (mCctDisplayState && abs(mlastlux - info.u.data[0]) >= 100)) {
                    Mutex::Autolock autoLock(mTruetoneLock);
                    HandleCCT(6500);
                    DF_LOGV("lux restrict cct to 6500");
                }
            } else if (mEnableTrueTone && (mlastlux < mCCTLuxThreshold && info.u.data[0] > mCCTLuxThreshold)
                    && (mTruetoneOnOffdimming > TRUE_TONE_ON_OFF_DIMMING + 1)) {
                Mutex::Autolock autoLock(mTruetoneLock);
                HandleCCT(0);
            }
        }
        mlastlux = lux;
        mCctDisplayState = false;
    }
}

void TrueTone::SetDisplayState(int display_state)
{
    if (mEnableTrueTone) {
        SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
        if (display_state != kStateOn && mDisplayState == kStateOn) {
            mSensorCtrl.sensorControlOP(displayId, SENSOR_STOP, SENSOR_FRONT_CCT, TRUETONE);
            mSensorCtrl.sensorControlOP(displayId, SENSOR_STOP, SENSOR_LIGHT, TRUETONE);
        } else if (display_state == kStateOn && mDisplayState != kStateOn) {
            mCctDisplayState = true;
            mSensorCtrl.sensorControlOP(displayId, SENSOR_START, SENSOR_FRONT_CCT, TRUETONE);
            mSensorCtrl.sensorControlOP(displayId, SENSOR_START, SENSOR_LIGHT, TRUETONE);
        }
    }
    mDisplayState = display_state;
}

void TrueTone::HandleCCTBacklight(float cct, int enable)
{
    float blCoeff = 0;
    int base = 0;
    if (enable) {
        if (mDisplayEffect->mVirtualBl >= mBlCalibThresh && cct > mBlCalibCctThresh) {
            if (mDisplayTruetoneStatus == TRUETONE_NORMAL)
                base = 4;
            for (int i = 0; i < mBlCalibExpLvl; i++) {
                blCoeff += mBlCalibCoeff[i + base] * pow(cct, i);
            }
            if (mBlCalibExpLvl != 0)
                mCCTBacklightCoeff = blCoeff;

            mDisplayEffect->updateBacklightCoeff(mCCTBacklightCoeff);
        }
    } else {
        mCCTBacklightCoeff = 1.0;
        mDisplayEffect->updateBacklightCoeff(mCCTBacklightCoeff);
    }

    DF_LOGD("mCCTBacklightCoeff %f", mCCTBacklightCoeff);
}

void TrueTone::setActiveApp(int modeId)
{
    if (mlastlux > mCCTLuxThreshold) {
        if ((modeId == GALLERY || modeId == CAMERA) && (mCurApp != GALLERY && mCurApp != CAMERA)) {
            mAppleLikeGallery = 1;
            if(mEnableTrueTone)
                HandleCCT(0);
        } else if ((modeId != GALLERY && modeId != CAMERA) && (mCurApp == GALLERY || mCurApp == CAMERA)) {
            mAppleLikeGallery = 0;
            if(mEnableTrueTone)
                HandleCCT(0);
        }
    }
    mCurApp = modeId;
}

int TrueTone::HandleTrueToneStatus(int mode)
{
    int ret = 0;
    struct OutputParam sParams;

    DF_LOGD("enter mode %d", mode);

    if (mode == mDisplayTruetoneStatus)
        return ret;

    mDisplayTruetoneStatus = mode;

    if (mEnableTrueTone) {
        setPCCConfigTrueTone((int)mLastCCT, mDisplayTruetoneStatus);
        HandleCCTBacklight(mLastCCT, 1);
    } else if (mTruetoneOnOffdimming != 0) {
        mTruetoneOnOffdimming = TRUE_TONE_ON_OFF_DIMMING + 1;
        trueToneClose();
    }

    return ret;
}

int TrueTone::getEffectStatus(int index)
{
    return (mEffectStatus>>index)&0x1;
}

void TrueTone::updateTrueToneStatus(int status)
{
    mEffectStatus = status;

    if (getEffectStatus(DISPLAY_STANDARD) ||
        getEffectStatus(DISPLAY_HDR)) {
        HandleTrueToneStatus(TRUETONE_D65);
    } else if (getEffectStatus(DISPLAY_ENV_ADAPTIVE_NORMAL) ||
        getEffectStatus(DISPLAY_COLOR_ENHANCE) ||
        getEffectStatus(DISPLAY_EXPERT)) {
        HandleTrueToneStatus(TRUETONE_NORMAL);
    }
}

void TrueTone::dump(std::string& result)
{
    char res_ch[1024] = {0};
    result.append("\nTT:\n");
    snprintf(res_ch, sizeof(res_ch), "Disp ID: %d, Enable: %d, Mode: %d, OnOffDim: %d, Lux: %f, CCT: %f, Target: %f, Last: %f\n",
             displayId, mEnableTrueTone, mDisplayTruetoneStatus, mTruetoneOnOffdimming,
             mlastlux, mSensorCCT, mTargetCCT, mTargetCCT);
    result.append(std::string(res_ch));
}
} // namespace android
