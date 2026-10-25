#include "DSPP_Platform.h"
#include "MtkPQServiceGet.h"
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "DisplayFeatureHal"
#endif

using ::vendor::mediatek::hardware::pq::V2_0::Result;
using ::vendor::mediatek::hardware::pq::V2_0::PQFeatureID;

namespace android {

sp<IPictureQuality> getPQService()
{
    static sp<MTKPQServiceGet> spMTKPQService = new MTKPQServiceGet();
    sp<IPictureQuality> pqClient;

    if (spMTKPQService.get()) {
       //ALOGV("%s,getPQService", __func__);
       pqClient = spMTKPQService->getMTKPQService();
    }

    return pqClient;
}

DSPP_Platform::DSPP_Platform(unsigned int displayId)
        : DSPP_Common(displayId)
{
    mSupportC3d = property_get_int32("ro.vendor.pq.mtk_disp_c3d_support", 0);
    mSupportCABC = property_get_bool("ro.vendor.cabc.enable", false);
    mParseXml[mDisplayId] = new MiParseXml();
    if (mParseXml[mDisplayId].get()) {
        int ret = 0;
        char p[4096];
        string key("CcorrBaseCoef");
        ret = mParseXml[mDisplayId]->parseXml(mDisplayId, key, p, sizeof(p));
        if(ret != 0) {
            ALOGE("parse %s failed", key.c_str());
            mCcorrBaseCoef = 1024;
        } else {
            mCcorrBaseCoef = atoi(p);
            ALOGV("mCcorrBaseCoef: %d",atoi(p));
        }
        key = ("GammaCoef");
        ret = mParseXml[mDisplayId]->parseXml(mDisplayId, key, p, sizeof(p));
        if(ret != 0) {
            ALOGE("parse %s failed", key.c_str());
            mGammaCoef = 5;
        } else {
            mGammaCoef = atoi(p);
            ALOGV("mGammaCoef: %d",atoi(p));
        }
        key = ("ColorStep");
        ret = mParseXml[mDisplayId]->parseXml(mDisplayId, key, p, sizeof(p));
        if(ret != 0) {
            ALOGE("parse %s failed", key.c_str());
            mColorStep = 1;
        } else {
            mColorStep = atoi(p);
            ALOGV("mColorStep: %d",atoi(p));
        }

    }
}

int DSPP_Platform::setPccConfig(double *rgbCoeff, int bl)
{
    int ret = 0, i, j;
    hidl_array<int32_t, 3, 3> colormatrix3_3;
    int step = mColorStep;
    if (bl != -1) {
        step = (bl << 16 | 1);
    }
    for (i = 0; i < ARRAY_SIZE; i++) {
        for (j = 0; j < ARRAY_SIZE; j++) {
          colormatrix3_3[i][j] = (int)(rgbCoeff[3 * i + j] * mCcorrBaseCoef + 0.5);
        }
    }
    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        ALOGV("%s,setColorMatrix3x3", __func__);
        android::hardware::Return<Result> result =
            pqClient->setColorMatrix3x3(colormatrix3_3, step); /*1- for 1 frame step */
        if (!result.isOk() || result != Result::OK) {
            ALOGE("%s: setColorMatrix3x3 failed\n", __FUNCTION__);
            ret = -1;
        }
    }
    return ret;
}

int DSPP_Platform::setDREFunction(int value)
{
    int lux = value >> 16;
    bool dre_enable = (bool)(value & 0xFFFF);
    int ret = 0;
    int DREValue = 0;
    ALOGV("%s, lux:%d, dre_enable:%d", __FUNCTION__, lux, dre_enable);

    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        /*mPQState
        *  0x0 - DRE, ESS(ACBC) off
        *  0x2 - ESS on
        *  0x4 - DRE on
        *  0x6 - DRE, ESS on
        */
        mPQState =( (mPQState & (~MTK_PQ_DRE_ON)) |(dre_enable << 2));
        DREValue = mPQState | (lux << 16);

        android::hardware::Return<Result> result = pqClient->setFunction(DREValue, false);
        if (!result.isOk() || result != Result::OK) {
            ALOGE("%s: setFunction failed\n", __FUNCTION__);
            ret = -1;
        }
    }
    return ret;
}

