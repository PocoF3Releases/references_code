#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "cust_define.h"
#include "DisplayPostprocessing.h"
#include "display_property.h"
#include "DisplayColorManager_hidl.h"
#include "display_effects.h"
#ifdef HAS_QDCOM
#include <QServiceUtils.h>

using snapdragoncolor::IMiStcService;
#endif

#define LOG_TAG "DisplayFeatureHal"
#define LOG_NDEBUG 0

#define R_COEFF_INDEX        2
#define G_COEFF_INDEX       14
#define B_COEFF_INDEX       26

namespace android {

const float CALIB_COEFF_THREASHOLD = 0.92;

template<typename T>
int gamma_correct(T &data, int size, T gamma)
{
    int ret = 0;
    T temp = 0.0;
    for (int i = 0; i < size; i++) {
        data[i] =  powf(data[i], 1.0/gamma);
        if (data[i] > temp)
            temp = data[i];
    }
    for (int i = 0; i < size; i++) {
        if (temp > 0)
            data[i] = data[i]/temp;
        else {
            DF_LOGE(" the max value is wrong:%f",  data[i]);
            ret = -1;
            break;
        }
    }
    return 0;
}

template<typename T>
int inverse_gamma(T &data, int size, T gamma)
{
    int ret = 0;
    T temp = 0.0;

    for (int i = 0; i < size; i++) {
        if (data[i] > temp)
            temp = data[i];
    }

    for (int i = 0; i < size; i++) {
        if (temp > 0) {
           data[i] = data[i]/temp;
           data[i] = powf(data[i], 2.2);
        } else {
           DF_LOGE(" the max value is wrong:%f", data[i]);
           ret = -1;
           break;
        }
    }
    return ret;
}

DisplayPostprocessing::DisplayPostprocessing(int displayId)
    :mWPSCCoeffComputed(false)
    , mDisplayId(displayId)
{
    DF_LOGD("Enter");

    int temp_arr_interval[HIST_LEVEL_MAX] = {25, 51, 76, 102, 153, 178, 204, 229, 255};

    memset(&mPccParams, 0, sizeof(mPccParams));
    memcpy((void *)mHistLevelInterval, (void *)temp_arr_interval, HIST_LEVEL_MAX*sizeof(int));

    mHistParse.changed = 0;
    mHistParse.hist_weighted = HIST_LEVEL_4;
    mHistParse.weight_value = 0;
    mHistParse.current_time = 0;
    mDSPP = new DSPP_Platform(displayId);
#ifdef MTK_PLATFORM
    mEyeCare[displayId] = new EyeCare(displayId);
#endif
}

DisplayPostprocessing::~DisplayPostprocessing()
{
    DF_LOGV("Enter");
#ifdef HAS_QDCM
    HWColorDrmMgr::getInstance().Deinit();
#endif
}

int DisplayPostprocessing::setPostprocessingMode(struct DispPPParam *pSDPPParams)
{
    int ret = 0;

    DF_LOGV("Enter");
    if (!pSDPPParams) {
        DF_LOGW("invalid parameter");
        return ret;
    }

    switch (( enum DISPLAY_FEATURE_TYPE)pSDPPParams->function_id) {
        case DISPLAY_FEATURE_CCBB: {
            ret = setPCCConfigCCBB(pSDPPParams->param1, pSDPPParams->param2);
        } break;
        case DISPLAY_FEATURE_REFRESH: {
            ret = forceScreenRefresh();
        } break;
        case DISPLAY_CAL_WP: {
            ret = calibrateWhitePoint();
        } break;
        case DISPLAY_CHANGE_WP: {
            ret = changeWhitePoint(pSDPPParams->param1);
        } break;
        case DISPLAY_CT_ADJUST: {
            ret = setPCCConfigCT(pSDPPParams->param1, pSDPPParams->param2);
        } break;
        case DISPLAY_FEATURE_HIST: {
            Mutex::Autolock autoLock(mHistGetLock);
            if(SRE_FEATURE == pSDPPParams->param2)
                ret = computeSREHistogram(pSDPPParams->param1, &pSDPPParams->len);/* len is used to store hist info */
            else if (GAME_SRE_FEATURE == pSDPPParams->param2)
                ret = computeGAMESREHistogram(pSDPPParams->param1, &pSDPPParams->len);/* len is used to store hist info */
            else if (BCBC_FEATURE == pSDPPParams->param2)
                ret = computeHistogram(pSDPPParams->param1);
            else if (VSDR2HDR_FEATURE == pSDPPParams->param2)
                ret = computeVsdr2hdrHistogram(pSDPPParams->param1, &pSDPPParams->len);/* len is used to store hist info */
            else if (IGC_DITHER == pSDPPParams->param2)
                ret = computeIGCDitherHistogram(pSDPPParams->param1, &pSDPPParams->len);/* len is used to store hist info */
            else
                ret = computeHistogram(pSDPPParams->param1);
        } break;
        case DISPLAY_EYECARE_MODE: {
            ret = setPCCConfigEyeCare(pSDPPParams->param1);
        } break;
        case DISPLAY_BREAK_CT_DIMMING : {
            setBreakCTDimming(pSDPPParams->param1);
        } break;
        case DISPLAY_CLEAR_PCC: {
            ret = clearPccConfig();
        } break;
        case DISPLAY_BREAK_SLPCC_DIMMING : {
            setPCCConfigSunlightScreenDimming(pSDPPParams->param1);
        } break;
        case DISPLAY_SUNLIGHT_SCREEN: {
           ret = setPCCConfigSunlightScreen(pSDPPParams->param1, pSDPPParams->param2);
        } break;
        case DISPLAY_RESTORE_PCC: {
            ret = restorePccConfig();
        } break;
        case DISPLAY_GABB: {
            ret = setPCCConfigGABB(pSDPPParams->param1);
        } break;
        case DISPLAY_HDR_PCC: {
            ret = setHDRPcc(pSDPPParams->param1);
        } break;
        case DISPLAY_EXPERT_MODE: {
            ret = setPCCConfigExpert(pSDPPParams->param1);
        } break;
        case DISPLAY_GRAY_MODE: {
            ret = setPCCConfigGrayMode(pSDPPParams->param1);
        } break;
        case DISPLAY_BL: {
            ret = setPCCConfigBL(pSDPPParams);
        } break;
        case DISPLAY_DC: {
            ret = setPCCConfigDC(pSDPPParams);
        } break;
        case DISPLAY_PAPER_MODE: {
            ret = setPCCConfigPaperMode(pSDPPParams);
        } break;
        case DISPLAY_TRUE_TONE: {
            ret = setPCCConfigTrueTone(pSDPPParams);
        } break;
        case DISPLAY_CABC: {
            ret = setESSFunction(pSDPPParams->param1);
        } break;
        case DISPLAY_PQ_MODE: {
            ret = setPQMode(pSDPPParams->param1);
        } break;
        case DISPLAY_GAME_PQ_MODE: {
            ret = setGamePQMode(pSDPPParams->param1);
        } break;
        case DISPLAY_DRE: {
            ret = setDREFunction(pSDPPParams->param1);
        } break;
        default:
            DF_LOGE("error mode ...");
    }

    return ret;
}

void DisplayPostprocessing::setParams(void* params)
{
    if (params != NULL)
        memcpy(&mPccParams, params, sizeof(mPccParams));
}

void DisplayPostprocessing::setBreakCTDimming(int value)
{
    break_ct = value;
}

int DisplayPostprocessing::setPCCConfigTrueTone(struct DispPPParam *pSDPPParams)
{
    int ret = 0;
    for (int i = 0; i < ARRAY_SIZE_EXP; i++) {
        mTrueToneCoeff[i] = pSDPPParams->pcc_payload[i];
    }
    ret = setPCCConfig();
    return 0;
}

int DisplayPostprocessing::setPCCConfigGrayMode(int enable)
{
    int ret = 0;
    double nativeCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
    double grayCoeff[ARRAY_SIZE_EXP] = {0.299, 0.587, 0.114, 0.299, 0.587, 0.114, 0.299, 0.587, 0.114};

    if (enable) {
        memcpy(mGrayModeCoeff, grayCoeff, sizeof(mGrayModeCoeff));
    } else {
        memcpy(mGrayModeCoeff, nativeCoeff, sizeof(mGrayModeCoeff));
    }
    ret = setPCCConfig();
    return ret;
}

int DisplayPostprocessing::setPCCConfigBL(struct DispPPParam *pSDPPParams)
{
    int i, j, ret = 0;
    static int last_brightness = -1;
    int real_brightness = 0;
    char BLValue[10];

    for (i = 0; i < ARRAY_SIZE; i++) {
        mBrightnessCoeff[i] = pSDPPParams->pcc_payload[0];
    }
    m12BitBl = pSDPPParams->param1;
    ret = setPCCConfig();
    DF_LOGV("pcc coeff %f", mBrightnessCoeff[0]);

    return ret;
}

int DisplayPostprocessing::setPCCConfigDC(struct DispPPParam *pSDPPParams)
{
    int i, ret = 0;
    for (i = 0; i < ARRAY_SIZE; i++)
        mDCBrightnessCoeff[i] = pSDPPParams->pcc_payload[0];

    ret = setPCCConfig();

    return ret;
}

int DisplayPostprocessing::setPCCConfigPaperMode(struct DispPPParam *pSDPPParams)
{
    int ret = 0;
    for (int i = 0; i < ARRAY_SIZE_EXP; i++) {
        mPaperModeCoeff[i] = pSDPPParams->pcc_payload[i];
    }
    ret = setPCCConfig();
    return 0;
}

int DisplayPostprocessing::setPCCConfigCCBB(int value, bool enable)
{
    DF_LOGV("enable %d ccbb_enable %d", enable, ccbb_enable);

    int ret = -1;
    int i ;
    float rgbCoeff[ARRAY_SIZE] = {1.0, 1.0, 1.0};

    if(!ccbb_enable)
        return -1;

    if (value <= 0 && enable) {
        DF_LOGE("%s: input parameters is invalide!", __FUNCTION__);
        return -1;
    }

    ret = computeRGBCoeffbyBL(rgbCoeff,(uint32_t) value);
    if(ret) {
        DF_LOGE("comput rgb coeff fail!\n");
        return -1;
    }
    for (i = 0; i < ARRAY_SIZE; i++)
        mCCBBCoeff[i] = rgbCoeff[i];

    DF_LOGV("rgbCoeff: R %f, G %f, B %f", mCCBBCoeff[0], mCCBBCoeff[1], mCCBBCoeff[2]);

    ret = setPCCConfig();
    return ret;
}

int DisplayPostprocessing::setPCCConfigEyeCare(int value)
{
    int ret = 0;
#ifdef MTK_PLATFORM
    int i = 0;
    if (mEyeCare[mDisplayId].get()) {
        mEyeCare[mDisplayId]->getEyeCareCoeff(value, mEyeCareCoeffExp);
    }

    for (i = 0; i < ARRAY_SIZE_EXP; i++) {
        DF_LOGV("eyecare coeff%d %f",i, mEyeCareCoeffExp[i]);
    }
    ret = setPCCConfig();
#endif
    return ret;
}

void DisplayPostprocessing::checkParameterValid(int *param, int min, int max)
{
    if (*param >= min && *param <= max) {
        //valid parameter
    } else if (*param < min) {
        *param = min;
    } else {
        *param = max;
    }
    return;
}

int DisplayPostprocessing::setPCCConfigCT(int value, int enable_dimming)
{
    DF_LOGV("enable set CT 0x%x", value);

    int RGB[ARRAY_SIZE];
    float fRGB[ARRAY_SIZE];
    int ret = 0;
    int fd = 0;
    int i,j,alpha;
    static int fd_backup = -1;
    double temp = 0.0;
    int strength = 0;
    int step = 1;
    float length[ARRAY_SIZE];

    alpha = (value & 0xFF000000) >> 24;
    RGB[0] = (value & 0xFF0000) >> 16;
    RGB[1] = (value & 0xFF00) >> 8;
    RGB[2] = value & 0xFF;
    strength = mPccParams.uctParam.paramMax - mPccParams.uctParam.strength;
    for(i = 0; i < ARRAY_SIZE; i ++) {
        checkParameterValid(&RGB[i], mPccParams.uctParam.uiParamMin, mPccParams.uctParam.paramMax);
    }
    if (value == 1) { //warm
        memcpy(RGB, mPccParams.uctParam.warm, sizeof(RGB));
    } else if (value == 2) { //default
        memcpy(RGB, mPccParams.uctParam.def, sizeof(RGB));
    } else if (value == 3) { //cold
        memcpy(RGB, mPccParams.uctParam.cold, sizeof(RGB));
    } else {
        /*UI R G B value range is 100~255, native range should be 220~255,add mapping
          (R - 100)/(255 - 100) = (native_R - strength)/(255 - strength)
          so native_R = strength + (255 - strength) * (R - 100) / 155
        */
        for(i = 0; i < ARRAY_SIZE; i++) {
            RGB[i] = strength + (mPccParams.uctParam.paramMax - strength)
                * (RGB[i] - mPccParams.uctParam.uiParamMin) / (mPccParams.uctParam.paramMax - mPccParams.uctParam.uiParamMin);
            checkParameterValid(&RGB[i], strength, mPccParams.uctParam.paramMax);
        }
    }

    if (enable_dimming) {
        step = RGB[1] < RGB[2] ? RGB[1] : RGB[2];
        step = RGB[0] < step ? RGB[0] : step;
        step = mPccParams.uctParam.paramMax - step;
        if (step >= mPccParams.uctParam.dimmingStep)
            step = mPccParams.uctParam.dimmingStep;

        for (i = 0; i < ARRAY_SIZE; i++) {
            length[i] = (255.0 - RGB[i])/(float)step;
            fRGB[i] = 255.0;
        }

        for (i = 0; i < step; i++) {
            temp = 0.0;
            for(j = 0; j < ARRAY_SIZE; j++) {
                fRGB[j] -= length[j];
                mCTCoeff[j] = fRGB[j]/255.0;
                if (mCTCoeff[j] > temp)
                    temp = mCTCoeff[j];
            }
            for(j = 0; j < ARRAY_SIZE; j++) {
                if (temp > 0)
                mCTCoeff[j] = mCTCoeff[j]/temp;
            }
            usleep(mPccParams.uctParam.dimmingTime * 1000);
            DF_LOGV("rgbCoeff: R %f, G %f, B %f", mCTCoeff[0], mCTCoeff[1], mCTCoeff[2]);
            ret = setPCCConfig();
            if (break_ct) {
                break_ct = 0;
                break;
            }
        }
    }else {
        for(i = 0; i < ARRAY_SIZE; i++) {
            mCTCoeff[i] = RGB[i]/255.0;
            if (mCTCoeff[i] > temp)
                temp = mCTCoeff[i];
        }
        for(i = 0; i < ARRAY_SIZE; i++) {
            if (temp > 0)
            mCTCoeff[i] = mCTCoeff[i]/temp;
        }
        DF_LOGV("rgbCoeff: R %f, G %f, B %f", mCTCoeff[0], mCTCoeff[1], mCTCoeff[2]);
        ret = setPCCConfig();
    }

    return ret;
}

int DisplayPostprocessing::setPCCConfig()
{
    int i,j;
    int ret = 0;
    double temp = 0.0;

    RgbTransform* rgbTrans = new RgbTransform();
    rgbTrans->applyTransform(mCCBBCoeff)
            .applyTransform(mCTCoeff);
    rgbTrans->normalizeTransform()
            .applyTransform(mBrightnessCoeff)
            .applyTransform(mExpertCoeff);

    ModeTransform * modeTans = new ModeTransform();
    modeTans->applyTransform(mPaperModeCoeff)
            .applyTransform(mSLScreenCoeff)
            .applyTransform(mGABBCoeff)
            .applyTransform(mEyeCareCoeffExp)
            .applyTransform(mGrayModeCoeff)
            .applyTransform(mTrueToneCoeff);

    for (i = 0; i < ARRAY_SIZE_EXP; i++)
        pccCoeff[i] = (*rgbTrans)[i/3] * (*modeTans)[i] * mDCBrightnessCoeff[i/3];

        DF_LOGV("rgbCoeff: R[r %f, g %f, b %f], G[r %f, g %f, b %f], B[r %f, g %f, b %f]",
            pccCoeff[0],pccCoeff[1],pccCoeff[2],pccCoeff[3],pccCoeff[4],pccCoeff[5],pccCoeff[6],pccCoeff[7],pccCoeff[8]);

    if (mDSPP.get()) {
        mDSPP->setPccConfig(pccCoeff, m12BitBl);
        if (m12BitBl != -1)
            m12BitBl = -1;
    }

    if(ret) {
        DF_LOGE("set PCC fail!\n");
        return ret;
    }

    DisplayUtil::onCallback(20000, 0
        , pow((pccCoeff[0]+pccCoeff[1]+pccCoeff[2])/mBrightnessCoeff[0], 1.0f/2.2f)
        , pow((pccCoeff[3]+pccCoeff[4]+pccCoeff[5])/mBrightnessCoeff[1], 1.0f/2.2f)
        , pow((pccCoeff[6]+pccCoeff[7]+pccCoeff[8])/mBrightnessCoeff[2], 1.0f/2.2f));

    delete rgbTrans;
    delete modeTans;
    return ret;
}

int DisplayPostprocessing::clearPccConfig()
{
    int ret = 0;
    int enable = 1;
    double rgbCoeffExp[ARRAY_SIZE_EXP] = {1.0, 0.0, 0.0,
                                          0.0, 1.0, 0.0,
                                          0.0, 0.0, 1.0};

    if (mDSPP.get()) {
        mDSPP->setPccConfig(rgbCoeffExp, -1);
    }
    return ret;
}

int DisplayPostprocessing::restorePccConfig()
{

    int ret = 0;

    ret = setPCCConfig();

    return ret;
}

void DisplayPostprocessing::setPCCConfigSunlightScreenDimming(int value)
{
    break_SLPCC = value;
}

int DisplayPostprocessing::setPCCConfigSunlightScreen(int value, int cookie)
{
    int levelDiff = 0;
    int i;
    int ret = 0;
    int temp = 0;

    DF_LOGD("set level %d, last level %d, cookie %d", value, mLastSLLevel, cookie);

    /* 0xFF == cookie or 0xEE == cookie means no need transit */
    if (0xFF == cookie ||0xEE == cookie ) {
        mLastSLCookie = cookie;
        mLastSLLevel = value;
        memcpy(mSLScreenCoeff, mPccParams.slsParams.coeff[value], sizeof(mSLScreenCoeff));
        ret = setPCCConfig();
        return ret;
    }

    /* 0xEE == lastCookie means fixed pcc level */
    if (0xEE == mLastSLCookie) {
        return ret;
    }

    levelDiff = abs(value - mLastSLLevel);
    for (i = 0; i < levelDiff; i++) {
        if (0xFF == mLastSLCookie && i != 0) {
            mLastSLCookie = cookie;
            return ret;
        } else
            mLastSLCookie = cookie;
        if (value > mLastSLLevel) {
            temp = mLastSLLevel + i + 1;
        } else {
            temp = mLastSLLevel - (i + 1);
        }
        if (temp >= 0)
            temp = temp > (SUNLIGHT_SCREEN_LEVEL_MAX-1) ? (SUNLIGHT_SCREEN_LEVEL_MAX-1) : temp;
        else
            temp = 0;
        memcpy(mSLScreenCoeff, mPccParams.slsParams.coeff[temp], sizeof(mSLScreenCoeff));
        ret = setPCCConfig();
        if (break_SLPCC) {
            break_SLPCC = 0;
            if (i > 0) {
                if (value > mLastSLLevel)
                    mLastSLLevel = mLastSLLevel + i + 1;
                else
                    mLastSLLevel = mLastSLLevel - (i + 1);
                DF_LOGD("need break, mLastSLLevel = %d", mLastSLLevel);
                return ret;
            }
        }
        usleep(SUNLIGHT_ADJUST_DELAY);
    }

    mLastSLLevel = value;

    return ret;
}

int DisplayPostprocessing::setPCCConfigGABB(int brightness)
{
    int ret = 0;

    for (int i = 0; i < 9; i++) {
        if (brightness > 0) {
            mGABBCoeff[i] = 0.0;
            for (int j = 0; j < 9; j++) {
                mGABBCoeff[i] += mPccParams.GABBParams.GABBCoeff[i][j] * pow(brightness, 8 - j);
            }
        } else {
            if (i%4 == 0)
                mGABBCoeff[i] = 1.0;
            else
                mGABBCoeff[i] = 0.0;
        }
        DF_LOGV("pcc gabb coeff %d %f", i, mGABBCoeff[i]);
    }
    ret = setPCCConfig();

    return ret;
}

int DisplayPostprocessing::setPCCConfigExpert(int rgb)
{
    int r, g, b, ret = 0;
    r = (rgb >> 16) & 0xFF;
    g = (rgb >> 8) & 0xFF;
    b = rgb & 0xFF;

    mExpertCoeff[ARRAY_INDEX_0] = r / 255.0;
    mExpertCoeff[ARRAY_INDEX_1] = g / 255.0;
    mExpertCoeff[ARRAY_INDEX_2] = b / 255.0;

    ret = setPCCConfig();
    DF_LOGV("r %f, g %f, b %f, ret %d",
        mExpertCoeff[ARRAY_INDEX_0], mExpertCoeff[ARRAY_INDEX_1], mExpertCoeff[ARRAY_INDEX_2], ret);
    return ret;
}

int DisplayPostprocessing::setHDRPcc(int value)
{
    int ret = 0;
    mHDREnable = value;

    ret = setPCCConfig();

    return ret;
}

int DisplayPostprocessing::forceScreenRefresh()
{
    DF_LOGV("Enter");
    int ret = 0;
#ifdef HAS_QDCM
    sp<qService::IQService> iqservice;

    iqservice = interface_cast<qService::IQService>(defaultServiceManager()->getService(String16("display.qservice")));
    if (iqservice.get()) {
        iqservice->dispatch(qService::IQService::SCREEN_REFRESH, NULL, NULL);
    } else {
        DF_LOGW("Failed to acquire %s", "display.qservice");
        ret = -1;
    }
#endif
    return ret;
}

int DisplayPostprocessing::calibrateWhitePoint(float x, float y)
{
    int ret = 0;

    if (mWPSCCoeffComputed) {
        if(x>0.0 && y>0.0)
            DF_LOGI("x=%f y=%f", x, y);
        else
            return ret;
    }

    int i, j = 0;
    int x_coordinate = 0, y_coordinate = 0;
    float temp = 0.0;
    float target_wp[2]; //target x y
    float xy_compensation[2];
    float x_range[2];
    float y_range[2];
    float wp_coordinate_XY[ARRAY_SIZE] = {1.0, 1.0, 1.0};
    /*target white pointer(0.305,0.32,1)*/
    float target_wp_coordinate_XY[ARRAY_SIZE];// = {0.953125, 1.0, 1.171875};
    /*rgb_coordinate_xyl=>{{Rx,Gx,Bx},{Ry,Gy,By},{RLv,GLv,BLv}}*/
    float rgb_coordinate_xyl[ARRAY_SIZE][ARRAY_SIZE];// = {{0.674, 0.268, 0.15}, {0.322, 0.69, 0.06}, {0, 0, 0}};
    /*rgb_coordinate_XYZ=>{{Xr,Xg,Xb},{Yr,Yg,Yb},{Zr,Zg,Zb}}*/
    float rgb_coordinate_XYZ[ARRAY_SIZE][2*ARRAY_SIZE];
    static float wp_offset[2];

    for (i = 0; i < 2; i++) {
        target_wp[i] = mPccParams.wpCalibParam.targetWP[i];
        xy_compensation[i] = mPccParams.wpCalibParam.xyComp[i];
        x_range[i] = mPccParams.wpCalibParam.xRange[i];
        y_range[i] = mPccParams.wpCalibParam.yRange[i];
        wp_offset[i] = mPccParams.wpCalibParam.xyOffset[i];
    }
    for (i = 0; i < ARRAY_SIZE; i++) {
        target_wp_coordinate_XY[i] = mPccParams.wpCalibParam.targetXY[i];
        for (j = 0; j < 3; j++)
            rgb_coordinate_xyl[i][j] = mPccParams.wpCalibParam.rgbCoodinatexyl[i][j];
    }

    /*
      1.read lcm white point x,y,Lv=1 => mWPXYCoord
      2.calculate the matrix for Yrgb convert XYZ => rgb_coordinate_XYZ{3x3}
        formula=>X=x*Y/y,Y=1,Z=(1-x-y)*Y/y
        [Xr,Xg,Xb][a]   [WX]
        [Yr,Yg,Yb][b] = [WY]
        [Zr,Zg,Zb][c]   [WZ]
      3.calculate WP's XYZ => wp_coordinate_XY{3x1}
      4.compute XYZ of Yrgb  matrix 's inverse matrix =>input A|E output E|A^
      5.calculate {a,b,c} by matrix multiplication {3x3}*{3x1}
      6.calculate RGB2XYZ matrix => RGB's xyLv convert RGB's XYZ refer step2
      7.caluelate RGB2XYZ's inverse matrix=>XYZ2RGB refer step 4
    */
    if (x > 0.0 && y > 0.0) {
        mWPXYCoord[ARRAY_INDEX_0] = x;
        mWPXYCoord[ARRAY_INDEX_1] = y;
        DF_LOGW("readPanelWhiteXYCoordinate tuning (%f %f)",mWPXYCoord[ARRAY_INDEX_0],mWPXYCoord[ARRAY_INDEX_1]);
    } else {
        ret = DisplayUtil::readPanelWhiteXYCoordinate(x_coordinate, y_coordinate);
        if (ret < 0 || x_coordinate<=0 || y_coordinate<=0) {
            DF_LOGE("readPanelWhiteXYCoordinate fail!");
            mWPXYCoord[ARRAY_INDEX_0] = target_wp[0];
            mWPXYCoord[ARRAY_INDEX_1] = target_wp[1];
        } else {
            mWPXYCoord[ARRAY_INDEX_0] = (float)x_coordinate/1000.0 + xy_compensation[0] + wp_offset[0];
            mWPXYCoord[ARRAY_INDEX_1] = (float)y_coordinate/1000.0 + xy_compensation[1] + wp_offset[1];
            if((mWPXYCoord[ARRAY_INDEX_0] < x_range[0]|| mWPXYCoord[ARRAY_INDEX_0] > x_range[1]) &&
                (mWPXYCoord[ARRAY_INDEX_1] < y_range[0]|| mWPXYCoord[ARRAY_INDEX_1] > y_range[1])) {
                mWPXYCoord[ARRAY_INDEX_0] = target_wp[0];
                mWPXYCoord[ARRAY_INDEX_1] = target_wp[1];
            }
        }
    }
    DF_LOGV("wp x %f y %f", mWPXYCoord[ARRAY_INDEX_0], mWPXYCoord[ARRAY_INDEX_1]);
    memset((void *)rgb_coordinate_XYZ, 0, sizeof(rgb_coordinate_XYZ));

    /* rgb x y z map to X Y Z, and add unit matrix to XYZ matrix for computing inverse matrix  */
    for (i = 0; i < ARRAY_SIZE; i++) {
        rgb_coordinate_XYZ[ARRAY_INDEX_0][i] = rgb_coordinate_xyl[ARRAY_INDEX_0][i]/rgb_coordinate_xyl[ARRAY_INDEX_1][i];
        rgb_coordinate_XYZ[ARRAY_INDEX_1][i] = 1;
        rgb_coordinate_XYZ[ARRAY_INDEX_2][i] = (1-rgb_coordinate_xyl[ARRAY_INDEX_0][i]-rgb_coordinate_xyl[ARRAY_INDEX_1][i])/rgb_coordinate_xyl[ARRAY_INDEX_1][i];
        rgb_coordinate_XYZ[i][ARRAY_SIZE+i] = 1;
    }

    /* white point x y map to X Y*/
    wp_coordinate_XY[ARRAY_INDEX_0] = (mWPXYCoord[ARRAY_INDEX_0]/mWPXYCoord[ARRAY_INDEX_1])*mWPXYCoord[ARRAY_INDEX_2];
    wp_coordinate_XY[ARRAY_INDEX_1] = 1;
    wp_coordinate_XY[ARRAY_INDEX_2] = mWPXYCoord[ARRAY_INDEX_2]*(1-mWPXYCoord[ARRAY_INDEX_0]-mWPXYCoord[ARRAY_INDEX_1])/mWPXYCoord[ARRAY_INDEX_1];

    DF_LOGV("WP XY:%f  %f  %f",  wp_coordinate_XY[ARRAY_INDEX_0],  wp_coordinate_XY[ARRAY_INDEX_1],  wp_coordinate_XY[ARRAY_INDEX_2]);
    /* compute XYZ's inverse matrix */
    ret = DisplayUtil::matrix_inverse(rgb_coordinate_XYZ);
    if (ret < 0) {
        DF_LOGE("step 1, compute inverse matrix fail!!");
        return ret;
    }

    for (i = 0; i < ARRAY_SIZE; i++) {
        DF_LOGV("inverse XYZ:%f  %f  %f  %f  %f  %f",  rgb_coordinate_XYZ[i][0],  rgb_coordinate_XYZ[i][1],  rgb_coordinate_XYZ[i][2],
             rgb_coordinate_XYZ[i][3],  rgb_coordinate_XYZ[i][4],  rgb_coordinate_XYZ[i][5]);
    }

    for (i = 0; i < ARRAY_SIZE; i++) {
        for (j = 0; j < ARRAY_SIZE; j++) {
            rgb_coordinate_xyl[ARRAY_SIZE-1][i] += rgb_coordinate_XYZ[i][ARRAY_SIZE + j]*wp_coordinate_XY[j];
        }
    }

    for (i = 0; i < ARRAY_SIZE; i++) {
        DF_LOGV("xyLv:%f  %f  %f",  rgb_coordinate_xyl[i][0],  rgb_coordinate_xyl[i][1],  rgb_coordinate_xyl[i][2]);
    }

    /* x y z map to X Y Z, and add unit matrix to XYZ matrix for computing inverse matrix  */
    memset((void *)rgb_coordinate_XYZ, 0, sizeof(rgb_coordinate_XYZ));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        rgb_coordinate_XYZ[ARRAY_INDEX_0][i] = (rgb_coordinate_xyl[ARRAY_INDEX_0][i]/rgb_coordinate_xyl[ARRAY_INDEX_1][i])*rgb_coordinate_xyl[ARRAY_INDEX_2][i];
        rgb_coordinate_XYZ[ARRAY_INDEX_1][i] = rgb_coordinate_xyl[ARRAY_SIZE-1][i];
        rgb_coordinate_XYZ[ARRAY_INDEX_2][i] = rgb_coordinate_xyl[ARRAY_INDEX_2][i]*(1-rgb_coordinate_xyl[ARRAY_INDEX_0][i]-rgb_coordinate_xyl[1][i])/rgb_coordinate_xyl[ARRAY_INDEX_1][i];

        rgb_coordinate_XYZ[i][ARRAY_SIZE+i] = 1;
    }
    for (i = 0; i < ARRAY_SIZE; i++) {
        DF_LOGV("XYZ:%f  %f  %f  %f  %f  %f",  rgb_coordinate_XYZ[i][0],  rgb_coordinate_XYZ[i][1],  rgb_coordinate_XYZ[i][2],
             rgb_coordinate_XYZ[i][3],  rgb_coordinate_XYZ[i][4],  rgb_coordinate_XYZ[i][5]);
    }
    /* compute XYZ's inverse matrix */
    ret = DisplayUtil::matrix_inverse(rgb_coordinate_XYZ);
    if (ret < 0) {
        DF_LOGE("step 2, compute inverse matrix fail!!");
        return ret;
    }

