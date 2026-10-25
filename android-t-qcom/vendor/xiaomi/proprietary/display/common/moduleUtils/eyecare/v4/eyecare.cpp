#include "eyecare.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "EyeCare"
#endif

using namespace std;
namespace android {
int DFLOG::loglevel = 1;
EyeCare::EyeCare(int display_id)
    : displayId(display_id)
{
    mParseXml[displayId] = new MiParseXml();
    if (mParseXml[displayId].get()) {
        init();
    }

}

void EyeCare::init()
{
    char temp[50];
    const unsigned int max_count = 1024;
    char *tokens[max_count] = {NULL};
    unsigned int count = 0;
    int ret = 0;
    char p[4096];

    string key("EyeCareMinCCT");
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mEyeCareParam.minCCT = atoi(p);
        DF_LOGV("mEyeCareParam.minCCT: %d",atoi(p));
    }
    key = "EyeCareMaxCCT";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mEyeCareParam.maxCCT = atoi(p);
        DF_LOGV("mEyeCareParam.maxCCT: %d",atoi(p));
    }
    key = "EyeCareStartLevel";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mEyeCareParam.startLevel = atoi(p);
        DF_LOGV("mEyeCareParam.startLevel: %d",atoi(p));
    }
    key = "EyeCareMaxLevel";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mEyeCareParam.maxLevel = atoi(p);
        DF_LOGV("mEyeCareParam.maxLevel: %d",atoi(p));
    }
    key = "EyeCareStep";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mEyeCareParam.step = atof(p);
        DF_LOGV("mEyeCareParam.step: %f",atof(p));
    }
    key = "EyeCareCCTThreshold";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mEyeCareParam.cctThreshold = atoi(p);
        DF_LOGV("mEyeCareParam.cctThreshold: %d",atoi(p));
    }
    key = "EyeCareRegModel";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0 || count != 4) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            mEyeCareParam.regModel.push_back(atof(tokens[i]));
            DF_LOGV("eyecare regModel[%d]:%f",i,mEyeCareParam.regModel[i]);
        }
    }
    key = "EyeCareXCoeff0";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0 || count != 4) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            mEyeCareParam.xCoeff0.push_back(atof(tokens[i]));
            DF_LOGV("eyecare xCoeff0[%d]:%f",i,mEyeCareParam.xCoeff0[i]);
        }
    }
    key = "EyeCareXCoeff1";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0 || count != 4) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            mEyeCareParam.xCoeff1.push_back(atof(tokens[i]));
            DF_LOGV("eyecare xCoeff1[%d]:%f",i,mEyeCareParam.xCoeff1[i]);
        }
    }
    key = "EyeCareYCoeff0";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0 || count != 4) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            mEyeCareParam.yCoeff0.push_back(atof(tokens[i]));
            DF_LOGV("eyecare yCoeff0[%d]:%f",i,mEyeCareParam.yCoeff0[i]);
        }
    }
    key = "EyeCareYCoeff1";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0 || count != 4) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            mEyeCareParam.yCoeff1.push_back(atof(tokens[i]));
            DF_LOGV("eyecare yCoeff1[%d]:%f",i,mEyeCareParam.yCoeff1[i]);
        }
    }
    key = "EyeCareBlueThreshold";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mEyeCareParam.BlueThreshold = atoi(p);
        DF_LOGV("mEyeCareParam.BlueThreshold: %d",atoi(p));
    }
    key = "EyeCareSrcBlueRatio";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0 || count != 2) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            mEyeCareParam.srcBRatio.push_back(atoi(tokens[i]));
            DF_LOGV("eyecare srcBRatio[%d]:%d",i,mEyeCareParam.srcBRatio[i]);
        }
    }
    key = "EyeCareTargetBlueRatio";
    ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
    if(ret != 0 || count != 2) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        for (int i = 0; i < count; i++) {
            mEyeCareParam.targetBRatio.push_back(atoi(tokens[i]));
            DF_LOGV("eyecare targetBRatio[%d]:%d",i,mEyeCareParam.targetBRatio[i]);
        }
    }
    key = "EyeCareSatThreshold";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mEyeCareParam.SatThreshold = atof(p);
        DF_LOGV("mEyeCareParam.SatThreshold: %f",atof(p));
    }
    key = "HueLeftBound";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        mEyeCareParam.mHueLeftBound = 30.0;
    } else {
        mEyeCareParam.mHueLeftBound = atof(p);
        DF_LOGV("mEyeCareParam.mHueLeftBound: %f",atof(p));
    }
    key = "HueRightBound";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        mEyeCareParam.mHueRightBound = 60.0;
    } else {
        mEyeCareParam.mHueRightBound = atof(p);
        DF_LOGV("mEyeCareParam.mHueRightBound: %f",atof(p));
    }
    key = "HueCenter";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        mEyeCareParam.mHueCenter = 45.0;
    } else {
        mEyeCareParam.mHueCenter = atof(p);
        DF_LOGV("mEyeCareParam.mHueCenter: %f",atof(p));
    }
    key = "HueTopGain";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        mEyeCareParam.mHueTopGain = 0.5;
    } else {
        mEyeCareParam.mHueTopGain = atof(p);
        DF_LOGV("mEyeCareParam.mHueTopGain: %f",atof(p));
    }
    mapping(White_xy[0], White_xy[1], RGB2XYZ_std);
    mInitDone = 1;
}

