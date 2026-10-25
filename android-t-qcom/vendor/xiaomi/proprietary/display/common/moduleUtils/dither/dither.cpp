#include "dither.h"
#include "DisplayEffectBase.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "IGCDither"
#endif
using snapdragoncolor::IMiStcService;
namespace android {
int DFLOG::loglevel = 1;
static void DitherSensorCallbackFunction(int event, const Event &info, void *user) {
    Dither *source = (Dither *) user;
    if (source) {
        source->sensorCallback(event, info);
    }
}

Dither::Dither(const sp<DisplayEffectBase>& display_effect)
      : mDisplayEffect(display_effect) {
    DF_LOGD("enter mHistControl = %d", mDisplayEffect->mHistControl);
    if (!mDisplayEffect.get()) {
        return;
    }

    displayId = mDisplayEffect->mDisplayId;
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
    mSensorCtrl.registerCallBack((void*)DitherSensorCallbackFunction, this);

    mParseXml[displayId] = new MiParseXml();
    if (mParseXml[displayId].get()) {
        init();
    }

    if (mDisplayEffect->mHistControl) {
        mThreadHistExit = 0;
        mIGCDitherHistCond = PTHREAD_COND_INITIALIZER;
        mIGCDitherHistlock = PTHREAD_MUTEX_INITIALIZER;
        pthread_attr_t attr_hist;
        pthread_attr_init(&attr_hist);
        pthread_attr_setdetachstate(&attr_hist, PTHREAD_CREATE_JOINABLE);
        pthread_create(&mThreadIGCDitherHist, &attr_hist, ThreadWrapperIGCDitherHist, this);
        pthread_attr_destroy(&attr_hist);
    }
}

void Dither::init()
{
    int ret = 0;
    char p[4096];
    const unsigned int max_count = 1024;
    char *tokens[max_count] = {NULL};
    unsigned int count = 0;

    if (!mParseXml[displayId].get()) {
        return;
    }
    string key("HistControl");
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        mDitherHist.push_back(10);
        mDitherHist.push_back(30);
        return;
    } else {
        for (int i = 0; i < count; i++) {
            mDitherHist.push_back(atoi(tokens[i]));
            DF_LOGV("Dither hist lux config[%d]:%d",i,mDitherHist[i]);
        }
    }
    key = "Hist0Thresh";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        mDither0GrayScaleThresh.push_back(80);
        mDither0GrayScaleThresh.push_back(90);
    } else {
        for (int i = 0; i < count; i++) {
            mDither0GrayScaleThresh.push_back(atoi(tokens[i]));
            DF_LOGV("Dither hist threshold[%d]:%d",i, mDither0GrayScaleThresh[i]);
        }
    }
    key = "DitherStrength";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if (ret >= 0) {
        mDefaultStrength = atoi(p);
        DF_LOGV("dither strength %d", mDefaultStrength);
    } else {
        DF_LOGE("parse %s failed", key.c_str());
        mDefaultStrength = 5;
    }
}

int Dither::getDefaultStrength()
{
    return mDefaultStrength;
}

void Dither::sensorCallback(int event, const Event &info)
{
    if (mDisplayState != kStateOn) {
        return;
    }
    if ((event == SENSOR_TYPE_LIGHT_PRIMARY_SCREEN && displayId != DISPLAY_PRIMARY) ||
        (event == SENSOR_TYPE_LIGHT_SECOND_SCREEN && displayId != DISPLAY_SECONDARY) ||
        (event != SENSOR_TYPE_LIGHT_PRIMARY_SCREEN && event != SENSOR_TYPE_LIGHT_SECOND_SCREEN)) {
        return;
    }
    DF_LOGV("mDisplayId = %d dataCallback value=<%9.4f,%9.4f,%9.4f>,"
       "time=%ld, sensor=%d, temperature(K)=%9.4f", mDisplayEffect->mDisplayId,
       info.u.data[0], info.u.data[1], info.u.data[2],
       info.timestamp, info.sensorType, info.u.data[1]);
    if (mIGCDitherEnable) {
        if ((int)info.u.data[0] <= mDitherHist[0]) {
            if (mHistStatus == HIST_STOPPED) {
                histogram_operation(HIST_CMD_START);
                pthread_cond_signal(&mIGCDitherHistCond);
            }
        } else if ((int)info.u.data[0] > mDitherHist[1]) {
            if (mHistStatus == HIST_STARTED)
                histogram_operation(HIST_CMD_STOP);
            if (!current_dither_en) {
                struct OutputParam sParams;
                sParams.function_id = COLOR_IGC_DITHER;
                sParams.param1 = 1;
                sParams.param2 = mIGCDitherStrength;
                mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sParams);
                current_dither_en = 1;
            }
        } else {
            if (mHistStatus != HIST_STARTED) {
                if (current_dither_en != 1) {
                    struct OutputParam sParams;
                    sParams.function_id = COLOR_IGC_DITHER;
                    sParams.param1 = 1;
                    sParams.param2 = mIGCDitherStrength;
                    mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sParams);
                    current_dither_en = 1;
                }
            }
        }
        mLastLux = (int)info.u.data[0];
    } 
}