    for (i = 0; i < ARRAY_SIZE; i++) {
        for (j = 0; j < ARRAY_SIZE; j++) {
            mP3XYZ2RGB[i][j] = rgb_coordinate_XYZ[i][j+ARRAY_SIZE];
        }
        DF_LOGV("inverse XYZ:%f  %f  %f  %f  %f  %f",  rgb_coordinate_XYZ[i][0],  rgb_coordinate_XYZ[i][1],  rgb_coordinate_XYZ[i][2],
               rgb_coordinate_XYZ[i][3],  rgb_coordinate_XYZ[i][4],  rgb_coordinate_XYZ[i][5]);
    }
    /*
     1.calculate R'G'B' matrix by multiplication
     2.calculate RGB value(0~255)
       formula : R=R'^(1/2.2)*255....
     3.rgb coef normalization
       select max R,G,B value and normalization
    */
    /* compute calibrate coeff */
    for (i = 0; i < ARRAY_SIZE; i++) {
        mCalibCoeff[i] = 0.0;
        for (j = 0; j < ARRAY_SIZE; j++) {
            mCalibCoeff[i] += rgb_coordinate_XYZ[i][ARRAY_SIZE + j]*target_wp_coordinate_XY[j];
        }
    }
    DF_LOGV("coeff:%f  %f  %f",  mCalibCoeff[ARRAY_INDEX_0],  mCalibCoeff[ARRAY_INDEX_1],  mCalibCoeff[ARRAY_INDEX_2]);