void EyeCare::get_inv(double mat[3][3], double inv[3][3])
{
    double adj[3][3] = {};
    double det_mat = mat[0][0] * mat[1][1] * mat[2][2] + mat[0][1] * mat[1][2] * mat[2][0] + mat[0][2] * mat[1][0] * mat[2][1] - mat[2][0] * mat[1][1] * mat[0][2] - mat[0][0] * mat[2][1] * mat[1][2] - mat[1][0] * mat[0][1] * mat[2][2];
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            double det[4] = {};
            int k = 0;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (i != r && j != c) {
                        det[k] = mat[i][j];
                        k++;
                    }
                }
            }
            double det_val = det[0] * det[3] - det[1] * det[2];
            adj[c][r] = pow(-1, r + c) * det_val;
        }
    }

    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            inv[x][y] = adj[x][y] / det_mat;
        }
    }
    return;
};

void EyeCare::mapping(double Wx, double Wy, double RGB2XYZ[3][3])
{
    double w[3] = { Wx / Wy,1.0,(1 - Wx - Wy) / Wy };
    double w1[3][3] = { {0,0,0},{0,0,0} ,{0,0,0} };
    double mat_XYZ_I[3][3] = {};
    get_inv(mat_XYZ, mat_XYZ_I);

    for (int k = 0; k < 3; k++) {
        w1[k][k] = mat_XYZ_I[k][0] * w[0] + mat_XYZ_I[k][1] * w[1] + mat_XYZ_I[k][2] * w[2];
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            RGB2XYZ[i][j] = mat_XYZ[i][j] * w1[j][j];
        }
    }
    return;
}

void EyeCare::xy_gen(int t, double* xy)
{
    double x = 0.0;
    double y = 0.0;
    if (mEyeCareParam.minCCT <= t && t < mEyeCareParam.cctThreshold) {
        x = mEyeCareParam.xCoeff0[0] * pow(10, 9) / pow(t, 3) + mEyeCareParam.xCoeff0[1] * pow(10, 6) / pow(t, 2)
             + mEyeCareParam.xCoeff0[2] * pow(10, 3) / t + mEyeCareParam.xCoeff0[3];
        y = mEyeCareParam.yCoeff0[0] * pow(x, 3) + mEyeCareParam.yCoeff0[1] * pow(x, 2)
             + mEyeCareParam.yCoeff0[2] * x + mEyeCareParam.yCoeff0[3];
    }
    else {
        x = mEyeCareParam.xCoeff1[0] * pow(10, 9) / pow(t, 3) + mEyeCareParam.xCoeff1[1] * pow(10, 6) / pow(t, 2)
             + mEyeCareParam.xCoeff1[2] * pow(10, 3) / t + mEyeCareParam.xCoeff1[3];
        y = mEyeCareParam.yCoeff1[0] * pow(x, 3) + mEyeCareParam.yCoeff1[1] * pow(x, 2)
             + mEyeCareParam.yCoeff1[2] * x + mEyeCareParam.yCoeff1[3];
    }
    xy[0] = x;
    xy[1] = y;
    return;
};

double EyeCare::rgb2bit(double inp)
{
    if (inp < 0) {
        inp = 0;
    }
    return pow(inp / 255.0, 2.2) * 4095;
};

double EyeCare::bit2rgb(double inp)
{
    if (inp < 0) {
        inp = 0;
    }
    if (inp > 4095)
        inp = 4095;
    return pow(inp / 4095.0, 1 / 2.2) * 255;
};

void EyeCare::rgb2hsv(int r, int g, int b, double* hsv)
{
    double rn = r / 4096.0;
    double gn = g / 4096.0;
    double bn = b / 4096.0;
    double mx = max(rn, gn);
    mx = max(mx, bn);
    double mn = min(rn, gn);
    mn = min(mn, bn);
    double m = mx - mn;
    double h = 0.0;
    double s = 0.0;
    double v = 0.0;
    if (mx == mn) {
        h = 0.0;
    } else if (mx == rn) {
        if (gn >= bn) {
            h = (gn - bn) / m * 60;
        } else {
            h = (gn - bn) / m * 60 + 360;
        }
    } else if (mx == gn) {
        h = (bn - rn) / m * 60 + 120;
    } else {
        h = (rn - gn) / m * 60 + 240;
    }

	if (mx == 0.0) {
        s = 0.0;
    } else {
        s = m / mx;
    }
    v = mx;

    hsv[0] = h;
    hsv[1] = s;
    hsv[2] = v;
    return;
};

