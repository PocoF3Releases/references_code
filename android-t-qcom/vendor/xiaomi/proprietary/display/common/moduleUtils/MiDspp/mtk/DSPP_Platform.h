#ifndef _DSPP_PLATFORM_H_
#define _DSPP_PLATFORM_H_
#include "../common/DSPP_Common.h"
#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include "parseXml.h"
#include "DisplayFeatureHal.h"
using ::android::hidl::allocator::V1_0::IAllocator;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hardware::hidl_memory;

#define ARRAY_SIZE 3
#define DISP_GAMMA_INDEX_DEFAULT 220

#define DISP_GAMMA        15
#define DISP_C3D          16
#define MTK_C3D_SIZE      17
#define LUT3D_ENTRIES_SIZE (MTK_C3D_SIZE * MTK_C3D_SIZE * MTK_C3D_SIZE)
#define CALIB_FILE_NUM    4

namespace android {

class DSPP_Platform : public DSPP_Common
{
public:
    DSPP_Platform(unsigned int displayId);
    virtual ~DSPP_Platform() {};
    int setPccConfig(double *rgbCoeff, int bl = -1);
    int setDREFunction(int value);
    int setESSFunction(int value);
    int setAALBypass(int value);
    int setDCStatus(int value);
    int setPQMode(int value);
    int setHSVC(int type, int value);
    int setRGB(uint32_t r_gain, uint32_t g_gain, uint32_t b_gain, int32_t step);
    int setGammaIndex(int value);
    int setGamePQMode(int value);
    int setC3dLut(int lutId, int state);
    int readC3dLutData(const char* const path, int lutId);
    int initC3dLut(uint8_t disp_id, bool reset);
    int getAshmem(hidl_memory& memory, uint32_t size);
    int getGamutTable(int lutId, hidl_memory& mem_c3d);
    int setColorMode(int mode, int cookie, enum DISPLAY_PP_MODE * renderIntent);
    int setGlobalPA(int caseId, int value, int dimming, GlobalPAParam param);
    int setGammaCorrection(int targetGamma, int type, struct DispPPParam *pSDPPParams);
    int setLut(int lutId, int cookie, int dimming);
    int DisablePA();
    int DisableGC();
    int DisableLut();

    enum {
        MTK_PQ_DRE_ESS_OFF = 0x0,
        MTK_PQ_ESS_ON = 0x2,
        MTK_PQ_DRE_ON = 0x4,
        MTK_PQ_ESS_DRE_ON = 0x6,
    };

    enum DISP_ESS_STRENGTH {
        ESS_OFF_STRENGTH = 0,
        ESS_UI_STRENGTH = 30,
        ESS_MOVIE_STRENGTH = 90,
        ESS_STILL_STRENGTH = 180,
    };
private:
    int mPQState = 0;
    int mCcorrBaseCoef;
    int mGammaCoef;
    int mSupportC3d;
    bool mSupportCABC;
    int mColorStep;
    std::vector<std::vector<rgb_entry>> mLutInfo;
    sp<MiParseXml> mParseXml[DISPLAY_MAX];
};
} //namespace android
#endif