    for (i = 0; i < ARRAY_SIZE; i++) {
        mCalibCoeff[i] =  powf(mCalibCoeff[i], 1.0/2.2)*255;
        if (mCalibCoeff[i] > temp) {
            temp = mCalibCoeff[i];
        }
    }

    for (i = 0; i < ARRAY_SIZE; i++) {
        if (temp >0) {
            mCalibCoeff[i] = mCalibCoeff[i]/temp;
        } else {
            DF_LOGE(" the max mCalibCoeff is wrong:%f!",  mCalibCoeff[i]);
        }

        if (mCalibCoeff[i] < CALIB_COEFF_THREASHOLD) {
            mCalibCoeff[ARRAY_INDEX_0] = 1.0;
            mCalibCoeff[ARRAY_INDEX_1] = 1.0;
            mCalibCoeff[ARRAY_INDEX_2] = 1.0;
            break;
        }
    }
    DF_LOGV("RGB calib:%f  %f  %f",  mCalibCoeff[ARRAY_INDEX_0],  mCalibCoeff[ARRAY_INDEX_1],  mCalibCoeff[ARRAY_INDEX_2]);

    mWPSCCoeffComputed = true;
    return ret;
}

int DisplayPostprocessing::computeRGBCoeffbyBL(float *rgbCoeff, unsigned int backlight)
{
    DF_LOGV("Enter");
    int i, j = 0;
    int bl_255 = 0;
    float temp = 0.0;
    float x_offset = 0.0f;
    float y_offset = 0.0f;
    float calib_wp_coordinate_xy[ARRAY_SIZE] = {0, 0, 1.0};
    float calib_wp_coordinate_XY[ARRAY_SIZE] = {0, 0, 0};
    float rgb_calib_coeff[ARRAY_SIZE] = {0, 0, 0};
    int A1_Coeff, B1_Coeff, C1_Coeff, A2_Coeff, B2_Coeff, C2_Coeff;
    float delta_x, delta_y;
    int blThreshold[2];
    static long max_backlight = 0;

    if (!max_backlight) {
        if ((DisplayUtil::readMaxBrightness(&max_backlight)) < 0)
            max_backlight = MAX_BL_LEVEL;
    }

    A1_Coeff = mPccParams.ccbbParam.xCoeff[0];
    B1_Coeff = mPccParams.ccbbParam.xCoeff[1];
    C1_Coeff = mPccParams.ccbbParam.xCoeff[2];
    A2_Coeff = mPccParams.ccbbParam.yCoeff[0];
    B2_Coeff = mPccParams.ccbbParam.yCoeff[1];
    C2_Coeff = mPccParams.ccbbParam.yCoeff[2];
    delta_x = mPccParams.ccbbParam.deltaX;
    delta_y = mPccParams.ccbbParam.deltaY;
    blThreshold[0] = mPccParams.ccbbParam.blThreshold[0];
    blThreshold[1] = mPccParams.ccbbParam.blThreshold[1];

    if (rgbCoeff == NULL || backlight == 0 || backlight > max_backlight) {
        DF_LOGW("backlight is over the range");
        return -1;
    }

    if (mWPSCCoeffComputed == 0) {
        calibrateWhitePoint();
        if (mWPSCCoeffComputed == 0) {
            DF_LOGW("White Point calculate fail!");
        }
    }
    DF_LOGV("computeRGBCoeffbyBL backlight:%d BL_255 %d",  backlight, backlight/16);
    /*
      1.backlight level 4096 map to 256 steps
      2.f(x) = a1*x^2 + b1*x + c1,bl>=6
        f(y) = a2*y^2 + b2*y + c2,bl>=50
        x_offset = f(x)/FLOAT_PRECISION
        y_offset = f(y)/FLOAT_PRECISION
      3.bl_wp_xy = wp_xy+xy_offset
      4.[a,b,c]=[RGBcoef 3x3][XYZ 3x1]
      5.wp[a,b,c]*bl[a,b,c]*[r,g,b]
    */
    bl_255 = backlight/16;
    if (bl_255 >= blThreshold[0]) {
        x_offset = A1_Coeff * bl_255 * bl_255 +B1_Coeff * bl_255 - C1_Coeff;
        x_offset = x_offset / FLOAT_PRECISION;
    } else
        x_offset = delta_x;

    if (bl_255 >= blThreshold[1]) {
        y_offset = A2_Coeff * bl_255 * bl_255 +B2_Coeff * bl_255 - C2_Coeff;
        y_offset = y_offset / FLOAT_PRECISION;
    } else
        y_offset = delta_y;

    calib_wp_coordinate_xy[ARRAY_INDEX_0] = mWPXYCoord[ARRAY_INDEX_0] + x_offset;
    calib_wp_coordinate_xy[ARRAY_INDEX_1] = mWPXYCoord[ARRAY_INDEX_1] + y_offset;

    DF_LOGV("offset x_offset:%f, y_offset:%f,  WP x:%f, y:%f",  x_offset, y_offset, mWPXYCoord[0], mWPXYCoord[1]);

    /* white point x y map to X Y*/
    calib_wp_coordinate_XY[ARRAY_INDEX_0] = (calib_wp_coordinate_xy[ARRAY_INDEX_0]/calib_wp_coordinate_xy[ARRAY_INDEX_1])*calib_wp_coordinate_xy[2];
    calib_wp_coordinate_XY[ARRAY_INDEX_1] = 1;
    calib_wp_coordinate_XY[ARRAY_INDEX_2] = calib_wp_coordinate_xy[ARRAY_INDEX_2]*(1-calib_wp_coordinate_xy[ARRAY_INDEX_0]-calib_wp_coordinate_xy[ARRAY_INDEX_1])/calib_wp_coordinate_xy[ARRAY_INDEX_1];

    /* compute calibrate coeff */
    for (i = 0; i < ARRAY_SIZE; i++) {
        for (j = 0; j < ARRAY_SIZE; j++) {
            rgb_calib_coeff[i] += mP3XYZ2RGB[i][j]*calib_wp_coordinate_XY[j];
        }
    }
    DF_LOGV("RGB coeff:%f  %f  %f",  rgb_calib_coeff[ARRAY_INDEX_0],  rgb_calib_coeff[ARRAY_INDEX_1],  rgb_calib_coeff[ARRAY_INDEX_2]);

    for (i = 0; i < ARRAY_SIZE; i++) {
        rgb_calib_coeff[i] =  powf(rgb_calib_coeff[i], 1.0/2.2)*255;
        if (rgb_calib_coeff[i] > temp) {
            temp = rgb_calib_coeff[i];
        }
    }

    for (i = 0; i < ARRAY_SIZE; i++) {
        if (temp >0) {
            rgb_calib_coeff[i] = rgb_calib_coeff[i]/temp;
        } else {
            DF_LOGE(" the max mCalibCoeff is wrong:%f",  rgb_calib_coeff[i]);
        }

        if (rgb_calib_coeff[i] < CALIB_COEFF_THREASHOLD) {
            rgb_calib_coeff[ARRAY_INDEX_0] = 1.0;
            rgb_calib_coeff[ARRAY_INDEX_1] = 1.0;
            rgb_calib_coeff[ARRAY_INDEX_2] = 1.0;
            break;
        }
    }

    DF_LOGV("Inverse gamma RGB coeff:%f %f %f", rgb_calib_coeff[ARRAY_INDEX_0], rgb_calib_coeff[ARRAY_INDEX_1], rgb_calib_coeff[ARRAY_INDEX_2]);
    DF_LOGV("WP coeff:%f  %f  %f",  mCalibCoeff[ARRAY_INDEX_0],  mCalibCoeff[ARRAY_INDEX_1],  mCalibCoeff[ARRAY_INDEX_2]);

    temp = 0;
    float temp_rgb[ARRAY_SIZE] = {0.0};
    for (i = 0; i < ARRAY_SIZE; i++) {
        temp_rgb[i] = rgb_calib_coeff[i] * mCalibCoeff[i];
        if (temp_rgb[i] > temp)
            temp = temp_rgb[i];
    }
    DF_LOGV("Coeff Multiply :%f  %f  %f",  temp_rgb[ARRAY_INDEX_0],  temp_rgb[ARRAY_INDEX_1],  temp_rgb[ARRAY_INDEX_2]);

    for (i = 0; i < ARRAY_SIZE; i++) {
        if (temp >0) {
           temp_rgb[i] = temp_rgb[i]/temp;
           temp_rgb[i] = powf(temp_rgb[i], 2.2);
        } else {
           DF_LOGE(" the max temp_rgb is wrong:%f",  temp_rgb[i]);
        }
    }

    /* configure pcc coeff*/
    rgbCoeff[ARRAY_INDEX_0] *= temp_rgb[ARRAY_INDEX_0];
    rgbCoeff[ARRAY_INDEX_1] *= temp_rgb[ARRAY_INDEX_1];
    rgbCoeff[ARRAY_INDEX_2] *= temp_rgb[ARRAY_INDEX_2];


    DF_LOGV("Gamma RGB coeff:%f %f %f", temp_rgb[ARRAY_INDEX_0], temp_rgb[ARRAY_INDEX_1], temp_rgb[ARRAY_INDEX_2]);
    DF_LOGV("pcc coeff:%f, %f, %f",  rgbCoeff[ARRAY_INDEX_0], rgbCoeff[ARRAY_INDEX_1], rgbCoeff[ARRAY_INDEX_2]);
    return 0;
}