int DSPP_Platform::setESSFunction(int value)
{
    int dre_enable = (value > 0 ? 1 : 0);
    int ess_strenth = 0;
    int ret = 0;

    ALOGV("%s, value:%d, mPQState:0x%x", __FUNCTION__, value, mPQState);

    if (!mSupportCABC) {
        ALOGW("%s, CABC is not support", __FUNCTION__);
        return 0;
    }

    if (value > CABC_STILL_ON || value < CABC_OFF) {
        ALOGW("%s, parameter is invalid", __FUNCTION__);
        return 0;
    }

    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        /*mPQState
        *  0x0 - DRE, ESS(ACBC) off
        *  0x2 - ESS on
        *  0x4 - DRE on
        *  0x6 - DRE, ESS on
        */
        mPQState = (mPQState & (~MTK_PQ_ESS_ON)) |(dre_enable << 1);
        android::hardware::Return<Result> result = pqClient->setFunction(mPQState, false);
        if (!result.isOk() || result != Result::OK) {
            ALOGE("%s: setFunction failed\n", __FUNCTION__);
            ret = -1;
        }

        if (value >= 0) {
            switch (value) {
                case CABC_UI_ON:
                    ess_strenth = ESS_UI_STRENGTH;
                    break;
                case CABC_MOVIE_ON:
                    ess_strenth = ESS_MOVIE_STRENGTH;
                    break;
                case CABC_STILL_ON:
                    ess_strenth = ESS_STILL_STRENGTH;
                    break;
                case CABC_OFF:
                    ess_strenth = ESS_OFF_STRENGTH;
                    break;
                default:
                    ess_strenth = ESS_UI_STRENGTH;
            }

            android::hardware::Return<Result> result = pqClient->setSmartBacklightStrength(ess_strenth);
            if (!result.isOk() || result != Result::OK) {
                ALOGE("%s: setSmartBacklightStrength failed\n", __FUNCTION__);
                ret = -1;
            }
        }
    }
    return ret;
}

int DSPP_Platform::setAALBypass(int value)
{
    bool dre_enable = (bool)value;
    int bypass_aal_flag = 0;
    int ret = 0;

    ALOGE("%s, value:%d in", __FUNCTION__, value);

    switch (value) {
        case MI_AAL_BYPASS:
            bypass_aal_flag = MI_AAL_BYPASS_FLAG;
            break;
        case MI_AAL_RESTORE:
            bypass_aal_flag = MI_AAL_RESTORE_FLAG;
            break;
        default:
            bypass_aal_flag = MI_AAL_RESTORE_FLAG;
    }

    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        /* bypass_aal_flag
        *  MI_AAL_BYPASS_FLAG = 0x110000
        *  MI_AAL_RESTORE_FLAG = 0x100000
        */
        android::hardware::Return<Result> result = pqClient->setFunction(bypass_aal_flag, false);
        if (!result.isOk() || result != Result::OK) {
            ALOGE("%s: setTuningField failed\n", __FUNCTION__);
            ret = -1;
        }

    }
    return ret;
}

int DSPP_Platform::setDCStatus(int value)
{
    int ret = 0;

    ALOGV("%s, value:%d", __FUNCTION__, value);
    if (value) {
        value = 2;
    }
    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        pqClient->setTuningField(0x0184, 0x660, value);
    }
    return ret;
}

