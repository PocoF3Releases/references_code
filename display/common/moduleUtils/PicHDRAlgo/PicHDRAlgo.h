#ifndef __PICTURE_HDR_ALGO_H__
#define __PICTURE_HDR_ALGO_H__

#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>
#include <math.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include <mi_stc_service.h>
#include <vector>
#include "DFComDefine.h"
#include "parseXml.h"
#include "display_effects.h"
#include "DisplayFeatureHal.h"
#include "display_color_processing.h"

using namespace std;
namespace android {
#define  PI 3.14159265358979323846
// enum MiImplType {
//     kMiImplPrimary = 0,
//     kMiImplSecondaryBuiltIn,
//     kMaxNumImpl,
// };
class PicHDRAlgo : public RefBase
{
public:
    PicHDRAlgo();
    ~PicHDRAlgo(){}
    int calcHDRLut(struct rgb_entry *lutInfo);
    void SetExif(double Adrc, double LuxIndex);

private:
    void Init(int disp_id);
    int readLutData(char* path);
    void H2C_init();
    void H2L_init();
    double* scene_judge(double lux_index);
    double Slope_cal(double lux_index);
    void C_tuning(int * H_range, double Cp, double Lp, int trans);
    double Lth_cal(double adrc);
    double* Lr_bound_cal(double adrc, double* Lr_start_range, double slope);
    double** get_inv(double** mat);
    double** RGB2XYZ_gen(double R[2], double G[2], double B[2], double W[2]);
    double* RGB2XYZ_cal(int R, int G, int B, double** RGB2XYZ, double WY);
    int* XYZ2RGB_cal(double XYZ[3], double** XYZ2RGB, double WY);
    double f(double x);
    double* xylv2XYZ_cal(double Wxy[2], double WY);
    double* XYZ2Lab_cal(double XYZ[3], double XYZW[3]);
    double* Lab2LCH_cal(double Lab[3]);
    double* LCH2Lab_cal(double LCH[3]);
    double* Lab2XYZ_cal(double Lab[3], double XYZW[3]);

    std::vector<double> Rp;
    std::vector<double> Gp;
    std::vector<double> Bp;
    std::vector<double> Wp;
    double WYp = 471.15329;//Screen panel native RGBW, written in xml

    std::vector<double> Rc;
    std::vector<double> Gc;
    std::vector<double> Bc;
    std::vector<double> Wc;
    double WYc = 471.15329;//LUT output corresponding RGBW

    double mAdrc = 0;//Adrc value read from image as HDR info ranging from 1 to 16 and the value is mostly located between 1 to 5
    double mLuxIndex = 0;

    double Lth_left = 50;
    double Lth_right = 85;//Luminance threshold range, written in xml

    double Lr_start_min_day = 0.6;
    double Lr_start_max_day = 0.9;//L ratio start point range for day scene
    double Lr_start_min_night = 0.5;
    double Lr_start_max_night = 0.8;//L ratio start point range for night scene

    double adrc_left = 1;
    double adrc_mid = 5;
    double adrc_right = 16;//Adrc value range and the commonly-used range distinguished by adrc_mid, written in xml
    double lux_index_th_left = 300;
    double lux_index_th_right = 360;//Lux index threshold for judging day and night scene

    double slope_left = 2.0/3;//Dark colors' enhanced slope, written in xml
    double slope_right = 1;

    double H2C[360] = {};//Hue and corresponding saturation
    double H2L[360] = {};//add for skin enhance
    std::vector<int> H_tuning_range;//Customize hue range for saturation tuning purpose
    std::vector<double> Cp;//Saturation tuning parameters
    std::vector<double> Lp;//add for skin enhance
    double LCH_in[4913][3];
    sp<MiParseXml> mParseXml[2];
}; // class PicHDRAlgo
} // namespace android
#endif //__PICTURE_HDR_ALGO_H__