int DisplayPostprocessing::changeWhitePoint(int value)
{
    int ret = 0;
    float x = (float)(value >> 8)/1000 + 0.25;
    float y = (float)(value & 0xFF)/1000 + 0.25;
    ret = calibrateWhitePoint(x, y);
    return ret;
}

int DisplayPostprocessing::computeSREHistogram(unsigned int cmd, uint32_t *sre_hist_need_info)
{
    int ret = -1;
#ifdef HAS_QDCM
    unsigned int hist_weighted = 0;
    int sre_hist_weighted_value = 0;
    static uint32_t last_sre_hist_need_info = 0;
    static int last_sre_hist_weighted_value = 255, time = 0;
    int sre_temp_hist_weighted = 0, sre_use_pixes = 0;
    float percent = 0.0, h1_percent =0.0, h2_percent =0.0, h3_percent =0.0;
    DrmHistBlobValue hist_blob = {};
    HWColorDrmMgr &drmMgr = HWColorDrmMgr::getInstance();
    int totalpixes = 0;

    switch ((enum HANDLE_HIST_CMD)cmd) {
        case HIST_CMD_STOP:
        case HIST_CMD_START: {
            ret = drmMgr.handleHistogramCmd(mDisplayId, SRE_FEATURE, cmd, &hist_blob);
            break;
        }
        case HIST_CMD_GET: {
            ret = drmMgr.handleHistogramCmd(mDisplayId, SRE_FEATURE, cmd, &hist_blob);
            if (ret) {
                DF_LOGE("GetHistogram fail!ret = %d\n", ret);
                return ret;
            }

            DF_LOGV("histogram info flag:%llu", (long long unsigned)hist_blob.flag);
            for (int i = 0; i < 256; i++) {
                totalpixes += hist_blob.blob_data[i];
                hist_weighted += i * hist_blob.blob_data[i];
                if (i <= 25)
                    h1_percent += hist_blob.blob_data[i];
                else if (i <= 50)
                    h2_percent += hist_blob.blob_data[i];
                else if (i <= 75)
                    h3_percent += hist_blob.blob_data[i];

                if (251 == i)  /* SRE just need 1-250*/
                    sre_temp_hist_weighted = i * hist_blob.blob_data[i];
                else if (255 == i)
                    sre_temp_hist_weighted = hist_weighted - sre_temp_hist_weighted - i * hist_blob.blob_data[i];
            }

            DF_LOGV("historigram totalpixes = %d!", totalpixes);

            hist_weighted = hist_weighted / totalpixes;
            DF_LOGV("historigram weighted value is :%d!", hist_weighted);
            /* SRE handle histogram strategy */
            h1_percent = h1_percent- hist_blob.blob_data[0];
            sre_use_pixes = totalpixes - (hist_blob.blob_data[0] +hist_blob.blob_data[251] + hist_blob.blob_data[255]);

            percent = hist_blob.blob_data[255]*1.0 / totalpixes;
            DF_LOGV("histogram blob_data[255] amount percent:%f! sre_use_pixes=%d", percent, sre_use_pixes);
            if (percent > 0.64 || hist_weighted >= 240)
                sre_hist_weighted_value = hist_weighted;
            else
                sre_hist_weighted_value = sre_temp_hist_weighted / sre_use_pixes;

            h1_percent = h1_percent / sre_use_pixes;
            h2_percent = h2_percent / sre_use_pixes;
            h3_percent = h3_percent / sre_use_pixes;

            if (last_sre_hist_weighted_value != sre_hist_weighted_value && time <= 1 && sre_hist_weighted_value != 0) {
                ++time;
                last_sre_hist_weighted_value = sre_hist_weighted_value;
                if (1 == time)
                    time =0;
                *sre_hist_need_info = last_sre_hist_need_info;
                break;
            }

            last_sre_hist_weighted_value = sre_hist_weighted_value;
            last_sre_hist_need_info =  sre_hist_weighted_value << 24 | int(h1_percent*100) << 16 |int(h2_percent*100) << 8  |int(h3_percent*100);
            *sre_hist_need_info = last_sre_hist_need_info;
            DF_LOGV("histogram sre_hist_weighted_value is :%d, h1_percent=%f, h2_percent=%f, h3_percent=%f, sre_hist_info = 0x%x",
                sre_hist_weighted_value, h1_percent, h2_percent, h3_percent, *sre_hist_need_info);

            break;
        }
        default:
            DF_LOGW("%s: command is not support!", __FUNCTION__);
    }
#endif
    return ret;
}
int DisplayPostprocessing::computeGAMESREHistogram(unsigned int cmd, uint32_t *sre_hist_need_info)
{
    int ret = -1;
#ifdef HAS_QDCM
    unsigned int hist_weighted = 0;
    int sre_hist_weighted_value = 0;
    static uint32_t last_sre_hist_need_info = 0;
    static int last_sre_hist_weighted_value = 255, time = 0;
    int sre_temp_hist_weighted = 0, sre_use_pixes = 0;
    float percent = 0.0, h1_percent =0.0, h2_percent =0.0, h3_percent =0.0;
    DrmHistBlobValue hist_blob = {};
    HWColorDrmMgr &drmMgr = HWColorDrmMgr::getInstance();
    int totalpixes = 0;
    switch ((enum HANDLE_HIST_CMD)cmd) {
        case HIST_CMD_STOP:
        case HIST_CMD_START: {
            ret = drmMgr.handleHistogramCmd(mDisplayId, SRE_FEATURE, cmd, &hist_blob);
            break;
        }
        case HIST_CMD_GET: {
            ret = drmMgr.handleHistogramCmd(mDisplayId, SRE_FEATURE, cmd, &hist_blob);
            if (ret) {
                DF_LOGE("GetHistogram fail!ret = %d\n", ret);
                return ret;
            }

            DF_LOGV("histogram info flag:%llu", (long long unsigned)hist_blob.flag);
            for (int i = 0; i < 256; i++) {
                totalpixes += hist_blob.blob_data[i];
                hist_weighted += i * hist_blob.blob_data[i];
                if (i <= 25)
                    h1_percent += hist_blob.blob_data[i];
                else if (i <= 50)
                    h2_percent += hist_blob.blob_data[i];
                else if (i <= 75)
                    h3_percent += hist_blob.blob_data[i];

                if (251 == i)  /* SRE just need 1-250*/
                    sre_temp_hist_weighted = i * hist_blob.blob_data[i];
                else if (46 == i || 47 == i)
                    sre_temp_hist_weighted = i * hist_blob.blob_data[i]+sre_temp_hist_weighted;
                else if (255 == i)
                    sre_temp_hist_weighted = hist_weighted - sre_temp_hist_weighted - i * hist_blob.blob_data[i];
            }

            DF_LOGV("historigram totalpixes = %d!", totalpixes);

            hist_weighted = hist_weighted / totalpixes;
            DF_LOGV("historigram weighted value is :%d!", hist_weighted);
            /* SRE handle histogram strategy */
            h1_percent = h1_percent- hist_blob.blob_data[0];
            sre_use_pixes = totalpixes - (hist_blob.blob_data[0] + hist_blob.blob_data[46] + hist_blob.blob_data[47] + hist_blob.blob_data[251] + hist_blob.blob_data[255]);

            percent = hist_blob.blob_data[255]*1.0 / totalpixes;
            DF_LOGV("histogram blob_data[255] amount percent:%f! sre_use_pixes=%d", percent, sre_use_pixes);
            if (percent > 0.64 || hist_weighted >= 240)
                sre_hist_weighted_value = hist_weighted;
            else
                sre_hist_weighted_value = sre_temp_hist_weighted / sre_use_pixes;

            h1_percent = h1_percent / sre_use_pixes;
            h2_percent = h2_percent / sre_use_pixes;
            h3_percent = h3_percent / sre_use_pixes;

            if (last_sre_hist_weighted_value != sre_hist_weighted_value && time <= 1 && sre_hist_weighted_value != 0) {
                ++time;
                last_sre_hist_weighted_value = sre_hist_weighted_value;
                if (1 == time)
                    time =0;
                *sre_hist_need_info = last_sre_hist_need_info;
                break;
            }

            last_sre_hist_weighted_value = sre_hist_weighted_value;
            last_sre_hist_need_info =  sre_hist_weighted_value << 24 | int(h1_percent*100) << 16 |int(h2_percent*100) << 8  |int(h3_percent*100);
            *sre_hist_need_info = last_sre_hist_need_info;
            DF_LOGV("histogram sre_hist_weighted_value is :%d, h1_percent=%f, h2_percent=%f, h3_percent=%f, sre_hist_info = 0x%x",
                sre_hist_weighted_value, h1_percent, h2_percent, h3_percent, *sre_hist_need_info);

            break;
        }
        default:
            DF_LOGW("%s: command is not support!", __FUNCTION__);
    }
#endif

    return ret;
}