int DSPP_Platform::setPQMode(int value)
{
    int ret = 0;
    int step = (mColorStep | 1 << 16);

    if ((value < DISP_PQ_DEFAULT || value > DISP_PQ_GAME_3)
        && (value < DISP_PQ_MODE_MOVIE_1 || value > DISP_PQ_MODE_MOVIE_11)) {
        ALOGW("%s, invalid parameter!", __func__);
        return ret;
    }

    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        ALOGD("%s: setPQMode, value = %d\n", __FUNCTION__, value);
        android::hardware::Return<Result> result = pqClient->setPQMode(value, step);
        if (!result.isOk() || result != Result::OK) {
            ALOGE("%s: setPQMode failed\n", __FUNCTION__);
            ret = -1;
        }
    }
    return ret;
}

int DSPP_Platform::setHSVC(int type, int value)
{
    int ret = 0;
    int index;

    switch (type) {
        case EXPERT_H: {
            //value += 180;
            break;
        }
        case EXPERT_S: {
            index = DISP_S_VALUE;
            value += 50;
            value /= 1.25;
            value += 10;
            break;
        }
        case EXPERT_V: {
            index = DISP_V_VALUE;
            value += 255;
            value /= 1.6;
            value += 96;
            break;
        }
        case EXPERT_CONTRAST: {
            index = DISP_CONTRAST_VALUE;
            value += 100;
            value /= 3;
            value += 17;
            break;
        }
        default:
            return ret;
            break;
    }
    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        ALOGW("%s, value(%d) index(%d) !", __func__, value, index);
        if (DISP_H == type) {
            android::hardware::Return<Result> result = pqClient->setHueOffset(value, mColorStep);
            if (!result.isOk() || result != Result::OK) {
                ALOGE("%s: setHueOffset failed\n", __FUNCTION__);
                ret = -1;
            }
        } else {
            android::hardware::Return<Result> result = pqClient->setPQIndex(value, 0, 0, index, mColorStep);
            if (!result.isOk() || result != Result::OK) {
                ALOGE("%s: setPQIndex failed\n", __FUNCTION__);
                ret = -1;
            }
        }
    }
    return ret;
}

int DSPP_Platform::setRGB(uint32_t r_gain, uint32_t g_gain, uint32_t b_gain, int32_t step)
{
    int ret = -1;

    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        ALOGW("%s, param_R(%d) param_G(%d) param_B(%d) !", __func__, r_gain, g_gain, b_gain);
        android::hardware::Return<Result> result = pqClient->setRGBGain(r_gain, g_gain, b_gain, step);
        if (!result.isOk() || result != Result::OK) {
            ALOGE("%s: setRGBGain failed\n", __FUNCTION__);
            ret = -1;
        }
    }

    return 0;
}

int DSPP_Platform::setGammaIndex(int value)
{
    int ret = 0;
    value = 270 - value;
    value /= mGammaCoef;

    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        ALOGW("%s, %d", __func__, value);
        android::hardware::Return<Result> result = pqClient->setGammaIndex(value, 0);
        if (!result.isOk() || result != Result::OK) {
            ALOGE("%s: setGammaIndex failed\n", __FUNCTION__);
            ret = -1;
        }
    }
    return ret;
}