void EyeCare::hsv2rgb(double h, double s, double v, int* rgb)
{
    double h60 = h / 60.0;
    double h60f = floor(h60);
    int hi = int(h60f) % 6;
    double f = h60 - h60f;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    if (hi == 0) {
        r = v;
        g = t;
        b = p;
    } else if (hi == 1) {
        r = q;
        g = v;
        b = p;
    } else if (hi == 2) {
        r = p;
        g = v;
        b = t;
    } else if (hi == 3) {
        r = p;
        g = q;
        b = v;
    } else if (hi == 4) {
        r = t;
        g = p;
        b = v;
    } else {
        r = v;
        g = p;
        b = q;
    }
    rgb[0] = (int)(r * 4096);
    rgb[1] = (int)(g * 4096);
    rgb[2] = (int)(b * 4096);
    return;
};

double EyeCare::suppress(double tl, double tr, double l, double r, double inp)
{
    return pow((tr - tl)/(r - l),1/2.2) * (inp - l) + tl;
}

double EyeCare::calcGain(double hue)
{
    double gain = 0.0;
    double slope = 0.0;
    double coeff = 0.0;
    double left = mEyeCareParam.mHueLeftBound;
    double right = mEyeCareParam.mHueRightBound;
    double center = mEyeCareParam.mHueCenter;
    double topGain = mEyeCareParam.mHueTopGain;

    if (hue >= left && hue <= right) {
        if (hue <= center) {
            slope = (topGain - 1.0) / (center - left);
        } else {
            slope = (1.0 - topGain) / (right - center);
        }
        coeff = topGain - slope * center;
        gain = slope * hue + coeff;
    } else {
        gain = 1.0;
    }
    DF_LOGV("hue: %f, gain: %f", hue, gain);
    return gain;
}

void EyeCare::gray_trans(int r, int g, int b, int t, int* rgb)
{
    double xy[2] = {};
    double hsv[3] = {};
    double RGB2XYZ[3][3] = {};
    double RGB2XYZ_new[3][3] = {};
    double RGB2RGB[3][3] = {};
    double XYZ2RGB[3][3] = {};
    double gain = 0.0;
    float th = mEyeCareParam.SatThreshold;

    xy_gen(t, xy);
    rgb2hsv(r, g, b, hsv);
    r *= 16;
    g *= 16;
    b *= 16;

    gain = calcGain(hsv[0]);

    if (hsv[1] <= mEyeCareParam.SatThreshold) {
        mapping(xy[0], xy[1], RGB2XYZ);

        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                RGB2XYZ_new[x][y] = RGB2XYZ_std[x][y];
            }
        }
        get_inv(RGB2XYZ_new, XYZ2RGB);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RGB2RGB[i][j] = XYZ2RGB[i][0] * RGB2XYZ[0][j] + XYZ2RGB[i][1] * RGB2XYZ[1][j] + XYZ2RGB[i][2] * RGB2XYZ[2][j];
            }
        }
        for (int k = 0; k < 3; k++) {
            if (k == 0) {
                rgb[k] = RGB2RGB[k][0] * r * (1 - hsv[1] / th * gain) + r * hsv[1] / th * gain;
            } else if (k == 1) {
                rgb[k] = RGB2RGB[k][1] * g * (1 - hsv[1] / th * gain) + g * hsv[1] / th * gain;
            } else {
                rgb[k] = RGB2RGB[k][2] * b * (1 - hsv[1] / th * gain) + b * hsv[1] / th * gain;
            }
        }
    } else {
        rgb[0] = r;
        rgb[1] = g;
        rgb[2] = b;
    }
    return;
};

double EyeCare::getBlueRatio(int r, int g, int b)
{
    return b * mEyeCareParam.regModel[0] + g * mEyeCareParam.regModel[1]
         + r * mEyeCareParam.regModel[2] + mEyeCareParam.regModel[3];
}

int EyeCare::IsGrayLut(int index)
{
    // All gray scale lut indexes are Integer multiple of 307
    if (index % 307)
        return 0;
    else
        return 1;
}

void EyeCare::SetGoogleEffectEnable(int enable)
{
    if (enable != mGoogleEffect) {
        mGoogleEffect = enable;
    }
}


void EyeCare::SetPaperModeId(int id) {
    mPaperModeId = id;
}

void EyeCare::SetLogLevel(int level)
{
    DFLOG::loglevel = level;
}

