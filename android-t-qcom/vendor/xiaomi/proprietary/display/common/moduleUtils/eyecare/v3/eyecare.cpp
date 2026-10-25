#include "eyecare.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "EyeCare"
#endif
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

    string key("maxCCT");
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mParam.maxCCT = atoi(p);
        DF_LOGV("maxCCT: %d",atoi(p));
    }

    key = "minCCT";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mParam.minCCT = atoi(p);
        DF_LOGV("minCCT: %d",atoi(p));
    }

    for (int i = 0; i < 9; i++) {
        key = "eyeCareCoeff";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < 4; j++) {
                mParam.Coeffs[i][j] = atof(tokens[j]);
                DF_LOGV("Coeffs %d %d %f ",i, j, atof(tokens[j]));
            }
        }
    }
    mInit = true;
}

int EyeCare::getEyeCareCoeff(int level, double* coeff)
{
    int i;
    int ret = 0;
    double temp = 0.0;
    double step = (mParam.maxCCT - mParam.minCCT) / (255 - EYECARE_START_LEVEL);
    double cct = mParam.maxCCT - step * (level - EYECARE_START_LEVEL);
    double dimmingStep[ARRAY_SIZE_EXP] = {0};
    int maxCCT = mParam.maxCCT;

    DF_LOGV("cct %f, value %d", cct, level);

    if (!mInit) {
        DF_LOGE("EyeCare does not init, return");
        return -1;
    }
    for (i = 0; i < ARRAY_SIZE_EXP; i++) {
        dimmingStep[i] = mParam.Coeffs[i][0] * maxCCT * maxCCT * maxCCT \
                     + mParam.Coeffs[i][1] * maxCCT * maxCCT \
                     + mParam.Coeffs[i][2] * maxCCT \
                     + mParam.Coeffs[i][3];
        if (i%4 == 0)
            dimmingStep[i] = (1.0 - dimmingStep[i])/EYECARE_START_LEVEL;
        else
            dimmingStep[i] = dimmingStep[i]/EYECARE_START_LEVEL;
    }
    if (level <= EYECARE_START_LEVEL) {
        for (i = 0; i < ARRAY_SIZE_EXP; i++) {
            if (i%4 == 0)
                coeff[i] = 1.0 - dimmingStep[i] * level;
            else
                coeff[i] = dimmingStep[i] * level;
        }
    } else {
        for (i = 0; i < ARRAY_SIZE_EXP; i++) {
            coeff[i] = mParam.Coeffs[i][0] * cct * cct * cct \
                     + mParam.Coeffs[i][1] * cct * cct \
                     + mParam.Coeffs[i][2] * cct \
                     + mParam.Coeffs[i][3];
        }
    }

    return ret;
}
} // namespace android