int DSPP_Platform::setGamePQMode(int value)
{
    int ret = 0;
    android::hardware::Return<Result> result = Result::OK;

    if (value < DISP_PQ_DEFAULT || value > DISP_PQ_GAME_3) {
        ALOGW("%s, invalid parameter!", __func__);
        return -1;
    }

    sp<IPictureQuality> pqClient = getPQService();
    if (pqClient.get()) {
        if (value < DISP_PQ_GAME_1) {
            result = pqClient->setDisplayGamePQ(1, -1, 0);
            if (!result.isOk() || result != Result::OK) {
                ALOGE("%s: setDisplayGamePQ failed\n", __FUNCTION__);
                ret = -1;
            }
            result = pqClient->setDisplayGamePQ(2, -1, 0);
            if (!result.isOk() || result != Result::OK) {
                ALOGE("%s: setDisplayGamePQ failed\n", __FUNCTION__);
                ret = -1;
            }
        }
        else {
            if (DISP_PQ_GAME_1 == value) {
                result = pqClient->setDisplayGamePQ(1, 0, 2);
                if (!result.isOk() || result != Result::OK) {
                    ALOGE("%s: setDisplayGamePQ failed\n", __FUNCTION__);
                    ret = -1;
                }
                result = pqClient->setDisplayGamePQ(2, -1, 0);
                if (!result.isOk() || result != Result::OK) {
                    ALOGE("%s: setDisplayGamePQ failed\n", __FUNCTION__);
                    ret = -1;
                }
            }else if (DISP_PQ_GAME_2 == value) {
                result = pqClient->setDisplayGamePQ(1, 1, 0);
                if (!result.isOk() || result != Result::OK) {
                    ALOGE("%s: setDisplayGamePQ failed\n", __FUNCTION__);
                    ret = -1;
                }
                result = pqClient->setDisplayGamePQ(2, 1, 2);
                if (!result.isOk() || result != Result::OK) {
                    ALOGE("%s: setDisplayGamePQ failed\n", __FUNCTION__);
                    ret = -1;
                }
            }else if (DISP_PQ_GAME_3 == value) {
                result = pqClient->setDisplayGamePQ(1, 2, 2);
                if (!result.isOk() || result != Result::OK) {
                    ALOGE("%s: setDisplayGamePQ failed\n", __FUNCTION__);
                    ret = -1;
                }
                result = pqClient->setDisplayGamePQ(2, 2, 2);
                if (!result.isOk() || result != Result::OK) {
                    ALOGE("%s: setDisplayGamePQ failed\n", __FUNCTION__);
                    ret = -1;
                }
            }
        }
    }
    return ret;
}