int DisplayPostprocessing::computeVsdr2hdrHistogram(unsigned int cmd, uint32_t *contrast_hist_need_info)
{
    int ret = -1;
#ifdef HAS_QDCM
    float contrast_h1p5 =0.0, contrast_h1 =0.0, contrast_h2 =0.0;
    int contrast_use_pixes = 0;
    unsigned int contrast_hist_weighted = 0;
    uint32_t contrast_temp_info = 0;
    unsigned int hist_weighted = 0;
    DrmHistBlobValue hist_blob = {};
    HWColorDrmMgr &drmMgr = HWColorDrmMgr::getInstance();
    int totalpixes = 0;

    switch ((enum HANDLE_HIST_CMD)cmd) {
        case HIST_CMD_STOP:
        case HIST_CMD_START: {
            ret = drmMgr.handleHistogramCmd(mDisplayId, VSDR2HDR_FEATURE, cmd, &hist_blob);
            break;
        }
        case HIST_CMD_GET: {
            ret = drmMgr.handleHistogramCmd(mDisplayId, VSDR2HDR_FEATURE, cmd, &hist_blob);
            if (ret) {
                DF_LOGE("GetHistogram fail!ret = %d\n", ret);
                return ret;
            }

            DF_LOGV("histogram info flag:%llu", (long long unsigned)hist_blob.flag);

            for (int i = 0; i < 256; i++) {
                totalpixes += hist_blob.blob_data[i];
                hist_weighted += i * hist_blob.blob_data[i];
                if (i <= 25) {
                    if (i <= 15)
                        contrast_h1p5 += hist_blob.blob_data[i];
                    contrast_h1 += hist_blob.blob_data[i];
                } else if (i <= 50)
                    contrast_h2 += hist_blob.blob_data[i];
                if (252 == i)  /* HDR just need 1-252*/
                    contrast_hist_weighted = hist_weighted;
            }

            contrast_h1p5 = contrast_h1p5 - hist_blob.blob_data[0];
            contrast_h1 = contrast_h1- hist_blob.blob_data[0];
            contrast_use_pixes = totalpixes - (hist_blob.blob_data[0] +hist_blob.blob_data[253]
                    + hist_blob.blob_data[254] + hist_blob.blob_data[255]);

            contrast_h1p5 = contrast_h1p5 /contrast_use_pixes;
            contrast_h1 = contrast_h1 / contrast_use_pixes;
            contrast_h2 = contrast_h2 / contrast_use_pixes;
            contrast_hist_weighted = contrast_hist_weighted / contrast_use_pixes;

            if (3 == last_contrast_hist_info && contrast_h1 < 0.1 && contrast_h2 > 0.1 && contrast_hist_weighted < 60)
                contrast_temp_info = last_contrast_hist_info;
            else if (3 == last_contrast_hist_info && contrast_h1p5 < 0.08 && contrast_h2 > 0.08 && contrast_hist_weighted < 60)
              contrast_temp_info = last_contrast_hist_info;
            else if (contrast_h1 < 0.1 && contrast_h2 > 0.1 && contrast_hist_weighted > 50)
                contrast_temp_info = 2;
            else if (contrast_h1p5 < 0.08 && contrast_h2 > 0.08 && contrast_hist_weighted > 50)
                contrast_temp_info = 1;
            else if (contrast_hist_weighted > 50)
                contrast_temp_info = 0;
            else {
                if (0 == last_contrast_hist_info && contrast_hist_weighted > 40)
                    contrast_temp_info = last_contrast_hist_info;
                else
                    contrast_temp_info = 3;
            }

            if (last_contrast_hist_info != contrast_temp_info && contrast_hist_jump_time <= 1) {
                ++contrast_hist_jump_time;
                last_contrast_hist_info = contrast_temp_info;
                *contrast_hist_need_info = last_real_set_hist_info;
                if (1 == contrast_hist_jump_time)
                    contrast_hist_jump_time =0;
                break;
            }
            last_real_set_hist_info = last_contrast_hist_info;
            *contrast_hist_need_info = last_real_set_hist_info;

            DF_LOGV("histogram contrast_h1p5=%f, contrast_h1=%f, contrast_h2=%f, contrast_hist_weighted = %d, contrast_hist_info = %d",
            contrast_h1p5, contrast_h1, contrast_h2, contrast_hist_weighted, *contrast_hist_need_info);
            break;
        }
        default:
            DF_LOGW("%s: command is not support!", __FUNCTION__);
    }
#endif
    return ret;
}