int Dither::histogram_operation(int cmd)
{
    int value = 0;
    struct OutputParam sParams;

    sParams.function_id = DISPLAY_FEATURE_HIST;
    sParams.param2 = IGC_DITHER;
    Mutex::Autolock autoLock(mHistLock);

    switch ((enum HANDLE_HIST_CMD)cmd) {
        case HIST_CMD_START: {
            if (mHistStatus != HIST_STOPPED) {
                DF_LOGD("IGC dither histogram_operation-Start, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_START;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mHistStatus = HIST_STARTED;
        } break;
        case HIST_CMD_GET: {
            if (mHistStatus != HIST_STARTED) {
                DF_LOGD("IGC dither histogram_operation-Get, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_GET;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mZeroGayScaleRatio = sParams.len;
        } break;
        case HIST_CMD_STOP: {
            if (mHistStatus != HIST_STARTED) {
                DF_LOGD("IGC dither histogram_operation-Stop, histgram status is wrong, status:%d!", mHistStatus);
                break;
            }

            sParams.param1 = HIST_CMD_STOP;
            mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
            mHistStatus = HIST_STOPPED;
        } break;
        default:
            DF_LOGD("IGC dither histogram_operation, cmd don't support!");
    }

    return mHistStatus;
}

void *Dither::ThreadWrapperIGCDitherHist(void *me) {
    return (void *)(uintptr_t)static_cast<Dither *>(me)->threadFuncIGCDitherHist();
}

int Dither::threadFuncIGCDitherHist() {
    struct OutputParam sParams;
    int ret = 0;
    sParams.function_id = COLOR_IGC_DITHER;

    while (mThreadHistExit == 0) {
        pthread_mutex_lock(&mIGCDitherHistlock);
        pthread_cond_wait(&mIGCDitherHistCond, &mIGCDitherHistlock);
        pthread_mutex_unlock(&mIGCDitherHistlock);

        usleep(500000);

        /*get histogram value*/
        while (histogram_operation(HIST_CMD_GET)) {
            if (mIGCDitherEnable) {
                if (mZeroGayScaleRatio >= mDither0GrayScaleThresh[1])
                    sParams.param1 = 0;
                else if (mZeroGayScaleRatio <= mDither0GrayScaleThresh[0])
                    sParams.param1 = 1;
                else
                    sParams.param1 = current_dither_en;
            } else {
                sParams.param1 = 0;
            }
            sParams.param2 = mIGCDitherStrength;
            if (current_dither_en != sParams.param1) {
                ret = mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sParams);
                current_dither_en = sParams.param1;
            }

            /* read histogram value per 500ms*/
            usleep(500000);
        }
    }

    return 0;
}

void Dither::setIGCDither(int enable, int strength)
{
    mIGCDitherEnable = enable;
    struct OutputParam sParams;
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();

    DF_LOGV("enable %d, strength %d, lux %d, hist status %d", enable, strength, mLastLux, mHistStatus);
    if (enable) {
        mIGCDitherStrength = strength;
        if (mLastLux <= 10) {
            histogram_operation(HIST_CMD_START);
            pthread_cond_signal(&mIGCDitherHistCond);
        }
        mSensorCtrl.sensorControlOP(displayId, SENSOR_START, SENSOR_LIGHT, DITHER);
    } else {
        mIGCDitherStrength = 0;
        sParams.function_id = COLOR_IGC_DITHER;
        sParams.param1 = 0;
        sParams.param2 = mIGCDitherStrength;
        mDisplayEffect->setDisplayEffect(SET_DSPP_MODE, &sParams);
        current_dither_en = sParams.param1;
        if (mHistStatus == HIST_STARTED)
            histogram_operation(HIST_CMD_STOP);
        mSensorCtrl.sensorControlOP(displayId, SENSOR_STOP, SENSOR_LIGHT, DITHER);
    }
}

void Dither::setDisplayState(int display_state)
{
    mDisplayState = display_state;
    SensorCtrlIntf & mSensorCtrl = SensorCtrlIntf::getInstance();
    if (mDisplayState == kStateOff) {
        if (mIGCDitherEnable == 1) {
            DF_LOGV("Display isn't ON, close IGC dither hist");
            histogram_operation(HIST_CMD_STOP);
        }
        mSensorCtrl.sensorControlOP(displayId, SENSOR_STOP, SENSOR_LIGHT, DITHER);
    }
}
} //namespace android