int DSPP_Platform::setC3dLut(int lutId, int state)
{
    int ret = 0;
    const uint8_t displayId = 0;
    static bool c3d_enabled = false;

    ALOGD("%s: state = %d, lutId = %d", __FUNCTION__, state, lutId);

    if (!mSupportC3d) {
        ALOGW("%s, C3d is not support", __FUNCTION__);
        return 0;
    }

    sp<IPictureQuality> pqClient = getPQService();
    if (!pqClient.get()) {
        return -1;
    }

    if (mLutInfo.empty() ||
            (lutId >= 0 && lutId < mLutInfo.size() && mLutInfo[lutId].empty()) ||
            state == C3D_LUT_RESET) {
        if (initC3dLut(displayId, state == C3D_LUT_RESET) < 0) {
            ALOGE("%s: failed to init c3d lut\n", __FUNCTION__);
            return -1;
        }
    }

    switch (state) {
        case C3D_LUT_OFF: {
            ALOGD("%s: C3D_LUT_OFF\n", __FUNCTION__);
            if (c3d_enabled) {
                android::hardware::Return<::vendor::mediatek::hardware::pq::V2_0::Result> result =
                    pqClient->setFeatureSwitch((PQFeatureID)DISP_C3D, 0);
                if (!result.isOk() || result != ::vendor::mediatek::hardware::pq::V2_0::Result::OK) {
                    ALOGE("%s: setFeatureSwitch failed\n", __FUNCTION__);
                    ret = -1;
                }
                c3d_enabled = false;
            }
        } break;
        case C3D_LUT_RESET: {
            ALOGD("%s: C3D_LUT_RESET, lutId = %d\n", __FUNCTION__, lutId);
        }
        [[fallthrough]];
        case C3D_LUT_SET: {
            ALOGD("%s: C3D_LUT_SET, lutId = %d\n", __FUNCTION__, lutId);
            if (lutId >= 0 && lutId < mLutInfo.size()) {
                hidl_memory mem_c3d = hidl_memory();
                if (!getAshmem(mem_c3d, LUT3D_ENTRIES_SIZE * 3 * sizeof(uint32_t))) {
                    getGamutTable(lutId, mem_c3d);
                    android::hardware::Return<::vendor::mediatek::hardware::pq::V2_0::Result> result =
                        pqClient->setDispPQBufHandle(displayId, mem_c3d.handle(), MTK_C3D_SIZE, NULL, 0);
                    if (!result.isOk() || result != ::vendor::mediatek::hardware::pq::V2_0::Result::OK) {
                        ALOGE("%s: setDispPQBufHandle() failed\n", __FUNCTION__);
                        ret = -1;
                    } else if (!c3d_enabled) {
                        android::hardware::Return<::vendor::mediatek::hardware::pq::V2_0::Result> result =
                            pqClient->setFeatureSwitch((PQFeatureID)DISP_C3D, 1);
                        if (!result.isOk() || result != ::vendor::mediatek::hardware::pq::V2_0::Result::OK) {
                            ALOGE("%s: setFeatureSwitch failed\n", __FUNCTION__);
                            ret = -1;
                        }
                        c3d_enabled = true;
                    }
                } else {
                    ALOGE("%s: failed to get ashmem\n", __FUNCTION__);
                    ret = -1;
                }
            } else {
                ALOGE("%s: lutId(%d) is out of range [0, %d]\n",
                        __FUNCTION__, lutId, mLutInfo.size() - 1);
                ret = -1;
            }
        } break;
        case C3D_LUT_DUMP: {
            ALOGD("%s: C3D_LUT_DUMP, lutId = %d\n", __FUNCTION__, lutId);
            if (lutId >= 0 && lutId < mLutInfo.size()) {
                const std::vector<rgb_entry> &LutInfo = mLutInfo[lutId];
                for (int i = 0; i < LutInfo.size(); ++i) {
                    const struct rgb_entry &rgb = LutInfo.at(i);
                    ALOGD("[c3d][%d]%u %u %u %u %u %u", i,
                            rgb.in.r, rgb.in.g, rgb.in.b, rgb.out.r, rgb.out.b, rgb.out.b);
                }
            } else {
                ALOGE("%s: lutId(%d) is out of range [0, %d]\n", __FUNCTION__, lutId, mLutInfo.size());
            }
        } break;
        default:
            break;
    }
    return 0;
}