int DisplayPostprocessing::computeIGCDitherHistogram(unsigned int cmd, uint32_t *igcDither_hist_need_info)
{
    int ret = -1;
#ifdef HAS_QDCM
    int totalpixels = 0;
    int zeroPixels = 0;
    DrmHistBlobValue hist_blob = {};

    HWColorDrmMgr &drmMgr = HWColorDrmMgr::getInstance();
    switch ((enum HANDLE_HIST_CMD)cmd) {
        case HIST_CMD_STOP:
        case HIST_CMD_START: {
            ret = drmMgr.handleHistogramCmd(mDisplayId, IGC_DITHER, cmd, &hist_blob);
            break;
        }
        case HIST_CMD_GET: {
            ret = drmMgr.handleHistogramCmd(mDisplayId, IGC_DITHER, cmd, &hist_blob);
            if (ret) {
                DF_LOGE("GetHistogram fail!ret = %d\n", ret);
                return ret;
            }
            zeroPixels = hist_blob.blob_data[0];
            for (int i = 0; i < 256; i++) {
                totalpixels += hist_blob.blob_data[i];
            }
            *igcDither_hist_need_info = zeroPixels * 100 / totalpixels;
        } break;
    }
#endif
    return ret;
}

int DisplayPostprocessing::analyzeHistogram(unsigned int cmd, int *weighted_value)
{
    int ret = -1;
#ifdef HAS_QDCM
    unsigned int hist_weighted = 0;
    DrmHistBlobValue hist_blob = {};
    HWColorDrmMgr &drmMgr = HWColorDrmMgr::getInstance();
    int totalpixes = 0;

    switch ((enum HANDLE_HIST_CMD)cmd) {
    case HIST_CMD_STOP:
    case HIST_CMD_START: {
        ret = drmMgr.handleHistogramCmd(mDisplayId, BCBC_FEATURE, cmd, &hist_blob);
        break;
    }
    case HIST_CMD_GET: {
        ret = drmMgr.handleHistogramCmd(mDisplayId, BCBC_FEATURE, cmd, &hist_blob);
        if (ret) {
            DF_LOGE("GetHistogram fail!\n");
            return ret;
        }

        mHistGray32 = 0;
        for (int i = 0; i < 256; i++) {
            totalpixes += hist_blob.blob_data[i];
            hist_weighted += i * hist_blob.blob_data[i];
            if (i > 32) {
                mHistGray32 += hist_blob.blob_data[i];
            }
        }

        mHistGray32 = mHistGray32 * 100 / totalpixes;
        DF_LOGV("histogram gray32+:%d!", mHistGray32);
        if (weighted_value) {
            *weighted_value = hist_weighted / totalpixes;
            DF_LOGV("historigram weighted value is :%d!", *weighted_value);
        }

        break;
    }
    default:
          DF_LOGW("%s: command is not support!", __FUNCTION__);
    }
#endif
    return ret;
}

