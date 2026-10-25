#ifndef _EYECARE_H_
#define _EYECARE_H_

#include <iostream>
#include <stdio.h>
#include <string>
#include <math.h>
#include <algorithm>
#include <ctime>
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <fstream>
#include <cutils/properties.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <vector>
#include "parseXml.h"
#include "display_effects.h"
#include "DisplayFeatureHal.h"
#include "display_color_processing.h"

using namespace std;
namespace android {
#define MAX_LOOP_CNT 500
typedef struct EyeCareParams {
    int minCCT;
    int maxCCT;
    int startLevel;
    int maxLevel;
    int cctThreshold;
    float step;
    int BlueThreshold;
    float SatThreshold;
    float mHueLeftBound;
    float mHueRightBound;
    float mHueCenter;
    float mHueTopGain;
    std::vector<double> regModel;
    std::vector<double> xCoeff0;
    std::vector<double> yCoeff0;
    std::vector<double> xCoeff1;
    std::vector<double> yCoeff1;
    std::vector<int> srcBRatio;
    std::vector<int> targetBRatio;
} EyeCareParams;

enum PaperColorId {
    FULL_COLOR,
    LIGHT_COLOR,
    BLACK_WITHE,
};

class EyeCare : public RefBase
{
public:
    EyeCare(int display_id);
    virtual ~EyeCare() { };
    void SetEyeCare(int value, int cookie, struct rgb_entry *lutInfo);
    void SetGoogleEffectEnable(int enable);
    void SetPaperModeId(int id);
    void SetLogLevel(int level);
    int displayId = DISPLAY_PRIMARY;

private:
    void init();
    void get_inv(double mat[3][3], double inv[3][3]);
    void mapping(double Wx, double Wy, double RGB2XYZ[3][3]);
    void xy_gen(int t, double* xy);
    double rgb2bit(double inp);
    double bit2rgb(double inp);
    void rgb2hsv(int r, int g, int b, double* hsv);
    void hsv2rgb(double h, double s, double v, int* rgb);
    double suppress(double tl, double tr, double l, double r, double inp);
    void gray_trans(int r, int g, int b, int t, int* rgb);
    double calcGain(double hue);
    double getBlueRatio(int r, int g, int b);
    int IsGrayLut(int index);
    sp<MiParseXml> mParseXml[DISPLAY_MAX];
    int mInitDone = 0;
    EyeCareParams mEyeCareParam;
    double mat_XYZ[3][3] = { { 0.68, 0.265, 0.15 },{ 0.32, 0.69, 0.06 },{ 0, 0.045, 0.79 } }; // {{Rx,Gx,Bx},{Ry,Gy,By},{Rz,Gz,Bz}} in P3 gamut.
    double White_xy[2] = {0.3, 0.315}; // Standard D75 white xy.
    int mGoogleEffect = 0;
    int mPaperModeId = FULL_COLOR;
    double RGB2XYZ_std[3][3] = {};
}; //Class EyeCare
}  //namespace android
#endif