int DSPP_Platform::initC3dLut(uint8_t disp_id, bool reset)
{
    struct rgb_entry temp;
    char path[256];
    std::vector<struct rgb_entry> rgb;
    FILE *fp = NULL;
    int lutFileNum;
    const char *kDISP_WP_PATH = "/sys/class/mi_display/disp-DSI-0/wp_info";
    const char *kDISP_ID_PATH = "/mnt/vendor/persist/display/screenColorCal_panelInfo.txt";
    const char *kDISP_CALIBRATION_STAUS = "vendor.display.panel.calibration.status";
    int wpInfo = -1;
    int displayId = -2;
    int calibLutNum = 0;
    enum {
        CALIB_STATUS_PASS = 0,          // Both lut file and id was valid
        CALIB_STATUS_INVALID_LUT = 1,   // Cant' read wp_info or calib files
        CALIB_STATUS_INVALID_ID = 2,    // Display id dosen't match wp_info
    } calibStatus = CALIB_STATUS_INVALID_LUT;

    if (!mLutInfo.empty()) {
        mLutInfo.clear();
    }

    /* parse lut file number */
    sprintf(path, "/vendor/etc/miLutInfo.txt");
    if (access(path, F_OK|R_OK) < 0) {
        ALOGE("%s: can not access file %s", __FUNCTION__, path);
        return -1;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        ALOGE("%s: open file error %s", __FUNCTION__, path);
        return -1;
    }
    fscanf(fp, "%d", &lutFileNum);
    fclose(fp);

    if (lutFileNum < CALIB_FILE_NUM) {
        ALOGE("%s: lutFileNum(%d) must bigger than %d\n",
                __FUNCTION__, lutFileNum, CALIB_FILE_NUM);
        return -1;
    }

    mLutInfo.resize(lutFileNum);

    /* read wp_info */
    fp = fopen(kDISP_WP_PATH, "r");
    if (fp) {
        fscanf(fp, "%x", &wpInfo);
        fclose(fp);
    }
    ALOGD("Value of %s = %06x", kDISP_WP_PATH, wpInfo);
    /* read screenColorCal_panelInfo */
    fp = fopen(kDISP_ID_PATH, "r");
    if (fp) {
        fscanf(fp, "%x", &displayId);
        fclose(fp);
    }
    ALOGD("Value of %s = %06x", kDISP_ID_PATH, wpInfo);
    /* parse factory calib lut files if wp_info equal displayId */
    if (wpInfo == displayId || reset) {
        for (int i = 0; i < CALIB_FILE_NUM; i++) {
            sprintf(path, "%s%s%d%s", "/mnt/vendor/persist/display/",
                    mLutFileName[i].c_str(), disp_id, ".txt");
            if (!readC3dLutData(path, i)) {
                calibLutNum++;
            }
        }
    }
    if (wpInfo == displayId) {
        if (calibLutNum > 0) {
            calibStatus = CALIB_STATUS_PASS;
        }
    } else {
        calibStatus = CALIB_STATUS_INVALID_ID;
    }

    for (int i = 0; i < lutFileNum; i++) {
        sprintf(path, "/vendor/etc/miLut_%d.txt", i);
        readC3dLutData(path, i);
    }

    property_set(kDISP_CALIBRATION_STAUS, std::to_string(calibStatus).c_str());
    return 0;
}

int DSPP_Platform::readC3dLutData(const char* const path, int lutId)
{
    FILE *fp = NULL;
    struct rgb_entry temp;
    int cnt = 0, ret = 0;
    std::vector<rgb_entry> &LutInfo = mLutInfo[lutId];

    if (LutInfo.size() == LUT3D_ENTRIES_SIZE) {
        ALOGD("%s: mLutInfo[%d] has been initiliazed, ignore %s\n",
                __FUNCTION__, lutId, path);
        return -1;
    }

    if (access(path, F_OK|R_OK) < 0) {
        ALOGE("%s: Can't access to %s", __FUNCTION__, path);
        return -1;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        ALOGE("%s: Can't open %s", __FUNCTION__, path);
        return -1;
    }

    while (fscanf(fp, "%u %u %u %u %u %u", &temp.in.r, &temp.in.g, &temp.in.b,
            &temp.out.r, &temp.out.g, &temp.out.b) != EOF) {
        LutInfo.push_back(temp);
        cnt++;
        if (cnt > LUT3D_ENTRIES_SIZE) {
            ALOGE("%s: %s line number is greater than entries size %d",
                    __FUNCTION__, path, LUT3D_ENTRIES_SIZE);
            ret = -1;
            break;
        }
    }

    fclose(fp);

    if (cnt == LUT3D_ENTRIES_SIZE) {
        ALOGD("%s: loaded %s to mLutInfo[%d] successfully\n", __FUNCTION__, path, lutId);
    } else {
        ALOGE("%s: %s is invalid\n", __FUNCTION__, path);
        LutInfo.clear();
        ret = -1;
    }

    return ret;
}

int DSPP_Platform::getAshmem(hidl_memory& memory, uint32_t size)
{
    int ret = 0;
    // Allocate memory.
    android::sp<IAllocator> ashmemAllocator = IAllocator::getService("ashmem");
    if (ashmemAllocator != nullptr) {
        ashmemAllocator->allocate(size, [&](bool success, const hidl_memory &mem) {
            if (success) {
                // now you can use the hidl_memory object 'mem' or pass it around
                memory = mem;
            } else {
                ALOGE("%s: Can not alloc memory\n", __FUNCTION__);
                ret = -1;
            }
        });
    } else {
        ALOGE("%s: Get IAllocator failed!\n", __FUNCTION__);
        ret = -1;
    }
    return ret;
}

