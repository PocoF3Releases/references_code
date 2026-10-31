#include "paperMode.h"
#include "DisplayEffectBase.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "PaperMode"
#endif
#ifndef MTK_PLATFORM
using snapdragoncolor::IMiStcService;
#endif
using namespace std;
namespace android {
int DFLOG::loglevel = 1;
PaperMode::PaperMode(const sp<DisplayEffectBase>& display_effect)
    : mDisplayEffect(display_effect) {
    if (mDisplayEffect.get()) {
        displayId = mDisplayEffect->mDisplayId;
    }
    mParseXml[displayId] = new MiParseXml();
    if (mParseXml[displayId].get()) {
        init();
    }
}

PaperMode::~PaperMode()
{

}

void PaperMode::init()
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
        mParam.minCCT = atoi(p);
        DF_LOGV("mParam.minCCT: %d",atoi(p));
    }
    key = "EyeCareMaxCCT";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mParam.maxCCT = atoi(p);
        DF_LOGV("mParam.maxCCT: %d",atoi(p));
    }
    key = "EyeCareStartLevel";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mParam.startLevel = atoi(p);
        DF_LOGV("mParam.startLevel: %d",atoi(p));
    }
    key = "EyeCareMaxLevel";
    ret = mParseXml[displayId]->parseXml(displayId, key, p, sizeof(p));
    if(ret != 0) {
        DF_LOGE("parse %s failed", key.c_str());
        return;
    } else {
        mParam.maxLevel = atoi(p);
        DF_LOGV("mParam.maxLevel: %d",atoi(p));
    }
    for (int i = 0; i < PAPER_COLOR_NUM; i++) {
        key = "paperCoeff";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < PAPER_PCC_COEFF_NUM; j++) {
                mParam.PaperCoeff[i][j] = atof(tokens[j]);
                DF_LOGV("paper coeff %d %d %f ",i, j, atof(tokens[j]));
            }
        }
    }
    for (int i = 0; i < EYECARE_PCC_COEFF_NUM; i++) {
        key = "eyeCareCoeff";
        sprintf(temp, "%d", i);
        key.append(temp);
        ret = mParseXml[displayId]->parseXml(displayId, key,p,tokens,&count);
        if(ret != 0) {
            DF_LOGE("parse %s failed", key.c_str());
            return;
        } else {
            for (int j = 0; j < EYECARE_POWER_NUM; j++) {
                mParam.EyeCareCoeff[i][j] = atof(tokens[j]);
                DF_LOGV("paper eyecare coeff %d %d %f ",i, j, atof(tokens[j]));
            }
        }
    }
    mInitDone = 1;
}

int PaperMode::setPaperMode(int modeId)
{
    int ret = 0;
    mPaperId = modeId;
    memcpy(mPaperCoeff, mParam.PaperCoeff[modeId], sizeof(mPaperCoeff));
    if (mEyeCareLevel != 0) {
        ret = setEyeCare(mEyeCareLevel);
    }
    if(!mDisplayEffect->GetVideoModeId()) {
        ret = setPCCConfigPaperMode();
    }


    return ret;
}

int PaperMode::clearPaperMode()
{
    int ret = 0;
    memcpy(mPaperCoeff, mParam.PaperCoeff[0], sizeof(mPaperCoeff));
    if(!mDisplayEffect->GetVideoModeId()) {
        ret = setPCCConfigPaperMode();
    }

    return ret;
}

int PaperMode::setEyeCare(int modeId)
{
    int i;
    int ret = 0;
    double temp = 0.0;
    double step = (mParam.maxCCT - mParam.minCCT) / (mParam.maxLevel - mParam.startLevel);
    double cct = mParam.maxCCT - step * (modeId - mParam.startLevel);
    double dimmingStep[ARRAY_SIZE_EXP] = {0};
    int maxCCT = mParam.maxCCT;

    DF_LOGV("cct %f, value %d", cct, modeId);
    if (mPaperId) {
        for (i = 0; i < ARRAY_SIZE_EXP; i++) {
            dimmingStep[i] = mParam.EyeCareCoeff[i][0] * maxCCT * maxCCT * maxCCT \
                         + mParam.EyeCareCoeff[i][1] * maxCCT * maxCCT \
                         + mParam.EyeCareCoeff[i][2] * maxCCT \
                         + mParam.EyeCareCoeff[i][3];
            if (i % 4 == 0)
                dimmingStep[i] = (1.0 - dimmingStep[i])/mParam.startLevel;
            else
                dimmingStep[i] = dimmingStep[i]/mParam.startLevel;
        }
        if (modeId <= mParam.startLevel) {
            for (i = 0; i < ARRAY_SIZE_EXP; i++) {
                if (i % 4 == 0)
                    mEyeCareCoeff[i] = 1.0 - dimmingStep[i] * modeId;
                else
                    mEyeCareCoeff[i] = dimmingStep[i] * modeId;
            }
        } else {
            for (i = 0; i < ARRAY_SIZE_EXP; i++) {
                mEyeCareCoeff[i] = mParam.EyeCareCoeff[i][0] * cct * cct * cct \
                         + mParam.EyeCareCoeff[i][1] * cct * cct \
                         + mParam.EyeCareCoeff[i][2] * cct \
                         + mParam.EyeCareCoeff[i][3];
            }
        }
        ret = setPCCCoeffsToHWC();
    }

    mEyeCareLevel = modeId;

    return ret;
}