int DisplayPostprocessing::computeHistogram(unsigned int cmd)
{
    int i, ret = 0;
#ifdef HAS_QDCM
    int weighted_value = 0;
    long duration = 0;
    enum DISPLAY_HIST_LEVEL hist_level = HIST_LEVEL_3;

    ret = analyzeHistogram(cmd, &weighted_value);
    if (ret) {
        DF_LOGW("%s: analyze histogram fail", __FUNCTION__);
        return ret;
    }

    if (cmd == HIST_CMD_STOP) {
        /* stop histogram analyze*/
        mHistParse.hist_weighted = HIST_LEVEL_3;
        mHistParse.weight_value = 0;
        mHistParse.changed = 0;

        /* notify framework to recovery default value*/
        DF_LOGD("mDisplayId=%d notify framework stop, V:%d!", mDisplayId, mHistLevelInterval[HIST_LEVEL_3]);
        DisplayUtil::onCallback(DISPLAY_INFO_GRAY, mHistLevelInterval[HIST_LEVEL_3]);
    }

    DF_LOGV("weighted_value :%d, hist_value:%d,cmd:%d!", weighted_value,
           mHistParse.hist_weighted, cmd);

    if (cmd == HIST_CMD_GET && weighted_value != 0) {
        /* adjust the V level*/
        for (i = 0; i < HIST_LEVEL_MAX; i++) {
            if (weighted_value <=  mHistLevelInterval[i]) {
                hist_level = ( enum DISPLAY_HIST_LEVEL)i;
                break;
            }
        }

        if (hist_level >= HIST_LEVEL_MAX) {
            return ret;
        }

        if (mHistParse.hist_weighted != hist_level) {
            DF_LOGV("hist_weighted changed, new weight:%d!", weighted_value);
            if (abs(mHistParse.weight_value - weighted_value) > V_HIST_WEIGHT_WINDOW) {
                mHistParse.hist_weighted = hist_level;
                mHistParse.weight_value = weighted_value;
                mHistParse.current_time = systemTime();
                mHistParse.changed= 1;
            }
        } else if (mHistParse.changed == 1) {
            duration = long(ns2ms(systemTime() - mHistParse.current_time));
            if (duration > V_HIST_DEBOUNCE_MS) {
                /* notify framework*/
                DF_LOGD("mDisplayId=%d notify framework changed, V:%d!", mDisplayId, mHistLevelInterval[mHistParse.hist_weighted]);
                DisplayUtil::onCallback(DISPLAY_INFO_GRAY, mHistLevelInterval[mHistParse.hist_weighted]);
                mHistParse.changed = 0;
           }
        }
    }
#endif
    return ret;
}