int DSPP_Platform::getGamutTable(int lutId, hidl_memory& mem_c3d)
{
    int ret = 0;

    // Write lut to memory.
    if (mem_c3d.handle() != nullptr && lutId < mLutInfo.size()) {
        android::sp<IMemory> memory = mapMemory(mem_c3d);
        if (memory != nullptr) {
            const std::vector<rgb_entry> &LutInfo = mLutInfo[lutId];
            void* ptr = memory->getPointer();
            memory->update();
            uint32_t *data = (uint32_t *)ptr;
            // update memory however you wish after calling update and before calling commit
            // read data from lut file
            for (const auto rgb : LutInfo) {
                const uint32_t r_in_idx = (rgb.in.r + 1) / 64;
                const uint32_t g_in_idx = (rgb.in.g + 1) / 64;
                const uint32_t b_in_idx = (rgb.in.b + 1) / 64;
                const uint32_t data_idx = ((r_in_idx * MTK_C3D_SIZE + g_in_idx) *
                            MTK_C3D_SIZE + b_in_idx) * 3;

                if (r_in_idx > MTK_C3D_SIZE - 1 || r_in_idx > MTK_C3D_SIZE - 1 ||
                        g_in_idx > MTK_C3D_SIZE -1) {
                    ALOGE("%s: input rgb(%u %u %u) of lutId-%d is out of range\n",
                            __FUNCTION__, rgb.in.r, rgb.in.g, rgb.in.b, lutId);
                    ret = -1;
                    break;
                }

                //ALOGV("[c3d][%d]%u %u %u %u %u %u", data_idx / 3, rgb.in.r, rgb.in.g,
                //        rgb.in.b, rgb.out.r, rgb.out.b, rgb.out.b);
                data[data_idx + 0] = rgb.out.r;
                data[data_idx + 1] = rgb.out.g;
                data[data_idx + 2] = rgb.out.b;
            }

            memory->commit();
        } else {
            ALOGE("%s: memory map failed!", __FUNCTION__);
            ret = -1;
        }
    } else {
        ALOGE("%s: mem_c3d is NULL or lutId is greater than mLutInfo.size()\n", __FUNCTION__);
        ret = -1;
    }

    return ret;
}