int PaperMode::getPaperId()
{
    return mPaperId;
}

int PaperMode::setPCCCoeffsToHWC()
{
    int ret = 0;
#ifndef MTK_PLATFORM
    int enable = 1;
    sp<IMiStcService> imistcservice;
    android::Parcel data;
    double coeff[INDEX_SIZE] = {1.0, 0.0, 0.0, 0.0,
                                0.0, 1.0, 0.0, 0.0,
                                0.0, 0.0, 1.0, 0.0,
                                0.0, 0.0, 0.0, 1.0};
    data.writeInt32(1);
    data.writeInt32(displayId);
    //multiply 3x3 and 4x4 matrix here. Now we only have 3x3 matrix, just put it in coeff.
    for (int i = 0; i < MATRIX_SIZE; i++)
        for (int j = 0; j < MATRIX_SIZE; j++) {
            if ((j != MATRIX_SIZE - 1) && (i < MATRIX_SIZE - 1))
                coeff[i * MATRIX_SIZE + j] = mEyeCareCoeff[i * (MATRIX_SIZE - 1) + j];
            data.writeDouble(coeff[i * MATRIX_SIZE + j]);
        }

    imistcservice = interface_cast<IMiStcService>(defaultServiceManager()->getService(String16("display.mistcservice")));
    if (imistcservice.get()) {
        ret = imistcservice->dispatch(IMiStcService::SET_EYECARE_PCC, &data, NULL);
    } else {
        DF_LOGW("Failed to acquire %s", "display.qservice");
        ret = -1;
    }
#endif
    return ret;
}

int PaperMode::setPCCConfigPaperMode()
{
    int i = 0, j = 0, ret = 0;
    struct OutputParam sParams;
    double coeff[ARRAY_SIZE_EXP] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};

    sParams.function_id = DISPLAY_PAPER_MODE;
    // Paper mode x Sunlight screen matrix
    for (i = 0; i < ARRAY_SIZE; i++)
        for (j = 0; j < ARRAY_SIZE; j++) {
            sParams.pcc_payload.push_back(mPaperCoeff[3 * i + j]);
        }

    for (i = 0; i < ARRAY_SIZE_EXP; i++) {
        DF_LOGV("paper coeff%d %f, eyecare Coeff%d %f",i, mPaperCoeff[i], mEyeCareCoeff[i]);
    }
    ret = mDisplayEffect->setDisplayEffect(SET_PCC_MODE, &sParams);
    return ret;
}

void PaperMode::dump(std::string& result)
{
    char res_ch[1024] = {0};
    result.append("\nPaper:\n");
    snprintf(res_ch, sizeof(res_ch), "Disp ID: %d, Mode: %d, value: %d\n",
                             displayId, mPaperId, mEyeCareLevel);
    result.append(std::string(res_ch));
    snprintf(res_ch, sizeof(res_ch), "EC Coeff[%f %f %f %f %f %f %f %f %f]\n",
             mEyeCareCoeff[0], mEyeCareCoeff[1], mEyeCareCoeff[2], mEyeCareCoeff[3], mEyeCareCoeff[4],
             mEyeCareCoeff[5], mEyeCareCoeff[6], mEyeCareCoeff[7], mEyeCareCoeff[8]);
    result.append(std::string(res_ch));
    snprintf(res_ch, sizeof(res_ch), "PP Coeff[%f %f %f %f %f %f %f %f %f]\n",
             mPaperCoeff[0], mPaperCoeff[1], mPaperCoeff[2], mPaperCoeff[3], mPaperCoeff[4],
             mPaperCoeff[5], mPaperCoeff[6], mPaperCoeff[7], mPaperCoeff[8]);
    result.append(std::string(res_ch));
}

} // namespace android
