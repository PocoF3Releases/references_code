#ifndef _DISPLAY_POSTPROCESSING_H_
#define _DISPLAY_POSTPROCESSING_H_

#include <pthread.h>
#include <sys/poll.h>
#include <utils/Timers.h>
#include <utils/Singleton.h>
#include <cutils/properties.h>
#include "display_utils.h"
#include "DisplayFeatureHal.h"
#include "DSPP_Platform.h"
#ifndef MTK_PLATFORM
#include "mi_stc_service.h"
#endif
#include "cust_define.h"
#include "hw/hw_color_drm_manager.h"
#include "eyecare.h"

enum HANDLE_HIST_CMD {
    HIST_CMD_STOP,
    HIST_CMD_START,
    HIST_CMD_GET,
    HIST_CMD_MAX,
};

#define CCBB_LOG(fmt, args...) \
    do { \
        if (DisplayPostprocessing::ccbb_log_en) \
            ALOGD(fmt, ##args); \
        else \
            ALOGV(fmt, ##args); \
    } while (0)
typedef struct UnlimitedCTParams {
    int strength;
    int def[3];
    int cold[3];
    int warm[3];
    int uiParamMin;
    int paramMax;
    int dimmingTime;
    int dimmingStep;
} UnlimitedCTParams;
typedef struct ccbbParams {
    int xCoeff[3];
    int yCoeff[3];
    int blThreshold[2];
    float deltaX;
    float deltaY;
} ccbbParams;
typedef struct wpCalibParams {
    float targetWP[2];
    float targetXY[3];
    float rgbCoodinatexyl[3][3];
    float xRange[2];
    float yRange[2];
    float xyComp[2];
    float xyOffset[2];
} wpCalibParams;

typedef struct SunlightScreenParams {
    int level;
    int delay;
    double coeff[SUNLIGHT_SCREEN_LEVEL_MAX][9];
    int luxThreshold[SUNLIGHT_SCREEN_LEVEL_MAX-1];
} SunlightScreenParams;

typedef struct PccGABBParams {
    double GABBCoeff[9][9];
    double threshold;
} PccGABBParams;

typedef struct DcBacklightParams {
    int minBL;
    int threshold;
    double startPcc;
    double step;
} DcBacklightParams;

typedef struct SoftBlParams {
    double pccLut[400];
    double pccLutDC[400];
} SoftBlParams;

typedef struct TrueToneParams {
    int NativeCCTZone[2];
    double D65Coeffs[9][4];
    double D75Coeffs[9][4];
} TrueToneParams;

typedef struct PaperModeParams {
    double coeff[3][9];
} PaperModeParams;

typedef struct PccParams {
    UnlimitedCTParams uctParam;
    ccbbParams ccbbParam;
    wpCalibParams wpCalibParam;
    SunlightScreenParams slsParams;
    PccGABBParams GABBParams;
    DcBacklightParams DCBLParams;
    SoftBlParams softBlParams;
    TrueToneParams trueToneParams;
    PaperModeParams PaperParams;
} PccParams;

namespace android {

enum DISPLAY_FEATURE_TYPE {
     DISPLAY_FEATURE_CCBB,
     DISPLAY_FEATURE_REFRESH,
     DISPLAY_CAL_WP,
     DISPLAY_CHANGE_WP,
     DISPLAY_HAL_CMD,
     DISPLAY_CT_ADJUST,
     DISPLAY_FEATURE_HIST,
     DISPLAY_SUNLIGHT_SCREEN,
     DISPLAY_BREAK_CT_DIMMING,
     DISPLAY_EYECARE_MODE,
     DISPLAY_CLEAR_PCC,
     DISPLAY_RESTORE_PCC,
     DISPLAY_GABB,
     DISPLAY_HDR_PCC,
     DISPLAY_DRE,
     DISPLAY_CABC,
     DISPLAY_PQ_MODE,
     DISPLAY_EXPERT_MODE,
     DISPLAY_BL,
     DISPLAY_GRAY_MODE,
     DISPLAY_TRUE_TONE,
     DISPLAY_PAPER_MODE,
     DISPLAY_PP_DEBUG,
     DISPLAY_BREAK_SLPCC_DIMMING,
     DISPLAY_DC,
     DISPLAY_GAME_PQ_MODE,
     DISPLAY_DITHER,
     DISPLAY_FEATURE_MAX,
};

enum DISPLAY_HIST_LEVEL {
    HIST_LEVEL_0,
    HIST_LEVEL_1,
    HIST_LEVEL_2,
    HIST_LEVEL_3,
    HIST_LEVEL_4,
    HIST_LEVEL_5,
    HIST_LEVEL_6,
    HIST_LEVEL_7,
    HIST_LEVEL_8,
    HIST_LEVEL_MAX,
};

enum CT_DIMMING_LEVEL {
    CT_DIMMING_NONE,
    CT_DIMMING_ENABLE,
};

enum CT_DIMMIN_BREAK {
    CT_DIMMING_NON_BREAK,
    CT_DIMMING_BREAK,
};

enum PIPE_FDS_INDEX {
    PIPE_READ,
    PIPE_WRITE,
    PIPE_INDEX_MAX,
};

enum DISP_DRE_OPT {
    DRE_DISABLE,
    DRE_ENABLE,
};

enum DISP_CABC_OPT {
    CABC_OFF,
    CABC_UI_ON,
    CABC_MOVIE_ON,
    CABC_STILL_ON,
};

enum DISP_CABC_STRENGTH {
    CABC_UI_STRENGTH = 24,/*10%*/
    CABC_MOVIE_STRENGTH = 68,/*30%*/
    CABC_STILL_STRENGTH = 148,/*60%*/
};

enum DISP_PP_DEBUG_TYPE {
    DISP_PP_CCBB_LOG,
    DISP_PP_CCBB_ENABLE,
};

typedef struct {
    unsigned int changed;
    enum DISPLAY_HIST_LEVEL hist_weighted;
    int weight_value;
    nsecs_t current_time;
} HistogramAnalyze;

class DisplayPostprocessing: public RefBase
{
public:
    DisplayPostprocessing(int displayId);
    ~DisplayPostprocessing();
    int setPostprocessingMode(struct DispPPParam *pSDPPParams);
    void setParams(void* params);
    void dumpPCCParam(std::string& result);
protected:
    void checkParameterValid(int *param, int min, int max);
    void composePccCoeffs(double *rgbCoeff);
    int setPCCConfig();
    int clearPccConfig();
    int restorePccConfig();
    int setPCCConfigCT(int value, int enable_dimming);
    int setPCCConfigCCBB(int value, bool enable);
    void setPCCConfigSunlightScreenDimming(int value);
    int setPCCConfigSunlightScreen(int value, int cookie);
    int setPCCConfigGABB(int brightness);
    int setHDRPcc(int value);
    int forceScreenRefresh();
    int calibrateWhitePoint(float x=0.0f, float y=0.0f);
    int computeRGBCoeffbyBL(float *rgbCoeff, unsigned int backlight);
    int changeWhitePoint(int value);
    int computeSREHistogram(unsigned int cmd, uint32_t *sre_hist_need_info);
    int computeGAMESREHistogram(unsigned int cmd, uint32_t *sre_hist_need_info);
    int computeVsdr2hdrHistogram(unsigned int cmd, uint32_t *contrast_hist_need_info);
    int computeIGCDitherHistogram(unsigned int cmd, uint32_t *igcDither_hist_need_info);
    int analyzeHistogram(unsigned int cmd, int *weighted_value);
    int computeHistogram(unsigned int cmd);
    void setBreakCTDimming(int value);
    int setPCCConfigEyeCare(int value);
    int setPCCConfigBacklight(int brightness);
    int setPCCConfigGrayMode(int enable);
    int setDREFunction(int value);
    int setESSFunction(int value);
    int setPQMode(int value);
    int setGamePQMode(int value);
    int setPCCConfigBL(struct DispPPParam *pSDPPParams);
    int setPCCConfigDC(struct DispPPParam *pSDPPParams);
    int setPCCConfigExpert(int rgb);
    int setPCCConfigPaperMode(struct DispPPParam *pSDPPParams);
    int setPCCConfigTrueTone(struct DispPPParam *pSDPPParams);
private:
    bool ccbb_enable = true;
    bool ccbb_log_en = true;
    bool mWPSCCoeffComputed;
    bool mWhitePointCalibrationEnable;
    float mWPXYCoord[ARRAY_SIZE] = {0.0, 0.0, 1.0};
    float mCalibCoeff[ARRAY_SIZE] = {1.0, 1.0, 1.0};
    float mP3XYZ2RGB[ARRAY_SIZE][ARRAY_SIZE] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    double mCCBBCoeff[ARRAY_SIZE] = {1.0, 1.0, 1.0};
    double mCTCoeff[ARRAY_SIZE] = {1.0, 1.0, 1.0};
    double mSLScreenCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
    double mGABBCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
    double pccCoeff[ARRAY_SIZE_EXP] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

    double mExpertCoeff[ARRAY_SIZE] = {1.0, 1.0, 1.0};
    double mBrightnessCoeff[ARRAY_SIZE] = {1.0, 1.0, 1.0};
    double mDCBrightnessCoeff[ARRAY_SIZE] = {1.0, 1.0, 1.0};
    double mGrayModeCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
    double mTrueToneCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
    double mPaperModeCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
    double mEyeCareCoeffExp[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
    int mHistGray32;
    int mHistLevelInterval[HIST_LEVEL_MAX];
    HistogramAnalyze mHistParse;

    int break_ct = 0;
    int mHDREnable = 0;

    int break_SLPCC = 0;
    int mLastSLLevel = 0;
    int mLastSLCookie = 0;

    int mDisplayId = 0;
    PccParams mPccParams;

    Mutex mHistGetLock;

    /* VSDR2HDR */
    int last_contrast_hist_info = 3;
    int last_real_set_hist_info = 3;
    int contrast_hist_jump_time = 0;
    sp<DSPP_Platform> mDSPP = nullptr;
    int m12BitBl = -1;
#ifdef MTK_PLATFORM
    sp<EyeCare> mEyeCare[DISPLAY_MAX] = {nullptr};
#endif
};

class ModeTransform{
    public:
        ModeTransform(){}
        ModeTransform(double* InputCoeff){
            applyTransform(InputCoeff);
        }
        ~ ModeTransform(){}

        ModeTransform &applyTransform(double* InputCoeff){
            double TempCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
            memcpy(TempCoeff, OutputCoeff, sizeof(OutputCoeff));
            for (int i = 0; i < ARRAY_SIZE; i++){
                for (int j = 0; j < ARRAY_SIZE; j++) {
                  OutputCoeff[3 * i + j] =  InputCoeff[3 * i] * TempCoeff[0 + j] \
                                            + InputCoeff[3 * i + 1] * TempCoeff[3 + j] \
                                            + InputCoeff[3 * i + 2] * TempCoeff[6 + j];
                }
            }
            DF_LOGV("R[r %f, g %f, b %f], G[r %f, g %f, b %f], B[r %f, g %f, b %f]",
            OutputCoeff[0],OutputCoeff[1],OutputCoeff[2],OutputCoeff[3],OutputCoeff[4],OutputCoeff[5],OutputCoeff[6],OutputCoeff[7],OutputCoeff[8]);
            return *this;
        }
        ModeTransform &applyTransform(ModeTransform* InputCoeff){
            double TempCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
            memcpy(TempCoeff, OutputCoeff, sizeof(OutputCoeff));
            for (int i = 0; i < ARRAY_SIZE; i++)
                for (int j = 0; j < ARRAY_SIZE; j++) {
                  OutputCoeff[3 * i + j] =  (*InputCoeff)[3 * i] * TempCoeff[0 + j] \
                                            + (*InputCoeff)[3 * i + 1] * TempCoeff[3 + j] \
                                            + (*InputCoeff)[3 * i + 2] * TempCoeff[6 + j];
            }
            DF_LOGV("R[r %f, g %f, b %f], G[r %f, g %f, b %f], B[r %f, g %f, b %f]",
            OutputCoeff[0],OutputCoeff[1],OutputCoeff[2],OutputCoeff[3],OutputCoeff[4],OutputCoeff[5],OutputCoeff[6],OutputCoeff[7],OutputCoeff[8]);
            return *this;
        }
        double operator[](int i){
            return OutputCoeff[i];
        }
        void prfTrans(char* title){
            DF_LOGV("%s: R[r %f, g %f, b %f], G[r %f, g %f, b %f], B[r %f, g %f, b %f]",
            title,OutputCoeff[0],OutputCoeff[1],OutputCoeff[2],OutputCoeff[3],OutputCoeff[4],OutputCoeff[5],OutputCoeff[6],OutputCoeff[7],OutputCoeff[8]);
        }
    private:
        double OutputCoeff[ARRAY_SIZE_EXP] = {1.0, 0, 0, 0, 1.0, 0, 0, 0, 1.0};
};
class RgbTransform{
    public:
        RgbTransform(){}
        RgbTransform(double* InputCoeff){
            applyTransform(InputCoeff);
        }
        ~ RgbTransform(){}
        RgbTransform &applyTransform(double* InputCoeff){
            for(int i = 0; i < ARRAY_SIZE; i++){
                OutputCoeff[i] = OutputCoeff[i] * InputCoeff[i];
                if (OutputCoeff[i] > mMax)
                    mMax = OutputCoeff[i];
            }
            DF_LOGV("r %f, g %f, b %f",OutputCoeff[0],OutputCoeff[1],OutputCoeff[2]);
            return *this;
        }
        RgbTransform &normalizeTransform(){
            for(int i = 0; i < ARRAY_SIZE; i++){
                OutputCoeff[i] = OutputCoeff[i] / mMax;
            }
            DF_LOGV("r %f, g %f, b %f",OutputCoeff[0],OutputCoeff[1],OutputCoeff[2]);
            return *this;
        }
        double operator[](int i){
            return OutputCoeff[i];
        }
        double getMax(){
            return mMax;
        }
        void prfTrans(char* title){
            DF_LOGV("%s: r %f, g %f, b %f",title,OutputCoeff[0],OutputCoeff[1],OutputCoeff[2]);
        }
    private:
        double OutputCoeff[ARRAY_SIZE_EXP] = {1.0, 1.0, 1.0};
        double mMax = 1;
};
};
#endif //_DISPLAY_POSTPROCESSING_H_