void EyeCare::SetEyeCare(int value, int cookie, struct rgb_entry *lutInfo) {
    int node_max = 0;
    int rgb_nodes_new[LUT3D_ENTRIES_SIZE][3] = {};
    int rgb_n[3] = {};
    double hsv[3] = {};
    int rgb[3] = {};
    int R,G,B;
    std::vector<rgb_entry> tempLut;
    rgb_entry temp;
    int cct = 0;
    if (!lutInfo)
        return;
    if (value <= mEyeCareParam.startLevel)
        cct = mEyeCareParam.maxCCT;
    else
        cct = mEyeCareParam.maxCCT - (mEyeCareParam.maxCCT - mEyeCareParam.minCCT) *
             (value - mEyeCareParam.startLevel) / (mEyeCareParam.maxLevel - mEyeCareParam.startLevel);

    if (!mInitDone) {
        DF_LOGE("EyeCare param not initialized!");
        return;
    }

    for (int i = 0; i < LUT3D_ENTRIES_SIZE; i++) {
        if (mGoogleEffect && IsGrayLut(i)) {
            rgb_nodes_new[i][0] = lutInfo[i].out.r;
            rgb_nodes_new[i][1] = lutInfo[i].out.g;
            rgb_nodes_new[i][2] = lutInfo[i].out.b;
            continue;
        }

        int r_val = lutInfo[i].out.r;
        int g_val = lutInfo[i].out.g;
        int b_val = lutInfo[i].out.b;
        rgb2hsv(r_val, g_val, b_val, hsv);
        double h_val = hsv[0];
        double s_val = hsv[1];
        double v_val = hsv[2];

        double blue = getBlueRatio(r_val, g_val, b_val);
        if (blue > mEyeCareParam.BlueThreshold) {
            double threshold = suppress(mEyeCareParam.targetBRatio[0], mEyeCareParam.targetBRatio[1],
                               mEyeCareParam.srcBRatio[0], mEyeCareParam.srcBRatio[1], blue);
            int cnt = 0;
            while (getBlueRatio(r_val, g_val, b_val) > threshold) {
                cnt++;
                if (cnt > MAX_LOOP_CNT) {
                    DF_LOGE("get max loop cnt, break, lut line id %d", i);
                    break;
                }
                s_val -= mEyeCareParam.step;
                v_val -= 3 * mEyeCareParam.step;
                hsv2rgb(h_val, s_val, v_val, rgb);
                r_val = rgb[0];
                g_val = rgb[1];
                b_val = rgb[2];
            }
        }
        gray_trans(r_val, g_val, b_val, cct, rgb_n);
        node_max = max(node_max, rgb_n[0]);
        node_max = max(node_max, rgb_n[1]);
        node_max = max(node_max, rgb_n[2]);
        rgb_nodes_new[i][0] = rgb_n[0];
        rgb_nodes_new[i][1] = rgb_n[1];
        rgb_nodes_new[i][2] = rgb_n[2];
    }
    for (int i = 0; i < LUT3D_ENTRIES_SIZE; i++) {
        if (value < mEyeCareParam.startLevel) {
            if (mGoogleEffect && IsGrayLut(i)) {
                temp.out.r = (int)rgb_nodes_new[i][0];
                temp.out.g = (int)rgb_nodes_new[i][1];
                temp.out.b = (int)rgb_nodes_new[i][2];
            } else {
                temp.out.r = (int)((double)rgb_nodes_new[i][0] / (double)node_max * 4096);
                temp.out.g = (int)((double)rgb_nodes_new[i][1] / (double)node_max * 4096);
                temp.out.b = (int)((double)rgb_nodes_new[i][2] / (double)node_max * 4096);
            }
            lutInfo[i].out.r = lutInfo[i].out.r + (int)(value * (temp.out.r - lutInfo[i].out.r)) / mEyeCareParam.startLevel;
            lutInfo[i].out.g = lutInfo[i].out.g + (int)(value * (temp.out.g - lutInfo[i].out.g)) / mEyeCareParam.startLevel;
            lutInfo[i].out.b = lutInfo[i].out.b + (int)(value * (temp.out.b - lutInfo[i].out.b)) / mEyeCareParam.startLevel;
        } else {
            if (mGoogleEffect && IsGrayLut(i)) {
                lutInfo[i].out.r = (int)rgb_nodes_new[i][0];
                lutInfo[i].out.g = (int)rgb_nodes_new[i][1];
                lutInfo[i].out.b = (int)rgb_nodes_new[i][2];
            } else {
                lutInfo[i].out.r = (int)((double)rgb_nodes_new[i][0] / (double)node_max * 4096);
                lutInfo[i].out.g = (int)((double)rgb_nodes_new[i][1] / (double)node_max * 4096);
                lutInfo[i].out.b = (int)((double)rgb_nodes_new[i][2] / (double)node_max * 4096);
            }
        } 
    }
}

} // namespace android