int DSPP_Platform::setColorMode(int mode, int cookie, enum DISPLAY_PP_MODE * renderIntent)
{
    int ret = 0;
    switch ((DISPLAY_COLOR_MODE)mode) {
        case DISPLAY_COLOR_MODE_NATURE: {
            ret = setPQMode(DISP_PQ_ENHANCE);
            ret |= setGammaIndex(DISP_GAMMA_INDEX_DEFAULT);
            ret |= setC3dLut(0, C3D_LUT_OFF);
        } break;
        case DISPLAY_COLOR_MODE_sRGB: {
            ret = setPQMode(DISP_PQ_STANDARD);
            ret |= setGammaIndex(DISP_GAMMA_INDEX_DEFAULT);
            ret |= setC3dLut(C3D_LUT_MODE_SRGB_D65, C3D_LUT_SET);
        } break;
        case DISPLAY_COLOR_MODE_P3: {
            ret = setPQMode(DISP_PQ_P3_D65);
            ret |= setGammaIndex(DISP_GAMMA_INDEX_DEFAULT);
            ret |= setC3dLut(C3D_LUT_MODE_P3_D65, C3D_LUT_SET);
        } break;
        case DISPLAY_COLOR_MODE_ACNORMAL: {
            ret = setPQMode(DISP_PQ_DEFAULT);
            ret |= setGammaIndex(DISP_GAMMA_INDEX_DEFAULT);
            ret |= setC3dLut(C3D_LUT_MODE_P3_D75, C3D_LUT_SET);
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_WCG: {
            int wcgState = cookie;
            if (wcgState == DATASPACE_SRGB) {
                ret = setC3dLut(C3D_LUT_MODE_NATURE_SRGB, C3D_LUT_SET);
            } else if (wcgState == DATASPACE_P3) {
                ret = setC3dLut(C3D_LUT_MODE_NATURE_P3, C3D_LUT_SET);
            }
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_SRGB: {
            ret = setPQMode(DISP_PQ_EXPERT_sRGB);
            ret |= setC3dLut(C3D_LUT_MODE_SRGB_D75, C3D_LUT_SET);
        } break;
        case DISPLAY_COLOR_MODE_EXPERT_P3: {
            ret = setPQMode(DISP_PQ_EXPERT_P3);
            ret |= setC3dLut(C3D_LUT_MODE_P3_D75, C3D_LUT_SET);
        } break;
        default:
            break;
    }

    return ret;
}

int DSPP_Platform::setGlobalPA(int caseId, int value, int dimming, GlobalPAParam param)
{
    int ret = 0;
    switch (caseId) {
        case EXPERT_H:
            ret = setHSVC(caseId, value);
            break;
        case EXPERT_S:
            ret = setHSVC(caseId, value);
            break;
        case EXPERT_V:
            ret = setHSVC(caseId, value);
            break;
        case EXPERT_CONTRAST:
            ret = setHSVC(caseId, value);
            break;
        case EXPERT_CLEAR:
        case EXPERT_RESTORE:
            // DisplayEffect HandleExpert will call setParams to transfer clear or restore param first.
            ret = setHSVC(EXPERT_H, param.hue);
            ret |= setHSVC(EXPERT_S, param.saturation);
            ret |= setHSVC(EXPERT_V, param.value);
            ret |= setHSVC(EXPERT_CONTRAST, param.contrast);
            break;
        case EXPERT_SRE_H_AND_S:
            ret = setHSVC(EXPERT_H, (value >> 8) & 0xFF);
            ret |= setHSVC(EXPERT_S, value & 0xFF);
            ALOGV("EXPERT_SRE_H_AND_S hue = %d, saturation = %d", (value >> 8) & 0xFF, value & 0xFF);
        break;
        case NOT_EXPERT_SRE_H_AND_S:
            ret = setHSVC(EXPERT_H, (value >> 8) & 0xFF);
            ret |= setHSVC(EXPERT_S, value & 0xFF);
            ret |= setHSVC(EXPERT_V, 0);
            ret |= setHSVC(EXPERT_CONTRAST, 0);
            ALOGV("NOT_EXPERT_SRE_H_AND_S hue = %d, saturation = %f", (value >> 8) & 0xFF, value & 0xFF);
        break;
        default:
            break;
    }
    return ret;
}

int DSPP_Platform::setGammaCorrection(int targetGamma, int type, struct DispPPParam *pSDPPParams)
{
    int ret = 0;
    if (type == GAMMA_GC) {
        ret = setGammaIndex(targetGamma);
    }
    return ret;
}

int DSPP_Platform::setLut(int lutId, int cookie, int dimming)
{
    int ret = 0;
    ret = setC3dLut(lutId, cookie);
    return ret;
}

int DSPP_Platform::DisablePA()
{
    int ret = 0;
    ret = setHSVC(EXPERT_H, 0);
    ret |= setHSVC(EXPERT_S, 0);
    ret |= setHSVC(EXPERT_V, 0);
    ret |= setHSVC(EXPERT_CONTRAST, 0);
    return ret;
}

int DSPP_Platform::DisableGC()
{
    int ret = 0;

    ret = setGammaIndex(220);

    return ret;
}

int DSPP_Platform::DisableLut()
{
    int ret = 0;

    ret = setC3dLut(0, C3D_LUT_OFF);

    return ret;
}
} // namespace android