int DisplayPostprocessing::setESSFunction(int value)
{
    int ret = 0;
    if (mDSPP.get())
        ret = mDSPP->setESSFunction(value);

    return ret;
}

int DisplayPostprocessing::setPQMode(int value)
{
    int ret = 0;
    if (mDSPP.get())
        ret = mDSPP->setPQMode(value);

    return ret;
}

int DisplayPostprocessing::setGamePQMode(int value)
{
    int ret = 0;
    if (mDSPP.get())
        ret = mDSPP->setGamePQMode(value);

    return ret;
}

int DisplayPostprocessing::setDREFunction(int value)
{
    int ret = 0;
    if (mDSPP.get())
        ret = mDSPP->setDREFunction(value);

    return ret;
}

void DisplayPostprocessing::dumpPCCParam(std::string& result)
{
    char res_ch[1024] = {0};
    result.append("\nPcc:\n");
    //PCC param
    snprintf(res_ch, sizeof(res_ch), "CT:    [%f, %f, %f], EXP: [%f, %f, %f], BRI: [%f], DC: [%f]\n",
             mCTCoeff[0], mCTCoeff[1], mCTCoeff[2], mExpertCoeff[0], mExpertCoeff[1], mExpertCoeff[2],
             mBrightnessCoeff[0],mDCBrightnessCoeff[0]);
    result.append(std::string(res_ch));
    snprintf(res_ch, sizeof(res_ch), "SLS:   [%f, %f, %f, %f, %f, %f, %f, %f, %f]\n",
             mSLScreenCoeff[0], mSLScreenCoeff[1], mSLScreenCoeff[2], mSLScreenCoeff[3], mSLScreenCoeff[4],
             mSLScreenCoeff[5], mSLScreenCoeff[6], mSLScreenCoeff[7], mSLScreenCoeff[8]);
    result.append(std::string(res_ch));
    snprintf(res_ch, sizeof(res_ch), "TT:    [%f, %f, %f, %f, %f, %f, %f, %f, %f]\n",
             mTrueToneCoeff[0], mTrueToneCoeff[1], mTrueToneCoeff[2], mTrueToneCoeff[3], mTrueToneCoeff[4],
             mTrueToneCoeff[5], mTrueToneCoeff[6], mTrueToneCoeff[7], mTrueToneCoeff[8]);
    result.append(std::string(res_ch));
    snprintf(res_ch, sizeof(res_ch), "PP:    [%f, %f, %f, %f, %f, %f, %f, %f, %f]\n",
             mPaperModeCoeff[0], mPaperModeCoeff[1], mPaperModeCoeff[2], mPaperModeCoeff[3], mPaperModeCoeff[4],
             mPaperModeCoeff[5], mPaperModeCoeff[6], mPaperModeCoeff[7], mPaperModeCoeff[8]);
    result.append(std::string(res_ch));
    snprintf(res_ch, sizeof(res_ch), "FINAL: [%f, %f, %f, %f, %f, %f, %f, %f, %f]\n",
             pccCoeff[0], pccCoeff[1], pccCoeff[2], pccCoeff[3], pccCoeff[4], pccCoeff[5], pccCoeff[6], pccCoeff[7], pccCoeff[8]);
    result.append(std::string(res_ch));
}
}
