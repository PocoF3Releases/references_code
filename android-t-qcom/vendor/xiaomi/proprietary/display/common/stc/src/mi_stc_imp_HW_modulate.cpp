#include <cstring>
#include <algorithm>
#include <android/log.h>
#include "mi_stc_imp_HW_modulate.h"
#include <binder/Parcel.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include "display_color_processing.h"
#include <cutils/properties.h>
#include "timer.hpp"

#define LOG_TAG "MiStcImpl"
#define CALIB_FILE_NUM 4

namespace snapdragoncolor {

ScPostBlendInterface* GetScPostBlendInterface(uint32_t major_version,uint32_t minor_version)
{
    ScPostBlendInterface *stc_intf = nullptr;
    if (major_version == 2 && minor_version == 0) {
        STC_LOGI("MiStcImpl created");
        stc_intf = new MiStcImpl;
    } else {
        STC_LOGE("Invalid major_version %d minor_version %d", major_version, minor_version);
    }

    return stc_intf;
}

int MiStcImpl::log_level = 1;

MiStcImpl * MiStcImpl::mi_impl_[kMaxNumImpl];

void timerCallbackFunc(void *user, int caseId ,int generation, int value)
{
    MiStcImpl *source = (MiStcImpl *) user;
    source->timerCallback(caseId, generation, value);
}

MiStcImpl::MiStcImpl()
{
    set_prop_funcs_[kPostBlendInverseGammaHwConfig] = &MiStcImpl::SetPostBlendInvGammaConfig;
    set_prop_funcs_[kPostBlendGammaHwConfig] = &MiStcImpl::SetPostBlendGammaConfig;
    set_prop_funcs_[kPostBlendGamutHwConfig] = &MiStcImpl::SetPostBlendGamutConfig;
    set_prop_funcs_[static_cast<ScProperty>(kPrivatePostBlendPccHwConfig)] = &MiStcImpl::SetPostBlendPccConfig;
    set_prop_funcs_[static_cast<ScProperty>(kPrivatePostBlendPaHwConfig)] = &MiStcImpl::SetPostBlendPAConfig;
    set_prop_funcs_[kSetColorTransform] = &MiStcImpl::SetColorTransform;

    get_prop_funcs_[kModeList] = &MiStcImpl::GetModeList;
    get_prop_funcs_[kNeedsUpdate] = &MiStcImpl::GetNeedsUpdate;
    get_prop_funcs_[static_cast<ScProperty>(kPrivatePostBlendHwConfigSupport)] = &MiStcImpl::GetSupportHwConfig;

    process_op_funcs_[kScModeRenderIntent] = &MiStcImpl::ProcessModeRenderIntent;
    process_op_funcs_[kScModeSwAssets] = &MiStcImpl::ProcessModeSwAssets;
}

int MiStcImpl::Init(const std::string &panel_name)
{
    std::lock_guard<std::mutex> guard(lock_);
    init_done_ = true;
    int ret = 0;
    // Creates fake mode with kOemModulateHw
    ColorMode mode;
    mode.intent = kOemModulateHw;
    modes_list_.list.push_back(mode);
    hw_assets.push_back(kPbGamut);
    hw_assets.push_back(kPbIgc);
    hw_assets.push_back(kPbGC);
    hw_assets.push_back(kPbPCC);
    hw_assets.push_back(kPbPa);
    std::memset(&mi_pcc, 0x00, sizeof(pcc_coeff_data));
    mi_pcc.r.r = 1.0;
    mi_pcc.g.g = 1.0;
    mi_pcc.b.b = 1.0;
    native_pcc.r.r = 1.0;
    native_pcc.g.g = 1.0;
    native_pcc.b.b = 1.0;
    mi_eyecare_pcc.r.r = 1.0;
    mi_eyecare_pcc.g.g = 1.0;
    mi_eyecare_pcc.b.b = 1.0;

    if (!mi_impl_[kMiImplPrimary]) {
        // Start MiStcService and connect to it.
        STC_LOGI("Initializing MiStcService");
        MiStcService::init();

        sp<IMiStcService> imistcservice = interface_cast<IMiStcService>(
            defaultServiceManager()->getService(String16("display.mistcservice")));
        STC_LOGI("Getting IMiStcService...done!");

        if (imistcservice.get()) {
            imistcservice->connect(sp<qClient::IQClient>(this));
            mistcservice_ = reinterpret_cast<MiStcService *>(imistcservice.get());
        } else {
            STC_LOGE("Failed to acquire display.mistcservice");
            return -EINVAL;
        }
        // Get QService: to trigger screen update
        qservice_ = interface_cast<qService::IQService>(
          defaultServiceManager()->getService(String16("display.qservice")));
        if (qservice_ == NULL) {
            STC_LOGE("Failed to acquire display.qservice");
        } else {
            STC_LOGI("Getting IQService...done!");
        }
        mMiCalibSupported = property_get_bool(RO_VENDOR_DISP_MI_CALIB_ENABLE, false);
        mMiNatureMode = property_get_bool(RO_VENDOR_DISP_NATURE_MODE_ENABLE, false);
        mMiAIDisp = property_get_bool(RO_VENDOR_AI_DISP_ENABLE, false);
        mMiExpertCalib = property_get_bool(RO_VENDOR_EXPERT_CALIB_ENABLE, false);
        mDispId = kMiImplPrimary;
        STC_LOGI("MiImplPrimary(%p) is initialized for panel %s", mi_impl_[kMiImplPrimary], panel_name.c_str());
    } else if (!mi_impl_[kMiImplSecondaryBuiltIn]) {
        // Get QService: to trigger screen update
        qservice_ = interface_cast<qService::IQService>(
          defaultServiceManager()->getService(String16("display.qservice")));
        if (qservice_ == NULL) {
            STC_LOGE("Failed to acquire display.qservice");
        } else {
            STC_LOGI("Getting IQService...done!");
        }
        mMiCalibSupported = property_get_bool(RO_VENDOR_DISP_MI_CALIB_ENABLE, false);
        mMiNatureMode = property_get_bool(RO_VENDOR_DISP_NATURE_MODE_ENABLE, false);
        mMiAIDisp = property_get_bool(RO_VENDOR_AI_DISP_ENABLE, false);
        mMiExpertCalib = property_get_bool(RO_VENDOR_EXPERT_CALIB_ENABLE, false);
        mDispId = kMiImplSecondaryBuiltIn;
        STC_LOGI("MiImplSecondaryBuiltIn(%p) is initialized for panel %s", mi_impl_[kMiImplSecondaryBuiltIn], panel_name.c_str());
    } else {
        STC_LOGE("MiStcImpl exceeds max number: %p", this);
        return -EINVAL;
    }
    mColorMode.intent = kMaxRenderIntent;
    mColorMode.gamut = ColorPrimaries_Max;
    mParseXml[mDispId] = new MiParseXml();
#ifdef EYECARE_V4
    mEyeCare[mDispId] = new EyeCare(mDispId);
#endif
    mPicHDRAlgo[mDispId] = new PicHDRAlgo();
    initLut(mDispId);
    InitGamma(mDispId);
    mi_impl_[mDispId] = this;
    initSre(mDispId);
    return 0;
}

int MiStcImpl::initSre(MiImplType disp_id)
{
    char temp[50];
    const unsigned int max_count = 1024;
    char *tokens[max_count] = {NULL};
    unsigned int count = 0;
    int ret = 0;
    char p[4096];
    string key("sreLut");
    for (int i = 0; i < SRE_LUT_NUM; i++) {
        sprintf(temp, "%d", i);
        key = "sreLut";
        key.append(temp);
        ret = mParseXml[disp_id]->parseXml((int)disp_id, key, p, tokens, &count);
        if (ret != 0 || count != SRE_LUT_LEN) {
            STC_LOGE("Failed to parse sreLut, ret %d, count %d", ret, count);
            break;
        }
        for (int j = 0; j < count; j++) {
            SreLutLevel[i][j] = atoi(tokens[j]);
        }
    }

    if (ret == 0)
        mSreInitilized = 1;
    key = "sreGcIndex";
    ret = mParseXml[disp_id]->parseXml((int)disp_id, key, p, sizeof(p));
    if (ret != 0) {
        STC_LOGE("parse sreGcIndex failed");
    } else {
        mSreGcIndex = atoi(p);
    }
    return ret;
}

int MiStcImpl::initLut(MiImplType disp_id)
{
    struct rgb_entry temp;
    char path[256];
    std::vector<struct rgb_entry> rgb;
    FILE *fp = NULL;
    int ret = 0;
    char panel_name[255];
    string file_name = disp_id == kMiImplPrimary ? PRIM_PANEL_INFO_PATH : SEC_PANEL_INFO_PATH;

    if (mParseXml[disp_id].get()) {
        mParseXml[disp_id]->getPanelName(file_name.c_str(),panel_name);
    }
    LutInfo.clear();
    //parse 3d lut info
    sprintf(path, "/odm/etc/disp%d/%s/miLutInfo.txt",disp_id, panel_name);
    if (access(path, F_OK|R_OK) < 0) {
        STC_LOGE("can not access file %s", path);
        goto FINISH;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        STC_LOGE("open file error %s", path);
        goto FINISH;
    }
    ret = fscanf(fp, "%d", &LutFileNum);
    fclose(fp);
    if (ret < 0)
        goto FINISH;
    // parse factory calibration lut files
    for (int i = 0; i < CALIB_FILE_NUM; i++) {
        sprintf(path, "%s%s%d%s", CALIB_PATH, mLutFileName[i].c_str(), disp_id, ".txt");

        STC_LOGD("reading %s", path);
        if (access(path, F_OK|R_OK) < 0) {
            STC_LOGE("can not access file %s", path);
            goto DEFAULT;
        }
        ret = readLutData(path);
        if (ret < 0) {
            STC_LOGE("Failed to parse calib lut srgb file");
            LutInfo.clear();
            goto DEFAULT;
        }
    }

    for (int i = CALIB_FILE_NUM; i < LutFileNum; i++) {
        sprintf(path, "/odm/etc/disp%d/%s/miLut_%d.txt", disp_id, panel_name, i);
        ret = readLutData(path);
        if (ret != 0) {
            STC_LOGE("Failed to parse miLut file");
            goto FINISH;
        }
    }
    mLutInitialized = true;
    goto FINISH;

DEFAULT:
    for (int i = (LutInfo.size() / LUT3D_ENTRIES_SIZE); i < LutFileNum; i++) {
        sprintf(path, "/odm/etc/disp%d/%s/miLut_%d.txt", disp_id, panel_name, i);
        ret = readLutData(path);
        if (ret != 0) {
            STC_LOGE("Failed to parse miLut file");
            goto FINISH;
        }
    }
    mLutInitialized = true;
FINISH:
    return 0;
}

void MiStcImpl::InitGamma(MiImplType disp_id)
{
    int ret = 0;
    FILE *fp = NULL;
    int gammaData;
    int cnt = 0;
    char path[256];
    char panel_name[255];
    string file_name = disp_id == kMiImplPrimary ? PRIM_PANEL_INFO_PATH : SEC_PANEL_INFO_PATH;

    igcDataBuf.r.clear();
    igcDataBuf.g.clear();
    igcDataBuf.b.clear();
    gcDataBuf.r.clear();
    gcDataBuf.g.clear();
    gcDataBuf.b.clear();
    mIgcFileCnt = 0;
    mGcFileCnt = 0;

    if (mParseXml[disp_id].get()) {
        mParseXml[disp_id]->getPanelName(file_name.c_str(),panel_name);
    } else {
        STC_LOGE("%s can not  get mParseXml[%d]", __func__, disp_id);
        return;
    }
    while(1) {
        cnt = 0;
        sprintf(path, "/odm/etc/disp%d/%s/mi_igc_%d.txt", disp_id, panel_name, mIgcFileCnt);
        if (access(path, F_OK|R_OK) < 0) {
            STC_LOGE("can not access file %s", path);
            break;
        }
        fp = fopen(path, "r");
        if (fp == NULL) {
            STC_LOGE("open file error %s", path);
            break;
        }
        while (fscanf(fp, "%d", &gammaData) != EOF) {
            igcDataBuf.r.push_back(gammaData);
            igcDataBuf.g.push_back(gammaData);
            igcDataBuf.b.push_back(gammaData);
            cnt++;
        }
        fclose(fp);
        if (cnt != IgcConfig.num_of_entries) {
             STC_LOGE("Invalid gamma file %s, cnt %d, numOfEntries %d", path, cnt, IgcConfig.num_of_entries);
             break;
        }
        mIgcFileCnt++;
    }
    while(1) {
        cnt = 0;
        sprintf(path, "/odm/etc/disp%d/%s/mi_gc_%d.txt", disp_id, panel_name, mGcFileCnt);
        if (access(path, F_OK|R_OK) < 0) {
            STC_LOGE("can not access file %s", path);
            break;
        }
        fp = fopen(path, "r");
        if (fp == NULL) {
            STC_LOGE("open file error %s", path);
            break;
        }
        while (fscanf(fp, "%d", &gammaData) != EOF) {
            gcDataBuf.r.push_back(gammaData);
            gcDataBuf.g.push_back(gammaData);
            gcDataBuf.b.push_back(gammaData);
            cnt++;
        }
        fclose(fp);
        if (cnt != GcConfig.num_of_entries) {
             STC_LOGE("Invalid gamma file %s, cnt %d, numOfEntries %d", path, cnt, GcConfig.num_of_entries);
             break;
        }
        mGcFileCnt++;
    }
}

int MiStcImpl::readLutData(char* path)
{
    FILE *fp = NULL;
    struct rgb_entry temp;
    int cnt = 0;
    int ret = 0;
    if (access(path, F_OK|R_OK) < 0) {
        STC_LOGE("can not access file %s", path);
        return -1;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        STC_LOGE("open file error %s", path);
        return -1;
    }
    while (fscanf(fp, "%u %u %u %u %u %u", &temp.in.r, &temp.in.g, &temp.in.b,
            &temp.out.r, &temp.out.g, &temp.out.b) != EOF) {
        LutInfo.push_back(temp);
        cnt++;
        if (cnt > LUT3D_ENTRIES_SIZE) {
            STC_LOGE("Invalid lut file");
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    if (cnt != LUT3D_ENTRIES_SIZE) {
         STC_LOGE("Invalid Lut file %s", path);
         return -1;
    }
    return ret;
}

int MiStcImpl::DeInit()
{
    std::lock_guard<std::mutex> guard(lock_);
    init_done_ = false;
    return 0;
}

int MiStcImpl::SetProperty(const ScPayload &payload)
{
    std::lock_guard<std::mutex> guard(lock_);

    if (init_done_ == false || payload.version != sizeof(ScPayload))
        return -EINVAL;

    auto it = set_prop_funcs_.find(payload.prop);
    if (it == set_prop_funcs_.end() || !it->second)
        return -EINVAL;

    return (this->*(it->second))(payload);
}

int MiStcImpl::GetProperty(ScPayload *payload)
{
    std::lock_guard<std::mutex> guard(lock_);

    if (init_done_ == false || !payload || payload->version != sizeof(ScPayload))
    return -EINVAL;

    auto it = get_prop_funcs_.find(payload->prop);
    if (it == get_prop_funcs_.end() || !it->second)
        return -EINVAL;

    return (this->*(it->second))(payload);
}

int MiStcImpl::ProcessOps(ScOps op,const ScPayload &input, ScPayload * output)
{
    std::lock_guard<std::mutex> guard(lock_);

    STC_LOGV("Process Ops called, op = %d", op);
    if (init_done_ == false || !output) {
        STC_LOGE("Invalid params init_done_ %d output %pK", init_done_, output);
        return -EINVAL;
    }

    auto it = process_op_funcs_.find(op);
    if (it == process_op_funcs_.end() || !it->second) {
        STC_LOGE("Invalid ops %d", op);
        return -EINVAL;
    }

    return (this->*(it->second))(input, output);
}

int MiStcImpl::SetPostBlendGamutConfig(const ScPayload &payload)
{
    if (payload.len != sizeof(PostBlendGamutHwConfig) || !payload.payload) {
        STC_LOGE("len of Input params act %zd exp %d", payload.len, sizeof(PostBlendGamutHwConfig));
        return -EINVAL;
    }

    PostBlendGamutHwConfig* config = reinterpret_cast<PostBlendGamutHwConfig*>(payload.payload);
    STC_LOGV("PostBlend Gamut HW Config: num_of_grid_entires %d, grid_entries_width %d",
        config->num_of_grid_entries, config->grid_entries_width);

    return 0;
}

int MiStcImpl::SetPostBlendGammaConfig(const ScPayload &payload)
{
    int paramMax = 1023;
    float temp = 0;
    uint32_t data = 0;
    float temp_2_3 = 0;

    if (payload.len != sizeof(PostBlendGammaHwConfig) || !payload.payload) {
        STC_LOGE("len of Input params act %zd exp %d", payload.len, sizeof(PostBlendGammaHwConfig));
        return -EINVAL;
    }

    PostBlendGammaHwConfig* config = reinterpret_cast<PostBlendGammaHwConfig*>(payload.payload);
    STC_LOGV("PostBlend Gamma HW Config: num_of_entries %d, entries_width %d",
        config->num_of_entries, config->entries_width);
    GcConfig.num_of_entries = config->num_of_entries;
    GcConfig.entries_width = config->entries_width;
    paramMax = pow(2, GcConfig.entries_width) - 1;
    for (int i = 0; i < GcConfig.num_of_entries; i++) {
        temp_2_3 = temp = ((double)i)/(GcConfig.num_of_entries - 1);
        data = (int)(temp * paramMax + 0.5);
        defaultGcData.r.push_back(data);  // default igc 1.0, used when xml mode igc disabled.
        temp = pow(temp, 1/2.2);
        data = (int)(temp * paramMax + 0.5);
        nativeGcData.r.push_back(data);
        temp_2_3 = pow(temp, 1/2.3);
        data = (int)(temp * paramMax + 0.5);
        GC2_3.r.push_back(data);
    }
    defaultGcData.g.assign(defaultGcData.r.begin(), defaultGcData.r.end());
    defaultGcData.b.assign(defaultGcData.r.begin(), defaultGcData.r.end());
    nativeGcData.g.assign(nativeGcData.r.begin(), nativeGcData.r.end());
    nativeGcData.b.assign(nativeGcData.r.begin(), nativeGcData.r.end());
    srcGcData = nativeGcData;
    GC2_3.g.assign(GC2_3.r.begin(), GC2_3.r.end());
    GC2_3.b.assign(GC2_3.r.begin(), GC2_3.r.end());
    gcDataBackup = defaultGcData;

    return 0;
}

int MiStcImpl::SetPostBlendInvGammaConfig(const ScPayload &payload)
{
    int paramMax = 4095;
    float temp = 0;
    uint32_t data = 0;
    if (payload.len != sizeof(PostBlendInverseGammaHwConfig) || !payload.payload) {
        STC_LOGE("len of Input params act %zd exp %d", payload.len,
            sizeof(PostBlendInverseGammaHwConfig));
        return -EINVAL;
    }

    PostBlendInverseGammaHwConfig* config =
        reinterpret_cast<PostBlendInverseGammaHwConfig*>(payload.payload);
    STC_LOGV("PostBlend InverseGamma HW Config: num_of_entries %d, entries_width %d",
        config->num_of_entries, config->entries_width);
    IgcConfig.num_of_entries = config->num_of_entries;
    IgcConfig.entries_width = config->entries_width;
    paramMax = pow(2, IgcConfig.entries_width) - 1;
    for (int i = 0; i < IgcConfig.num_of_entries; i++) {
        temp = ((double)i)/(IgcConfig.num_of_entries - 1);
        data = (int)(temp * paramMax + 0.5);
        defaultIgcData.r.push_back(data);  // default igc 1.0, used when xml mode igc disabled.
        temp = pow(temp, 2.2);
        data = (int)(temp * paramMax + 0.5);
        nativeIgcData.r.push_back(data);
        srcIgcData.r.push_back(data);
    }
    defaultIgcData.g.assign(defaultIgcData.r.begin(), defaultIgcData.r.end());
    defaultIgcData.b.assign(defaultIgcData.r.begin(), defaultIgcData.r.end());
    nativeIgcData.g.assign(nativeIgcData.r.begin(), nativeIgcData.r.end());
    nativeIgcData.b.assign(nativeIgcData.r.begin(), nativeIgcData.r.end());
    srcIgcData.g.assign(srcIgcData.r.begin(), srcIgcData.r.end());
    srcIgcData.b.assign(srcIgcData.r.begin(), srcIgcData.r.end());
    igcDataBackup = defaultIgcData;
    return 0;
}

int MiStcImpl::SetPostBlendPccConfig(const ScPayload &payload)
{
    if (payload.len != sizeof(PostBlendPccHwConfig) || !payload.payload) {
        STC_LOGE("len of Input params act %zd exp %d", payload.len,
            sizeof(PostBlendPccHwConfig));
        return -EINVAL;
    }

    PostBlendPccHwConfig* config =
        reinterpret_cast<PostBlendPccHwConfig*>(payload.payload);
    STC_LOGV("PostBlend PCC HW Config");
    return 0;
}

int MiStcImpl::SetPostBlendPAConfig(const ScPayload &payload)
{
    if (payload.len != sizeof(PostBlendPaHwConfig) || !payload.payload) {
        STC_LOGE("len of Input params act %zd exp %d", payload.len,
            sizeof(PostBlendPaHwConfig));
        return -EINVAL;
    }

    PostBlendPaHwConfig* config =
        reinterpret_cast<PostBlendPaHwConfig*>(payload.payload);
    STC_LOGV("PostBlend PA HW Config");
    return 0;
}

int MiStcImpl::GetModeList(ScPayload *payload)
{
    if (payload->len != sizeof(ColorModeList) || !payload->payload)
        return -EINVAL;

    ColorModeList *mode_list = reinterpret_cast<ColorModeList *>(payload->payload);
    mode_list->list = modes_list_.list;
    return 0;
}

int MiStcImpl::GetNeedsUpdate(ScPayload *payload)
{
    STC_LOGV("GetNeedsUpdate calling %d", needs_update_);
    bool *ptr;
    if (payload->len != sizeof(bool) || !payload->payload)
        return -EINVAL;
    if (mWaitRefresh)
        needs_update_ = true;
    ptr = reinterpret_cast<bool *>(payload->payload);
    if (mStcEnable)
        *ptr = needs_update_;
    else
        *ptr = 0;

    return 0;
}

int MiStcImpl::GetSupportHwConfig(ScPayload *payload)
{
    STC_LOGV("GetSupportHwConfig calling");
    std::vector<std::string> *ptr;
    if (payload->len != sizeof(hw_assets) || !payload->payload)
        return -EINVAL;

    ptr = reinterpret_cast<std::vector<std::string> *>(payload->payload);
    ptr->swap(hw_assets);
    return 0;
}

int MiStcImpl::SetColorTransform(const ScPayload &payload)
{
    STC_LOGV("SetColorTransform calling");
    float* ptr;
    int enable = 0;
    ptr = reinterpret_cast<float *>(payload.payload);
    std::memcpy(native_coeff_array.begin(), ptr, sizeof(native_coeff_array));
    native_pcc.r.r = native_coeff_array[1];
    native_pcc.g.r = native_coeff_array[2];
    native_pcc.b.r = native_coeff_array[3];
    native_pcc.r.g = native_coeff_array[5];
    native_pcc.g.g = native_coeff_array[6];
    native_pcc.b.g = native_coeff_array[7];
    native_pcc.r.b = native_coeff_array[9];
    native_pcc.g.b = native_coeff_array[10];
    native_pcc.b.b = native_coeff_array[11];
    native_pcc.r.c = native_coeff_array[13];
    native_pcc.g.c = native_coeff_array[14];
    native_pcc.b.c = native_coeff_array[15];
#ifdef EYECARE_V4
    if (mEyeCare[mDispId].get()) {
        if ((int)native_pcc.r.r != 1 || (int)native_pcc.g.g != 1) {
            enable = 1;
        }
        mEyeCare[mDispId]->SetGoogleEffectEnable(enable);
        if (mEyeCareEnable && (enable != GoogleEffect)) {
            Mutex::Autolock autoLock(mLutDimmingLock);
            if (mCurLutInfo.size() == 0) {
                if (mLutInitialized) {
                    mTargetLutInfo.assign(&LutInfo[0], &LutInfo[LUT3D_ENTRIES_SIZE]);
                    STC_LOGW("%s mCurLutInfo is null, use lut 0 for eyecare calc", __func__);
                }
            }
            AssignEyeCare();
            AssignLut();
        }
        GoogleEffect = enable;
    }
#endif
    needs_update_ = true;
    update_list[PCC] = 1;

  return 0;
}

void MiStcImpl::MultiplyPccMatrix(struct pcc_coeff_data *color_comp,
                        struct pcc_coeff_data *new_color_comp)
{
    std::array<double, kMatrixSize> out_coeff_array;
    std::array<double, kMatrixSize> mi_coeff_array =
        {new_color_comp->r.r, new_color_comp->g.r, new_color_comp->b.r, 0,
        new_color_comp->r.g, new_color_comp->g.g, new_color_comp->b.g, 0,
        new_color_comp->r.b, new_color_comp->g.b, new_color_comp->b.b, 0,
        new_color_comp->r.c, new_color_comp->g.c, new_color_comp->b.c, 1};
    std::array<double, kMatrixSize> coeff_array =
        {color_comp->r.r, color_comp->g.r, color_comp->b.r, 0,
        color_comp->r.g, color_comp->g.g, color_comp->b.g, 0,
        color_comp->r.b, color_comp->g.b, color_comp->b.b, 0,
        color_comp->r.c, color_comp->g.c, color_comp->b.c, 1};

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            out_coeff_array[i * 4 + j] = coeff_array[i * 4] * mi_coeff_array[j] \
                                       + coeff_array[i * 4 + 1] * mi_coeff_array[4 + j] \
                                       + coeff_array[i * 4 + 2] * mi_coeff_array[8 + j] \
                                       + coeff_array[i * 4 + 3] * mi_coeff_array[12 + j];
        }

    color_comp->r.r = out_coeff_array[0];
    color_comp->g.r = out_coeff_array[1];
    color_comp->b.r = out_coeff_array[2];
    color_comp->r.g = out_coeff_array[4];
    color_comp->g.g = out_coeff_array[5];
    color_comp->b.g = out_coeff_array[6];
    color_comp->r.b = out_coeff_array[8];
    color_comp->g.b = out_coeff_array[9];
    color_comp->b.b = out_coeff_array[10];
    color_comp->r.c = out_coeff_array[12];
    color_comp->g.c = out_coeff_array[13];
    color_comp->b.c = out_coeff_array[14];
}

void MiStcImpl::HandleIGCDither(GammaPostBlendConfig *cfg)
{
    if (enable_list_mi[IGC_DITHER]) {
        cfg->dither_en = true;
        cfg->dither_strength = igcDitherStrength;
    }
}

void MiStcImpl::EyeCareProcess(struct rgb_entry * in)
{
#ifdef EYECARE_V4
    struct rgb_entry * temp = NULL;

    if (mEyeCare[mDispId].get() && mEyeCarePaperId == PaperColorId::FULL_COLOR)
    {
        mEyeCare[mDispId]->SetEyeCare(mEyeCareLevel, mEyeCarePaperId, in);
        mEyeCareLutUpdate = true;
    } else {
        mEyeCareLutUpdate = false;
    }
#endif
}
void MiStcImpl::PicHDRProcess(struct rgb_entry* in)
{
    if (mPicHDRAlgo[mDispId].get() )
    {
        mPicHDRAlgo[mDispId]->calcHDRLut(in);
        mPicHDRLutUpdate = true;
    } else {
        mPicHDRLutUpdate = false;
    }
}

int MiStcImpl::ProcessModeRenderIntent(const ScPayload &in, ScPayload *out)
{
    STC_LOGV("ProcessModeRenderIntent calling, needs_update_ %d, disp id %d", needs_update_, mDispId);
    mWaitRefresh = 0;
    if (/*!needs_update_ || */!mStcEnable) {
        return 0;
    }
    PccConfig updated_cfg = {};
    int i;
    Timer t;
    bool need_dimming = false;
    std::memcpy(&updated_cfg.pcc_info, &mi_pcc, sizeof(pcc_coeff_data));
    HwConfigOutputParams *output = reinterpret_cast<HwConfigOutputParams *>(out->payload);
    for (auto &cur_hw : output->payload) {
        STC_LOGV("cur hw asset %s", cur_hw.hw_asset.c_str());
        if (cur_hw.hw_asset == kPbPCC) {
            PccConfig *cfg = reinterpret_cast<PccConfig*>(cur_hw.hw_payload.get());
            if (!enable_list_mi[PCC]) {
                current_pcc = cfg->pcc_info;
                continue;
            }
            cfg->enabled = true;
            // multiply with native PCC
            cfg->pcc_info.r.c = cfg->pcc_info.r.c/255.0;
            cfg->pcc_info.g.c = cfg->pcc_info.g.c/255.0;
            cfg->pcc_info.b.c = cfg->pcc_info.b.c/255.0;
            STC_LOGV("Native Pcc RGB[%f %f %f]", cfg->pcc_info.r.r, cfg->pcc_info.g.g, cfg->pcc_info.b.b);

            if (cfg->pcc_info.r.r < 1.0 && (abs(cfg->pcc_info.r.r - cfg->pcc_info.g.g) < 0.0001) && !GoogleEffect) {
                restore_pcc = cfg->pcc_info;
            }
            if (need_restore_pcc && !GoogleEffect) {
                cfg->pcc_info = restore_pcc;
                need_restore_pcc = false;
                STC_LOGD("restore pcc %f", restore_pcc.r.r);
            }
            MultiplyPccMatrix(&cfg->pcc_info, &updated_cfg.pcc_info);
#ifdef EYECARE_V4
            if (mEyeCareEnable && !mEyeCareLutUpdate) {
                std::memcpy(&updated_cfg.pcc_info, &mi_eyecare_pcc, sizeof(pcc_coeff_data));
                MultiplyPccMatrix(&cfg->pcc_info, &updated_cfg.pcc_info);
            }
#endif
            cfg->pcc_info.r.c = 255.0 * cfg->pcc_info.r.c;
            cfg->pcc_info.g.c = 255.0 * cfg->pcc_info.g.c;
            cfg->pcc_info.b.c = 255.0 * cfg->pcc_info.b.c;
            if (cfg->pcc_info.g.g < 0.01) {
                STC_LOGE("Invalid Pcc coeff, g.g %f",cfg->pcc_info.g.g);
            }
            current_pcc = cfg->pcc_info;
        } else if (cur_hw.hw_asset == kPbIgc) {
            GammaPostBlendConfig *cfg = reinterpret_cast<GammaPostBlendConfig*>(cur_hw.hw_payload.get());
            if (mModeChanged) {
                /*Change native data, based on cfg->enabled, if mode enabled this module,
                  native data use cfg payload, or use default */
                if (cfg->enabled) {
                    nativeIgcData.r.assign(cfg->r.begin(), cfg->r.end());
                    nativeIgcData.g.assign(cfg->g.begin(), cfg->g.end());
                    nativeIgcData.b.assign(cfg->b.begin(), cfg->b.end());
                } else {
                    nativeIgcData = defaultIgcData;
                }
                if (!mIgcEnable && enable_list_mi[IGC]) {
                    /* In this case, mi stc igc is closing and dimming to mode, mode change means
                       dimming target change */
                    mTargetChanged[IGC] = true;
                    targetIgcData = nativeIgcData;
                    mCurDimIndex[IGC] = 0;
                    enable_list_mi[IGC] = 1;
                    need_dimming = true;
                } else if (!mIgcEnable && !enable_list_mi[IGC]) {
                    /* In this case, mi stc does not set igc, dimming from one mode to another*/
                    targetIgcData = nativeIgcData;
                    if (enable_list_qcom[IGC] || cfg->enabled) {
                        /* only one or both of the src and target mode igc is enabled, need dimming*/
                        if (cfg->enabled == false)
                            cfg->enabled = true;
                        if (update_list[IGC] == 1) {
                            cfg->r.assign(igcData.r.begin(), igcData.r.end());
                            cfg->g.assign(igcData.g.begin(), igcData.g.end());
                            cfg->b.assign(igcData.b.begin(), igcData.b.end());
                        } else {
                            cfg->r.assign(igcDataBackup.r.begin(), igcDataBackup.r.end());
                            cfg->g.assign(igcDataBackup.g.begin(), igcDataBackup.g.end());
                            cfg->b.assign(igcDataBackup.b.begin(), igcDataBackup.b.end());
                        }
                        mCurDimIndex[IGC] = 0;
                        enable_list_mi[IGC] = 1;
                        need_dimming = true;
                    }
                }
                enable_list_qcom[IGC] = cfg->enabled;
            }
            if (!enable_list_mi[IGC]) {
                HandleIGCDither(cfg);
                continue;
            }
            cfg->enabled = true;
            if (enable_list_mi[IGC]) {
                if (update_list[IGC] == 1) {
                    cfg->r.assign(igcData.r.begin(), igcData.r.end());
                    cfg->g.assign(igcData.g.begin(), igcData.g.end());
                    cfg->b.assign(igcData.b.begin(), igcData.b.end());
                } else {
                    cfg->r.assign(igcDataBackup.r.begin(), igcDataBackup.r.end());
                    cfg->g.assign(igcDataBackup.g.begin(), igcDataBackup.g.end());
                    cfg->b.assign(igcDataBackup.b.begin(), igcDataBackup.b.end());
                }
            }
            HandleIGCDither(cfg);
        } else if (cur_hw.hw_asset == kPbGC) {
            GammaPostBlendConfig *cfg = reinterpret_cast<GammaPostBlendConfig*>(cur_hw.hw_payload.get());
            if (mModeChanged) {
                /*Change native data, based on cfg->enabled, if mode enabled this module,
                  native data use cfg payload, or use default */
                if (cfg->enabled) {
                    nativeGcData.r.assign(cfg->r.begin(), cfg->r.end());
                    nativeGcData.g.assign(cfg->g.begin(), cfg->g.end());
                    nativeGcData.b.assign(cfg->b.begin(), cfg->b.end());
                } else {
                    nativeGcData = defaultGcData;
                }
                if (!mGcEnable && enable_list_mi[GC]) {
                    /* In this case, mi stc gc is closing and dimming to mode, mode change means
                       dimming target change */
                    mTargetChanged[GC] = true;
                    targetGcData = nativeGcData;
                    mCurDimIndex[GC] = 0;
                    enable_list_mi[GC] = 1;
                    need_dimming = true;
                } else if (!mGcEnable && !enable_list_mi[GC]) {
                    /* In this case, mi stc does not set gc, dimming from one mode to another*/
                    targetGcData = nativeGcData;
                    if (enable_list_qcom[GC] || cfg->enabled) {
                        /* only one or both of the src and target mode gc is enabled, need dimming*/
                        cfg->enabled = true;
                        if (update_list[GC] == 1) {
                            cfg->r.assign(gcData.r.begin(), gcData.r.end());
                            cfg->g.assign(gcData.g.begin(), gcData.g.end());
                            cfg->b.assign(gcData.b.begin(), gcData.b.end());
                        } else {
                            cfg->r.assign(gcDataBackup.r.begin(), gcDataBackup.r.end());
                            cfg->g.assign(gcDataBackup.g.begin(), gcDataBackup.g.end());
                            cfg->b.assign(gcDataBackup.b.begin(), gcDataBackup.b.end());
                        }
                        mCurDimIndex[GC] = 0;
                        enable_list_mi[GC] = 1;
                        need_dimming = true;
                    }
                }
                enable_list_qcom[GC] = cfg->enabled;
            }

            if (!enable_list_mi[GC] && !mSreEnable) {
                continue;
            }
            cfg->enabled = true;
            if (update_list[GC] == 1) {
                cfg->r.assign(gcData.r.begin(), gcData.r.end());
                cfg->g.assign(gcData.g.begin(), gcData.g.end());
                cfg->b.assign(gcData.b.begin(), gcData.b.end());
            } else {
                cfg->r.assign(gcDataBackup.r.begin(), gcDataBackup.r.end());
                cfg->g.assign(gcDataBackup.g.begin(), gcDataBackup.g.end());
                cfg->b.assign(gcDataBackup.b.begin(), gcDataBackup.b.end());
            }
        } else if (cur_hw.hw_asset == kPbGamut) {
            GamutConfig *cfg = reinterpret_cast<GamutConfig*>(cur_hw.hw_payload.get());
            if (mModeChanged) {
                if (mDefaultLutInfo.size() == 0) {
                    if (mLutInitialized) {
                        mDefaultLutInfo.assign(&LutInfo[0], &LutInfo[LUT3D_ENTRIES_SIZE]);
                    } else {
                        mDefaultLutInfo.assign(cfg->gamut_info.entries,
                            &cfg->gamut_info.entries[LUT3D_ENTRIES_SIZE - 1] + 1);
                    }
                    for (int i = 0; i < mDefaultLutInfo.size(); i++) {
                        mDefaultLutInfo[i].out = mDefaultLutInfo[i].in;
                    }
                }
                /*Change native data, based on cfg->enabled, if mode enabled this module,
                  native data use cfg payload, or use default */
                if (cfg->enabled) {
                    mNativeLutInfo.assign(cfg->gamut_info.entries,
                                &cfg->gamut_info.entries[LUT3D_ENTRIES_SIZE - 1] + 1);
                } else {
                    mNativeLutInfo.assign(mDefaultLutInfo.begin(), mDefaultLutInfo.end());
                }
                if (mSrcLutInfo.size() == 0) {
                    mSrcLutInfo.assign(mNativeLutInfo.begin(), mNativeLutInfo.end());
                }
                if (!mLutEnable && enable_list_mi[LUT]) {
                    /* In this case, mi stc lut is closing and dimming to mode, mode change means
                       dimming target change */
                    mTargetChanged[LUT] = true;
                    mTargetLutInfo = mNativeLutInfo;
                    mCurDimIndex[LUT] = 0;
                    enable_list_mi[LUT] = 1;
                    if (mSreEnable)
                        mSreUpdate = 1;
                    need_dimming = true;
                } else if (!mLutEnable && !enable_list_mi[LUT]) {
                    /* In this case, mi stc does not set lut, dimming from one mode to another*/
                    mTargetLutInfo = mNativeLutInfo;
                    if (enable_list_qcom[LUT] || cfg->enabled) {
                        /* only one or both of the src and target mode lut is enabled, need dimming*/
                        cfg->enabled = true;
                        cfg->gamut_info.flags = 0;
                        cfg->gamut_info.lut3d_id = 1;
                        cfg->gamut_info.uniform = 1;
                        mCurDimIndex[LUT] = 0;
                        enable_list_mi[LUT] = 1;
                        need_dimming = true;
                        if (update_list[LUT] == 1) {
                            if (mCurLutInfo.size() != LUT3D_ENTRIES_SIZE) {
                                STC_LOGE("Invalid lut size");
                                continue;
                            }
                            std::memcpy(cfg->gamut_info.entries, mCurLutInfo.data(), sizeof(cfg->gamut_info.entries));
                        } else {
                            if (mLutInfoBackup.size() != LUT3D_ENTRIES_SIZE) {
                                STC_LOGE("Invalid backup lut size");
                                continue;
                            }
                            std::memcpy(cfg->gamut_info.entries, mLutInfoBackup.data(), sizeof(cfg->gamut_info.entries));
                        }
                    }
                }
                enable_list_qcom[LUT] = cfg->enabled;
            }

            if (!enable_list_mi[LUT] || !mLutInitialized) {
                if (mEyeCareEnable) {
                    cfg->enabled = true;
                    cfg->gamut_info.flags = 0;
                    cfg->gamut_info.lut3d_id = 1;
                    cfg->gamut_info.uniform = 1;
                    if (update_list[LUT] == 1) {
                        if (mEyeCareLut.size() != LUT3D_ENTRIES_SIZE) {
                            STC_LOGE("Invalid lut size");
                            mCurDimIndex[LUT] = 0;
                            need_dimming = true;
                            continue;
                        }
                        std::memcpy(cfg->gamut_info.entries, mEyeCareLut.data(), sizeof(cfg->gamut_info.entries));
                    } else {
                        if (mEyeCareLutBackup.size() != LUT3D_ENTRIES_SIZE) {
                            STC_LOGE("Invalid Eyecare backup lut size");
                            continue;
                        }
                        std::memcpy(cfg->gamut_info.entries, mEyeCareLutBackup.data(), sizeof(cfg->gamut_info.entries));
                    }
                }
                if (mPicHDREnable) {
                    cfg->enabled = true;
                    cfg->gamut_info.flags = 0;
                    cfg->gamut_info.lut3d_id = 1;
                    cfg->gamut_info.uniform = 1;
                    if (update_list[LUT] == 1) {
                        if (mPicHDRLut.size() != LUT3D_ENTRIES_SIZE) {
                            STC_LOGE("Invalid lut size");
                            mCurDimIndex[LUT] = 0;
                            need_dimming = true;
                            continue;
                        }
                        std::memcpy(cfg->gamut_info.entries, mPicHDRLut.data(), sizeof(cfg->gamut_info.entries));
                    } else {
                        if (mPicHDRLutBackup.size() != LUT3D_ENTRIES_SIZE) {
                            STC_LOGE("Invalid PicHDR backup lut size");
                            continue;
                        }
                        std::memcpy(cfg->gamut_info.entries, mPicHDRLutBackup.data(), sizeof(cfg->gamut_info.entries));
                    }
                }
                continue;
            }
            if (update_list[LUT] == 1) {
                if (mEyeCareEnable) {
                    if (mEyeCareLut.size() != LUT3D_ENTRIES_SIZE) {
                        STC_LOGE("Invalid eyecare lut size");
                        mCurDimIndex[LUT] = 0;
                        need_dimming = true;
                        continue;
                    }
                    std::memcpy(cfg->gamut_info.entries, mEyeCareLut.data(), sizeof(cfg->gamut_info.entries));
                }
                else if (mPicHDREnable) {
                    if (mPicHDRLut.size() != LUT3D_ENTRIES_SIZE) {
                        STC_LOGE("Invalid PicHDR lut size");
                        mCurDimIndex[LUT] = 0;
                        need_dimming = true;
                        continue;
                    }
                    std::memcpy(cfg->gamut_info.entries, mPicHDRLut.data(), sizeof(cfg->gamut_info.entries));
                } 
                else {
                    if (mCurLutInfo.size() != LUT3D_ENTRIES_SIZE) {
                        STC_LOGE("Invalid lut size");
                        continue;
                    }
                    std::memcpy(cfg->gamut_info.entries, mCurLutInfo.data(), sizeof(cfg->gamut_info.entries));
                }
            } else {
                if (mEyeCareEnable) {
                    if (mEyeCareLutBackup.size() != LUT3D_ENTRIES_SIZE) {
                        STC_LOGE("Invalid Eyecare backup lut size");
                        continue;
                    }
                    std::memcpy(cfg->gamut_info.entries, mEyeCareLutBackup.data(), sizeof(cfg->gamut_info.entries));
                }
                else if (mPicHDREnable) {
                    if (mPicHDRLutBackup.size() != LUT3D_ENTRIES_SIZE) {
                        STC_LOGE("Invalid PicHDR backup lut size");
                        continue;
                    }
                    std::memcpy(cfg->gamut_info.entries, mPicHDRLutBackup.data(), sizeof(cfg->gamut_info.entries));
                }
                else {
                    if (mLutInfoBackup.size() != LUT3D_ENTRIES_SIZE) {
                        STC_LOGE("Invalid backup lut size");
                        continue;
                    }
                    std::memcpy(cfg->gamut_info.entries, mLutInfoBackup.data(), sizeof(cfg->gamut_info.entries));
                }
            }
            cfg->enabled = true;
            cfg->gamut_info.flags = 0;
            cfg->gamut_info.lut3d_id = 1;
            cfg->gamut_info.uniform = 1;
        } else if (cur_hw.hw_asset == kPbPa) {
            PaConfig *cfg = reinterpret_cast<PaConfig*>(cur_hw.hw_payload.get());
            if (mModeChanged) {
                /*Change native data, based on cfg->enabled, if mode enabled this module,
                  native data use cfg payload, or use default */
                mDefault6Zone.flags = cfg->pa_info.six_zone_config.sz_cfg.flags;
                mDefault6Zone.num_entries = cfg->pa_info.six_zone_config.sz_cfg.num_entries;
                if (cfg->enabled) {
                    if (cfg->pa_info.enable_flags & 0x1) {
                      mNativePaData = cfg->pa_info.global_pa_config.data;
                    } else {
                        mNativePaData = mDefaultPaData;
                    }
                    if (cfg->pa_info.enable_flags & (0x1 << 8)) {
                        mTarget6Zone = cfg->pa_info.six_zone_config.sz_cfg;
                    } else {
                        mTarget6Zone = mDefault6Zone;
                    }
                } else {
                    mNativePaData = mDefaultPaData;
                    mTarget6Zone = mDefault6Zone;
                }
                if (!mPaEnable && enable_list_mi[PA]) {
                    /* In this case, mi stc pa is closing and dimming to mode, mode change means
                       dimming target change */
                    mTargetChanged[PA] = true;
                    mTargetPaData = mNativePaData;
                    mCurDimIndex[PA] = 0;
                    enable_list_mi[PA] = 1;
                    need_dimming = true;
                } else if (!mPaEnable && !enable_list_mi[PA]) {
                    /* In this case, mi stc does not set pa, dimming from one mode to another*/
                    mTargetPaData = mNativePaData;
                    if (enable_list_qcom[PA] || cfg->enabled) {
                        /* only one or both of the src and target mode pa is enabled, need dimming*/
                        cfg->enabled = true;
                        cfg->pa_info.enable_flags |= 1;
                        //cfg->pa_info.global_pa_config.flags = 1;
                        cfg->pa_info.global_pa_config.data = mPaData;
                        mCurDimIndex[PA] = 0;
                        enable_list_mi[PA] = 1;
                        need_dimming = true;
                    }
                }
                if (enable_list_mi[SIX_ZONE]) {
                    /* During mode change 6zone dimming, new mode change */
                    cfg->pa_info.enable_flags |= 0x1 << 8;
                    cfg->pa_info.six_zone_config.sz_cfg = mCur6Zone;
                    mTargetChanged[SIX_ZONE] = true;
                    mCurDimIndex[SIX_ZONE] = 0;
                    enable_list_mi[SIX_ZONE] = 1;
                    need_dimming = true;
                } else {
                    if (cfg->pa_info.enable_flags & (0x1 << 8) || enable_list_qcom[SIX_ZONE]) {
                        cfg->pa_info.six_zone_config.sz_cfg = mSrc6Zone;
                        mTargetChanged[SIX_ZONE] = true;
                        mCurDimIndex[SIX_ZONE] = 0;
                        enable_list_mi[SIX_ZONE] = 1;
                        need_dimming = true;
                    }
                }
                enable_list_qcom[PA] = cfg->pa_info.enable_flags & 0x1;
                enable_list_qcom[SIX_ZONE] = cfg->pa_info.enable_flags & (0x1 << 8);
            }

            if (!enable_list_mi[PA] && !enable_list_mi[SIX_ZONE]) {
                continue;
            }
            cfg->enabled = true;
            if (enable_list_mi[PA]) {
                cfg->pa_info.enable_flags |= 1;
                cfg->pa_info.global_pa_config.flags = 1;
                cfg->pa_info.global_pa_config.data = mPaData;
            }
            if (enable_list_mi[SIX_ZONE]) {
                cfg->pa_info.enable_flags |= 0x1 << 8;
                cfg->pa_info.six_zone_config.sz_cfg = update_list[SIX_ZONE] ? mCur6Zone : m6ZoneBackup;
            }
        }
    }

    if (need_dimming && mTimerGeneration[TIMER_SYNC] == 0) {
        t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
    }
    mModeChanged = 0;
    needs_update_ = false;
    return 0;
}

int MiStcImpl::checkModeChange(struct ModeRenderInputParams* params)
{
    int ret = 0;
    if (params->color_mode.intent != mColorMode.intent) {
        ret = 1;
    } else if (params->color_mode.gamut != mColorMode.gamut) {
        ret = 1;
    }
    mColorMode = params->color_mode;
    return ret;
}

int MiStcImpl::ProcessModeSwAssets(const ScPayload &in, ScPayload *out)
{
    int dimming = 1;
    if (!in.payload) {
      STC_LOGE("invalid input");
      return -EINVAL;
    }
    if (!mStcEnable)
        return 0;


    struct ModeRenderInputParams* params = reinterpret_cast< struct ModeRenderInputParams*>(in.payload);
    mModeChanged = checkModeChange(params);
    if (mModeChanged) {
        needs_update_ = true;
        if (mMiCalibSupported) {
            if (params->color_mode.intent_name == kHal_srgb) {
                this->mLutEnable = 1;
                this->enable_list_mi[LUT] = 1;
                this->SetLut(0, D65_sRGB, dimming);
            } else if (params->color_mode.intent_name == kHal_dcip3) {
                this->mLutEnable = 1;
                this->enable_list_mi[LUT] = 1;
                this->SetLut(0, D65_P3, dimming);
            } else if (params->color_mode.intent_name == kexpert_wcg_srgb
                && !mMiNatureMode) {
                this->mLutEnable = 1;
                this->enable_list_mi[LUT] = 1;
                this->SetLut(0, D75_sRGB, dimming);
            } else if (params->color_mode.intent_name == kexpert_wcg_dcip3
                && !mMiNatureMode) {
                this->mLutEnable = 1;
                this->enable_list_mi[LUT] = 1;
                this->SetLut(0, D75_P3, dimming);
            } else if (params->color_mode.intent < kOemCustomStart) {
                if (this->mLutEnable) {
                    //this->enable_list_mi[LUT] = 0;
                    this->mLutEnable = 0;
                    this->SetLut(0, 0, dimming);
                }
            }
            if (mMiExpertCalib) {
                if (params->color_mode.intent == DISPLAY_PP_MODE_mode267) {
                    this->enable_list_mi[LUT] = 1;
                    this->mLutEnable = 1;
                    this->SetLut(0, D75_sRGB, dimming);
                } else if (params->color_mode.intent == DISPLAY_PP_MODE_mode268) {
                    this->enable_list_mi[LUT] = 1;
                    this->mLutEnable = 1;
                    this->SetLut(0, D75_P3, dimming);
                }
            }
        }
        if (mMiNatureMode) {
            if (params->color_mode.intent_name == kexpert_wcg_srgb) {
                this->enable_list_mi[LUT] = 1;
                this->mLutEnable = 1;
                this->SetLut(0, nature_srgb, dimming);
                mSatShift = -0.1;
                mContrastShift = -0.018;
            } else if (params->color_mode.intent_name == kexpert_wcg_dcip3) {
                this->enable_list_mi[LUT] = 1;
                this->mLutEnable = 1;
                this->SetLut(0, nature_P3, dimming);
                mSatShift = -0.1;
                mContrastShift = -0.018;
            } else {
                mSatShift = 0;
                mContrastShift = 0;
            }
        }
    }
    return 0;
}

bool MiStcImpl::checkLutStatus(int modeId)
{
    for (int i = 0; i < LUT_MODE_NUM; i++) {
        if (modeId == mLutModeList[i])
            return false;
    }
    return true;
}

status_t MiStcImpl::notifyCallback(uint32_t command, const Parcel *input_parcel,
                                             Parcel *output_parcel) {
    status_t status = -EINVAL;
    uint32_t disp_id = 0;
    int gamma;
    int lutId;
    disp_pa_config_data paCfg;
    int enable = 0;
    int reInit = 0;
    int file_id = 0;
    STC_DisplayState state = kStateOn;
    Timer t;
    int dimming = 0;
    int sreLevel = 0;
    int sreDivisor = 0;

    if (IMiStcService::SET_DPPS_PROP != command
        && IMiStcService::ENABLE_STC != command
        && IMiStcService::SET_LOG != command
        && IMiStcService::SET_DISPLAY_STATE != command
        && IMiStcService::SET_PARAM != command
        && IMiStcService::GET_UPDATE_STATUS != command
        && IMiStcService::RESTORE_BL_PCC != command
        && IMiStcService::SET_SRE_GC != command
        && IMiStcService::STC_DUMP != command
        && IMiStcService::SET_EXIF != command) {
        enable = input_parcel->readInt32();
        disp_id = static_cast<uint32_t>(input_parcel->readInt32());
    }

    switch (command) {
        case IMiStcService::SET_PCC: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id])
                mi_impl_[disp_id]->enable_list_mi[PCC] = enable;
            else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
            if (enable == 0)
                goto FORCEREFRESH;

            std::array<double, kMatrixSize> pccCoeff;
            for (int i = 0; i < kMatrixSize; i++)
                pccCoeff[i] = static_cast<double>(input_parcel->readDouble());
            ApplyPCC(static_cast<MiImplType>(disp_id), pccCoeff.begin());
        } break;
        case IMiStcService::SET_PA: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id])
                mi_impl_[disp_id]->mPaEnable = enable;
            else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
            dimming = input_parcel->readInt32();
            paCfg.hue = input_parcel->readInt32();
            paCfg.saturation = input_parcel->readFloat();
            paCfg.value = input_parcel->readFloat();
            paCfg.contrast = input_parcel->readFloat();
            paCfg.sat_thresh = input_parcel->readFloat();
            ApplyPA(static_cast<MiImplType>(disp_id), paCfg, dimming);
        } break;
        case IMiStcService::SET_IGC: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id])
                mi_impl_[disp_id]->mIgcEnable = enable;
            else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }

            dimming = input_parcel->readInt32();
            gamma = input_parcel->readInt32();
            if (gamma == 0) {
                file_id = input_parcel->readInt32();
            }
            ApplyIGC(static_cast<MiImplType>(disp_id), gamma, file_id, dimming);
        } break;
        case IMiStcService::SET_SRE_IGC: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id])
                mi_impl_[disp_id]->mIgcEnable = enable;
            else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
            dimming = input_parcel->readInt32();
            gamma = input_parcel->readInt32();
            if (mi_impl_[disp_id]->tempData.size() > 0) {
                mi_impl_[disp_id]->tempData.clear();
            }
            input_parcel->readInt32Vector(&mi_impl_[disp_id]->tempData);
            ApplySREIGC(static_cast<MiImplType>(disp_id), mi_impl_[disp_id]->tempData, dimming);
        } break;
        case IMiStcService::SET_GC: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id])
                mi_impl_[disp_id]->mGcEnable = enable;
            else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
            dimming = input_parcel->readInt32();
            gamma = input_parcel->readInt32();
            if (gamma == 0)
                file_id = input_parcel->readInt32();
            ApplyGC(static_cast<MiImplType>(disp_id), gamma, file_id, dimming);
        } break;
        case IMiStcService::SET_LUT: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mLutInitialized == false) {
                STC_LOGE("Lut data does not initialized, can not set lut");
                break;
            }
            if (mi_impl_[disp_id]) {
                if (!mi_impl_[disp_id]->enable_list_mi[LUT] && !enable)
                    return 0;
                mi_impl_[disp_id]->mLutEnable = enable;
            } else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
            dimming = input_parcel->readInt32();
            reInit = input_parcel->readInt32();
            lutId = input_parcel->readInt32();
            ApplyLut(static_cast<MiImplType>(disp_id), reInit, lutId, dimming);
        } break;
        case IMiStcService::SET_SRE: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id])
                mi_impl_[disp_id]->mSreEnable = enable;
            else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
            dimming = input_parcel->readInt32();
            sreLevel = input_parcel->readInt32();
            sreDivisor = input_parcel->readInt32();
            ApplySRE(static_cast<MiImplType>(disp_id), sreLevel, sreDivisor, dimming);
        } break;
        case IMiStcService::SET_DPPS_PROP: {
           if(qservice_ != NULL) {
               qservice_->dispatch(qService::IQService::SET_DPPS_PROP, input_parcel, NULL);
           }
        } break;
        case IMiStcService::ENABLE_STC: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            enable = input_parcel->readInt32();
            for (int i = 0; i < kMaxNumImpl; i++) {
                if (mi_impl_[i]) {
                    mi_impl_[i]->mStcEnable = enable;
                    mi_impl_[i]->needs_update_ = true;
                    STC_LOGD("Setting DisplayId %d Stc enable to %d", i, enable);
                } else {
                    STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                    continue;
                }
            }
        } break;
        case IMiStcService::SET_LOG: {
            log_level = input_parcel->readInt32();
#ifdef EYECARE_V4
            if (mEyeCare[kMiImplPrimary].get())
                mEyeCare[kMiImplPrimary]->SetLogLevel(log_level);
#endif
        } break;
        case IMiStcService::SET_DISPLAY_STATE: {
            disp_id = input_parcel->readInt32();
            state = static_cast<STC_DisplayState>(input_parcel->readInt32());
            if (mi_impl_[disp_id]) {
                mi_impl_[disp_id]->mDispState = state;
                STC_LOGD("Setting DisplayId %d display state to %d", disp_id, state);
            } else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
        } break;
        case IMiStcService::SET_PARAM: {
        } break;
        case IMiStcService::SET_IGC_DITHER: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id]) {
                mi_impl_[disp_id]->enable_list_mi[IGC_DITHER] = enable;
                mi_impl_[disp_id]->igcDitherStrength = input_parcel->readInt32();
                mi_impl_[disp_id]->update_list[IGC_DITHER] = 1;
                mi_impl_[disp_id]->needs_update_ = true;
                STC_LOGE("Seting Display %d dither en %d strength %d", disp_id, enable, mi_impl_[disp_id]->igcDitherStrength);
            } else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
            goto FORCEREFRESH;
        } break;
#ifdef EYECARE_V4
        case IMiStcService::SET_EYECARE: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id]) {
                dimming = input_parcel->readInt32(); // Do not need dimming now.
                int level = input_parcel->readInt32();
                int paperModeId = input_parcel->readInt32();
                ApplyEyeCare(static_cast<MiImplType>(disp_id), enable, level, paperModeId);
            } else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
        } break;
#endif
        case IMiStcService::SET_EXIF: {
            if (!input_parcel) {
                STC_LOGE("MiClstcService command = %d: input_parcel needed.", command);
                break;
            }
            disp_id = input_parcel->readInt32();
            if (mi_impl_[disp_id]) {
                int adrc = input_parcel->readInt32();
                int luxIndex = input_parcel->readInt32();
                if (adrc == 0) {
                    enable = 0;
                } else {
                    enable = 1;
                }
                ApplyPicHDR(static_cast<MiImplType>(disp_id), enable, adrc, luxIndex);
            } else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
        } break;
        case IMiStcService::GET_UPDATE_STATUS: {
            if (!output_parcel) {
                STC_LOGE("MiStcService command = %d: output_parcel needed.", command);
                break;
            }
            bool updateStatus = false;
            for (int i = 0; i < kMaxNumImpl; i++) {
                if (mi_impl_[i]) {
                    if (mi_impl_[i]->mDispState == kStateOn)
                        updateStatus |= mi_impl_[i]->needs_update_;
                }
            }
            output_parcel->writeBool(updateStatus);
            return 0;
        } break;
        case IMiStcService::RESTORE_BL_PCC: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            disp_id = static_cast<uint32_t>(input_parcel->readInt32());
            if (mi_impl_[disp_id])
                mi_impl_[disp_id]->need_restore_pcc = true;
            else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
        } break;
        case IMiStcService::SET_SRE_GC: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            disp_id = static_cast<uint32_t>(input_parcel->readInt32());
            if (mi_impl_[disp_id]) {
                mi_impl_[disp_id]->mSreGcIndexOriginalMode = input_parcel->readInt32();
                mi_impl_[disp_id]->mSreGCCorrectDone = false;
                mi_impl_[disp_id]->mCurDimIndex[GC] = 0;
            } else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
        } break;
        case IMiStcService::STC_DUMP: {
            if (!output_parcel || !input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            disp_id = static_cast<uint32_t>(input_parcel->readInt32());
            std::string result;
            if (mi_impl_[disp_id]) {
                result.append("\nSTC Params:\n");
                mi_impl_[disp_id]->dump(result);
            }
            output_parcel->writeCString(result.c_str());
        } break;
#ifdef EYECARE_V4
        case IMiStcService::SET_EYECARE_PCC: {
            if (!input_parcel) {
                STC_LOGE("MiStcService command = %d: input_parcel needed.", command);
                break;
            }
            if (mi_impl_[disp_id])
                mi_impl_[disp_id]->enable_list_mi[PCC] = enable;
            else {
                STC_LOGE("%s invalid disp id %d", __func__, disp_id);
                return 0;
            }
            if (enable == 0)
                goto FORCEREFRESH;

            std::array<double, kMatrixSize> pccCoeff;
            for (int i = 0; i < kMatrixSize; i++)
                pccCoeff[i] = static_cast<double>(input_parcel->readDouble());
            ApplyEyeCarePcc(static_cast<MiImplType>(disp_id), pccCoeff.begin());
        } break;
#endif
        default:
            STC_LOGI("MiStcService command = %d is not supported.", command);
            break;
    }

FORCEREFRESH:
    if (mi_impl_[disp_id]) {
        mi_impl_[disp_id]->needs_update_ = true;
        mi_impl_[disp_id]->mTimerGeneration[TIMER_REFRESH]++;
        t.AsyncWait(0, timerCallbackFunc, mi_impl_[disp_id], TIMER_REFRESH,
                      mi_impl_[disp_id]->mTimerGeneration[TIMER_REFRESH], 0);
    }
    return 0;
}

void MiStcImpl::dump(std::string& result)
{
    char res_ch[1024] = {0};
    result.append("\nSTC PCC\n");
    snprintf(res_ch, sizeof(res_ch), "%f %f %f %f %f %f %f %f %f %f %f %f",
             current_pcc.r.r, current_pcc.r.g, current_pcc.r.b, current_pcc.r.c,
             current_pcc.g.r, current_pcc.g.g, current_pcc.g.b, current_pcc.g.c,
             current_pcc.b.r, current_pcc.b.g, current_pcc.b.b, current_pcc.b.c);
    result.append(std::string(res_ch));
    result.append("\nSTC IGC\n");
    snprintf(res_ch, sizeof(res_ch), "Enable %d %d\n", mIgcEnable, enable_list_mi[IGC]);
    result.append(std::string(res_ch));
    if (igcData.r.size() == IgcConfig.num_of_entries) {
        for (int i = 0; i < IgcConfig.num_of_entries; i++) {
            snprintf(res_ch, sizeof(res_ch), "[%03d: %03d]", i, igcData.r[i]);
            result.append(std::string(res_ch));
        }
    }
    result.append("\nSTC GC\n");
    snprintf(res_ch, sizeof(res_ch), "Enable %d %d\n", mGcEnable, enable_list_mi[GC]);
    result.append(std::string(res_ch));
    if (gcData.r.size() == GcConfig.num_of_entries) {
        for (int i = 0; i < GcConfig.num_of_entries; i++) {
            snprintf(res_ch, sizeof(res_ch), "[%03d: %03d] ", i, gcData.r[i]);
            result.append(std::string(res_ch));
        }
    }
    result.append("\nSTC GAMUT\n");
    snprintf(res_ch, sizeof(res_ch), "Enable %d %d\n", mLutEnable, enable_list_mi[LUT]);
    result.append(std::string(res_ch));
    for (int i = 0; i < LUT3D_ENTRIES_SIZE; i+=20) {
        if (mEyeCareEnable) {
            if (mEyeCareLut.size() == LUT3D_ENTRIES_SIZE)
                snprintf(res_ch, sizeof(res_ch), "[%d: %04d %04d %04d] ", i, mEyeCareLut[i].out.r, mEyeCareLut[i].out.g, mEyeCareLut[i].out.b);
        } else {
            if (mCurLutInfo.size() == LUT3D_ENTRIES_SIZE)
                snprintf(res_ch, sizeof(res_ch), "[%d: %04d %04d %04d] ", i, mCurLutInfo[i].out.r, mCurLutInfo[i].out.g, mCurLutInfo[i].out.b);
        }
        result.append(std::string(res_ch));
    }
    if (mEyeCareEnable) {
        if (mEyeCareLut.size() == LUT3D_ENTRIES_SIZE)
            snprintf(res_ch, sizeof(res_ch), "[4912: %04d %04d %04d]\n", mEyeCareLut[4912].out.r, mEyeCareLut[4912].out.g, mEyeCareLut[4912].out.b);
    } else {
        if (mCurLutInfo.size() == LUT3D_ENTRIES_SIZE)
            snprintf(res_ch, sizeof(res_ch), "[4912: %04d %04d %04d]\n", mCurLutInfo[4912].out.r, mCurLutInfo[4912].out.g, mCurLutInfo[4912].out.b);
    }
    result.append(std::string(res_ch));
    result.append("\nSTC PA\n");
    snprintf(res_ch, sizeof(res_ch), "Enable %d %d, %d %f %f %f\n", mPaEnable, enable_list_mi[PA], mPaData.hue, mPaData.saturation, mPaData.value, mPaData.contrast);
    result.append(std::string(res_ch));
}

void MiStcImpl::ApplyPCC(MiImplType disp_type, double* coeff)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
        impl->SetPCC(coeff);
    }
}

void MiStcImpl::SetPCC(double* coeff)
{
    mi_pcc.r.r = coeff[0];
    mi_pcc.r.g = coeff[1];
    mi_pcc.r.b = coeff[2];
    mi_pcc.r.c = coeff[3];
    mi_pcc.g.r = coeff[4];
    mi_pcc.g.g = coeff[5];
    mi_pcc.g.b = coeff[6];
    mi_pcc.g.c = coeff[7];
    mi_pcc.b.r = coeff[8];
    mi_pcc.b.g = coeff[9];
    mi_pcc.b.b = coeff[10];
    mi_pcc.b.c = coeff[11];
    needs_update_ = true;
    update_list[PCC] = 1;
}

void MiStcImpl::ApplyPA(MiImplType disp_type, disp_pa_config_data cfg, int dimming)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
        impl->SetPA(cfg, dimming);
    }
}

void MiStcImpl::SetPA(disp_pa_config_data cfg, int dimming)
{
    Timer t;

    Mutex::Autolock autoLock(mPaDimmingLock);
    if (mPaEnable == 0) {
        mTargetPaData = mNativePaData;
    } else {
        enable_list_mi[PA] = mPaEnable;
        mTargetPaData = cfg;
    }
    if (!dimming) {
        mPaData = mTargetPaData;
        mSrcPaData = mTargetPaData;
        mTimerGeneration[TIMER_PA] = mDimStep[PA] + 1;
        return;
    }
    mCurDimIndex[PA] = 0;
    if (mTimerGeneration[TIMER_SYNC] == 0) {
        t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
    }
}

void MiStcImpl::ApplyIGC(MiImplType disp_type, int gamma, int file_id, int dimming)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
      impl->SetIGC(gamma, file_id, dimming);
    }
}

void MiStcImpl::SetIGC(int gamma, int file_id, int dimming)
{
    float targetGamma = 2.2;
    int numOfEntries = 257;
    int entriesWidth = 12;
    int paramMax = 4095;
    std::vector<int> iEntries;
    float temp;
    int data;
    Timer t;

    Mutex::Autolock autoLock(mIGCDimmingLock);
    if (mIgcEnable == 0) {
        targetIgcData = nativeIgcData;
        //mTimerGeneration[TIMER_IGC]++;
        mCurDimIndex[IGC] = 0;
        mDimStep[IGC] = dimming;
        if (mTimerGeneration[TIMER_SYNC] == 0) {
            t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
        }
        return;
    } else {
        enable_list_mi[IGC] = mIgcEnable;
    }

    if (gamma != 0)
        targetGamma = (double)gamma/100.0;
    if (IgcConfig.num_of_entries != 0) {
        numOfEntries = IgcConfig.num_of_entries;
        entriesWidth = IgcConfig.entries_width;
    }
    if (targetIgcData.r.size() > 0) {
        targetIgcData.r.clear();
        targetIgcData.g.clear();
        targetIgcData.b.clear();
    }

    if (gamma == 0) {
        if (mIgcFileCnt == 0 || file_id >= mIgcFileCnt)
            InitGamma(mDispId);

        if (mIgcFileCnt && file_id <= mIgcFileCnt - 1) {
            targetIgcData.r.assign(igcDataBuf.r.begin() + file_id * numOfEntries,
                igcDataBuf.r.begin() + (file_id + 1) * numOfEntries);
            targetIgcData.g.assign(igcDataBuf.g.begin() + file_id * numOfEntries,
                igcDataBuf.g.begin() + (file_id + 1) * numOfEntries);
            targetIgcData.b.assign(igcDataBuf.b.begin() + file_id * numOfEntries,
                igcDataBuf.b.begin() + (file_id + 1) * numOfEntries);
        }
    } else {
        paramMax = pow(2, entriesWidth) - 1;
        for (int i = 0; i < numOfEntries; i++) {
            temp = ((double)i)/(numOfEntries - 1);
            temp = pow(temp, targetGamma);
            data = (int)(temp * paramMax + 0.5);
            targetIgcData.r.push_back(data);
            targetIgcData.g.push_back(data);
            targetIgcData.b.push_back(data);
        }
    }
    if (!dimming) {
        update_list[IGC] = 0;
        igcData = targetIgcData;
        srcIgcData = targetIgcData;
        update_list[IGC] = 1;
        mTimerGeneration[TIMER_IGC] = mDimStep[IGC] + 1;
        igcDataBackup = igcData;
        return;
    } else {
        mDimStep[IGC] = dimming;
    }
    //mTimerGeneration[TIMER_IGC]++;
    mCurDimIndex[IGC] = 0;
    if (mTimerGeneration[TIMER_SYNC] == 0) {
        t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
    }
}

void MiStcImpl::ApplySREIGC(MiImplType disp_type, std::vector<int32_t> tempData, int dimming)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
      impl->SetSREIGC(tempData, dimming);
    }
}

void MiStcImpl::SetSREIGC(std::vector<int32_t> tempData, int dimming)
{
    Timer t;
    Mutex::Autolock autoLock(mIGCDimmingLock);
    if (mIgcEnable == 0) {
        targetIgcData = nativeIgcData;
        mCurDimIndex[IGC] = 0;
        if (mTimerGeneration[TIMER_SYNC] == 0) {
            t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
        }
        return;
    } else {
        enable_list_mi[IGC] = mIgcEnable;
    }

    //update_list[IGC] = 0;
    if (targetIgcData.r.size() > 0) {
        targetIgcData.r.clear();
        targetIgcData.g.clear();
        targetIgcData.b.clear();
    }
    targetIgcData.r.assign(tempData.begin(), tempData.end());
    targetIgcData.g.assign(tempData.begin(), tempData.end());
    targetIgcData.b.assign(tempData.begin(), tempData.end());

    if (!dimming) {
        update_list[IGC] = 0;
        igcData= targetIgcData;
        srcIgcData = targetIgcData;
        update_list[IGC] = 1;
        mTimerGeneration[TIMER_IGC] = mDimStep[IGC] + 1;
        igcDataBackup = igcData;
        return;
    } else {
        mDimStep[IGC] = dimming;
    }

    mCurDimIndex[IGC] = 0;
    if (mTimerGeneration[TIMER_SYNC] == 0) {
        t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
    }
}

void MiStcImpl::ApplyGC(MiImplType disp_type, int gamma, int file_id, int dimming)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
        impl->SetGC(gamma, file_id, dimming);
    }
}

void MiStcImpl::SetGC(int gamma, int file_id, int dimming)
{
    double targetGamma = 2.2;
    int numOfEntries = 1024;
    int entriesWidth = 10;
    int paramMax = 1023;
    std::vector<int> entries;
    double temp;
    int data;
    FILE *fp = NULL;
    int cnt = 0;
    char path[256];
    int gammaData;
    Timer t;

    Mutex::Autolock autoLock(mGCDimmingLock);
    if (mGcEnable == 0) {
        targetGcData = nativeGcData;
        mCurDimIndex[GC] = 0;
        if (mTimerGeneration[TIMER_SYNC] == 0) {
            t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
        }
        return;
    } else {
        enable_list_mi[GC] = mGcEnable;
    }

    if (gamma != 0)
        targetGamma = (double)gamma/100.0;
    if (GcConfig.num_of_entries != 0) {
        numOfEntries = GcConfig.num_of_entries;
        entriesWidth = GcConfig.entries_width;
    }
    if (targetGcData.r.size() > 0) {
        targetGcData.r.clear();
        targetGcData.g.clear();
        targetGcData.b.clear();
    }

    if (gamma == 0) {
        if (mGcFileCnt == 0 || file_id >= mGcFileCnt)
            InitGamma(mDispId);

        if (mGcFileCnt && file_id <= mGcFileCnt - 1) {
            targetGcData.r.assign(gcDataBuf.r.begin() + file_id * numOfEntries,
                gcDataBuf.r.begin() + (file_id + 1) * numOfEntries);
            targetGcData.g.assign(gcDataBuf.g.begin() + file_id * numOfEntries,
                gcDataBuf.g.begin() + (file_id + 1) * numOfEntries);
            targetGcData.b.assign(gcDataBuf.b.begin() + file_id * numOfEntries,
                gcDataBuf.b.begin() + (file_id + 1) * numOfEntries);
        }
    } else {
        paramMax = pow(2, entriesWidth) - 1;
        for (int i = 0; i < numOfEntries; i++) {
            temp = ((double)i)/(numOfEntries - 1);
            temp = pow(temp, 1/targetGamma);
            data = (int)(temp * paramMax + 0.5);
            targetGcData.r.push_back(data);
            targetGcData.g.push_back(data);
            targetGcData.b.push_back(data);
        }
    }
    if (!dimming && !mSreEnable) {
        update_list[GC] = 0;
        gcData = targetGcData;
        srcGcData = targetGcData;
        update_list[GC] = 1;
        mTimerGeneration[TIMER_GC] = mDimStep[GC] + 1;
        gcDataBackup = gcData;
        return;
    }
    //mTimerGeneration[TIMER_GC]++;
    mCurDimIndex[GC] = 0;
    if (mTimerGeneration[TIMER_SYNC] == 0) {
        t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
    }
}

void MiStcImpl::ApplyLut(MiImplType disp_type, int reInit, int lutIndex, int dimming)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
        impl->SetLut(reInit, lutIndex, dimming);
    }
}

void MiStcImpl::SetLut(int reInit, int lutIndex, int dimming)
{
    Timer t;
    Mutex::Autolock autoLock(mLutDimmingLock);
    if (mSreEnable)
        mSreUpdate = 1;
    if (mLutEnable == 0) {
        if (dimming) {
            mTargetLutInfo.assign(mNativeLutInfo.begin(), mNativeLutInfo.end());

            if (mCurLutInfo.size() == 0)
                mCurLutInfo.assign(mNativeLutInfo.begin(), mNativeLutInfo.end());
            mCurDimIndex[LUT] = 0;
            if (mTimerGeneration[TIMER_SYNC] == 0) {
                t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
            }
        } else {
            mTargetLutInfo = mNativeLutInfo;
            mCurLutInfo = mNativeLutInfo;
            mSrcLutInfo = mNativeLutInfo;
            if (mEyeCareEnable) {
                AssignEyeCare();
                AssignLut();
            }
            if (mPicHDREnable) {
                AssignPicHDR();
                AssignPicLut();
            }
            enable_list_mi[LUT] = mLutEnable;
        }
        return;
    } else {
        enable_list_mi[LUT] = mLutEnable;
    }

    if (reInit) {
        initLut(mDispId);
    }
    if (lutIndex > -1 && lutIndex < LutFileNum) {
        mLutIndex = lutIndex;
        mTargetLutInfo.assign(&LutInfo[LUT3D_ENTRIES_SIZE * lutIndex],
            &LutInfo[LUT3D_ENTRIES_SIZE * (lutIndex + 1)]);

        if (mCurLutInfo.size() == 0)
            mCurLutInfo.assign(mNativeLutInfo.begin(), mNativeLutInfo.end());
    } else {
        STC_LOGE("Invalid lut index %d",lutIndex);
        return;
    }
    if (!dimming) {
        update_list[LUT] = 0;
        mCurLutInfo.assign(mTargetLutInfo.begin(), mTargetLutInfo.end());
        mSrcLutInfo.assign(mTargetLutInfo.begin(), mTargetLutInfo.end());
        if (mEyeCareEnable) {
            AssignEyeCare();
            AssignLut();
        }
        if (mPicHDREnable) {
            AssignPicHDR();
            AssignPicLut();
        }
        mTimerGeneration[TIMER_LUT] = mDimStep[LUT] + 1;
        update_list[LUT] = 1;
        mLutInfoBackup.assign(mTargetLutInfo.begin(), mTargetLutInfo.end());
        return;
    }
    mCurDimIndex[LUT] = 0;
    if (mTimerGeneration[TIMER_SYNC] == 0) {
        t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
    }
    STC_LOGD("Applying lut %d", lutIndex);
}

void MiStcImpl::ApplySRE(MiImplType disp_type, int level, int divisor, int dimming)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
        impl->SetSRE(level, divisor, dimming);
    }
}

void MiStcImpl::SetSRE(int level, int divisor, int dimming)
{
    Timer t;
    Mutex::Autolock autoLock(mLutDimmingLock);
    if (mSreEnable) {
        mSreUpdate = 1;
    } else {
        mSreClosing = 1;
    }
    for (int i = 0; i < 256; i++) {
        mSreLut[i] = SreLutLevel[0][i] + (SreLutLevel[level][i] - SreLutLevel[0][i]) * divisor / 15;
    }
    mCurDimIndex[LUT] = 0;
    mCurDimIndex[GC] = 0;
    enable_list_mi[GC] = 1;
    mDimStep[LUT] = dimming;
    mDimStep[GC] = dimming;
    if (mTimerGeneration[TIMER_SYNC] == 0) {
        t.AsyncWait(0, timerCallbackFunc, this, TIMER_SYNC, 0,0);
    }
}

void MiStcImpl::AssignPicHDR()
{
    std::vector<rgb_entry> target;
    if (mEyeCareEnable) {
        if (mEyeCareLut.size() != LUT3D_ENTRIES_SIZE) {
              STC_LOGE("Invalid eyecare lut size %d", mEyeCareLut.size());
             return;
          }
        target.assign(mEyeCareLut.begin(), mEyeCareLut.end());
    } else {
        if (mTargetLutInfo.size() != LUT3D_ENTRIES_SIZE) {
                STC_LOGE("Invalid target lut size %d", mTargetLutInfo.size());
                return;
        }
        target.assign(mTargetLutInfo.begin(), mTargetLutInfo.end());
    }
    if (target.size() == LUT3D_ENTRIES_SIZE) {
        mPicHDRTargetLut.assign(target.begin(), target.end());
        // for (uint32_t i = 0;  i < LUT3D_ENTRIES_SIZE; i++ ) {
        //     STC_LOGI("mPicHDRTargetLut before HDR: MiStcService AssignPicHDR,Before HDR:  LUT[%d]: in[%d %d %d], out[%d %d %d]", i, mPicHDRTargetLut[i].in.r, mPicHDRTargetLut[i].in.g ,mPicHDRTargetLut[i].in.b, mPicHDRTargetLut[i].out.r, mPicHDRTargetLut[i].out.g, mPicHDRTargetLut[i].out.b);
        // }
        PicHDRProcess(&mPicHDRTargetLut[0]);
        // for (uint32_t i = 0;  i < LUT3D_ENTRIES_SIZE; i++ ) {
        //     STC_LOGI("mPicHDRTargetLut after hdr : MiStcService AssignPicHDR,After HDR:  LUT[%d]: in[%d %d %d], out[%d %d %d]", i, mPicHDRTargetLut[i].in.r, mPicHDRTargetLut[i].in.g ,mPicHDRTargetLut[i].in.b, mPicHDRTargetLut[i].out.r, mPicHDRTargetLut[i].out.g, mPicHDRTargetLut[i].out.b);
        // }
    }
}

void MiStcImpl::AssignPicLut()
{
    if (mCurDimIndex[LUT] > mDimStep[LUT]) {
        update_list[LUT] = 0;
        mPicHDRLut.assign(mPicHDRTargetLut.begin(), mPicHDRTargetLut.end());
        mCurLutInfo.assign(mPicHDRTargetLut.begin(), mPicHDRTargetLut.end());
        update_list[LUT] = 1;
        mPicHDRLutBackup.assign(mPicHDRLut.begin(), mPicHDRLut.end());
        mLutInfoBackup.assign(mCurLutInfo.begin(), mCurLutInfo.end());
    }
}

void MiStcImpl::ApplyPicHDR(MiImplType disp_type, int enable, int adrc, int luxIndex)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
        impl->SetPicHDR(enable, adrc, luxIndex);
    }
}

void MiStcImpl::SetPicHDR(int enable,int adrc, int luxIndex)
{
    if (mPicHDRAlgo[mDispId].get() && enable) {
        mPicHDRAlgo[mDispId]->SetExif(static_cast<double>(adrc), static_cast<double>(luxIndex));
        {
            Mutex::Autolock autoLock(mLutDimmingLock);
            AssignPicHDR();
            AssignPicLut();
        }
    }
    mPicHDREnable = enable;
}

void MiStcImpl::AssignEyeCare()
{
    std::vector<rgb_entry> target;
    if (mSreEnable) {
        if (mSRELutInfo.size() != LUT3D_ENTRIES_SIZE) {
            STC_LOGE("Invalid sre lut size %d", mSRELutInfo.size());
            return;
        }
        target.assign(mSRELutInfo.begin(), mSRELutInfo.end());
    } else if (mPicHDREnable) {
        if (mPicHDRLut.size() != LUT3D_ENTRIES_SIZE) {
            STC_LOGE("Invalid pichdr lut size %d", mPicHDRLut.size());
            return;
        }
        target.assign(mPicHDRLut.begin(), mPicHDRLut.end());
    }
     else {
        if (mTargetLutInfo.size() != LUT3D_ENTRIES_SIZE) {
            STC_LOGE("Invalid target lut size %d", mTargetLutInfo.size());
            return;
        }
        target.assign(mTargetLutInfo.begin(), mTargetLutInfo.end());
    }
    if (target.size() == LUT3D_ENTRIES_SIZE) {
        mEyeCareTargetLut.assign(target.begin(), target.end());
        EyeCareProcess(&mEyeCareTargetLut[0]);
    }
}
void MiStcImpl::AssignLut()
{
    if (mCurDimIndex[LUT] > mDimStep[LUT]) {
        update_list[LUT] = 0;
        mEyeCareLut.assign(mEyeCareTargetLut.begin(), mEyeCareTargetLut.end());
        mCurLutInfo.assign(mEyeCareTargetLut.begin(), mEyeCareTargetLut.end());
        update_list[LUT] = 1;
        mEyeCareLutBackup.assign(mEyeCareLut.begin(), mEyeCareLut.end());
        mLutInfoBackup.assign(mCurLutInfo.begin(), mCurLutInfo.end());
    }
}

#ifdef EYECARE_V4
void MiStcImpl::ApplyEyeCare(MiImplType disp_type, int enable, int level, int paperModeId)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
        impl->SetEyeCare(enable, level, paperModeId);
    }
}

void MiStcImpl::SetEyeCare(int enable, int level, int paperModeId)
{
    if (mEyeCare[mDispId].get())
        mEyeCare[mDispId]->SetPaperModeId(paperModeId);

    if (level == mEyeCareLevel && paperModeId == mEyeCarePaperId) {
        mEyeCareEnable = enable;
        return;
    }
    mEyeCareLevel = level;
    mEyeCarePaperId = paperModeId;
    {
        Mutex::Autolock autoLock(mLutDimmingLock);
        AssignEyeCare();
        AssignLut();
    }
    mEyeCareEnable = enable;
}

void MiStcImpl::ApplyEyeCarePcc(MiImplType disp_type, double* coeff)
{
    MiStcImpl *impl = mi_impl_[disp_type];
    if (impl) {
        impl->SetEyeCarePcc(coeff);
    }
}

void MiStcImpl::SetEyeCarePcc(double* coeff)
{
    mi_eyecare_pcc.r.r = coeff[0];
    mi_eyecare_pcc.r.g = coeff[1];
    mi_eyecare_pcc.r.b = coeff[2];
    mi_eyecare_pcc.r.c = coeff[3];
    mi_eyecare_pcc.g.r = coeff[4];
    mi_eyecare_pcc.g.g = coeff[5];
    mi_eyecare_pcc.g.b = coeff[6];
    mi_eyecare_pcc.g.c = coeff[7];
    mi_eyecare_pcc.b.r = coeff[8];
    mi_eyecare_pcc.b.g = coeff[9];
    mi_eyecare_pcc.b.b = coeff[10];
    mi_eyecare_pcc.b.c = coeff[11];

    needs_update_ = true;
    update_list[PCC] = 1;
}
#endif

int MiStcImpl::forceScreenRefresh()
{
    int ret = 0;
    Timer t;
    if (mDispState != kStateOn) {
        STC_LOGW("Forbidden refresh screen in state %d", mDispState);
        return ret;
    }
    Parcel inParcel, outParcel;
    inParcel.writeInt32(1);
    inParcel.setDataPosition(0);
    if(qservice_ != NULL && mWaitRefresh == 0) {
        int err = qservice_->dispatch(qService::IQService::TOGGLE_SCREEN_UPDATES, &inParcel , &outParcel);
        if (err) {
            STC_LOGE("Failed to dispatch screen refresh");
            ret = -1;
        } else {
            mWaitRefresh = 1;
            mTimerGeneration[TIMER_REFRESH_TIMEOUT]++;
            t.AsyncWait(20, timerCallbackFunc, this,
                        TIMER_REFRESH_TIMEOUT, mTimerGeneration[TIMER_REFRESH_TIMEOUT], 0);
        }
        STC_LOGV("%s id %d", __func__, mDispId);
    }
    return ret;
}

void MiStcImpl::sreProcess()
{
    int max;
    int min;
    int R, G, B;
    double h,s,f;
    HSV_type hsv;
    int H, a, b, c;

    mSRELutInfo.assign(mTargetLutInfo.begin(), mTargetLutInfo.end());
    for (int i = 0; i < LUT3D_ENTRIES_SIZE; i++) {
// RGB to HSV
        R = mSRELutInfo[i].out.r/16;
        G = mSRELutInfo[i].out.g/16;
        B = mSRELutInfo[i].out.b/16;
        R = R > 255 ? 255 : R;
        G = G > 255 ? 255 : G;
        B = B > 255 ? 255 : B;
        max = Max(R, G, B);
        min = Min(R, G, B);
        if (max == min) {
            hsv.h = 0;
        } else {
            if (R == max) {
                hsv.h = (double)(G - B) / (max - min);
            } else if (G == max) {
                hsv.h = 2 + (double)(B - R) / (max - min);
            } else {
                hsv.h = 4 + (double)(R - G) / (max - min);
            }
        }
        hsv.h *= 60;
        hsv.h = hsv.h < 0 ? hsv.h + 360 : hsv.h;
        hsv.s = (double)(max - min) / max;
        hsv.v = mSreLut[max]; //SRE Process, V mapping.
// HSV to RGB
        if (hsv.s == 0) {
            R = hsv.v;
            G = hsv.v;
            B = hsv.v;
        } else {
            hsv.h /= 60;
            H = (int) hsv.h;
            f = hsv.h - H;
            a = (int) hsv.v * (1 - hsv.s);
            b = (int) hsv.v * (1 - hsv.s * f);
            c = (int) hsv.v * (1 - hsv.s * (1 - f));
            switch (H) {
                case 0: {
                    R = hsv.v;
                    G = c;
                    B = a;
                } break;
                case 1: {
                    R = b;
                    G = hsv.v;
                    B = a;
                } break;
                case 2: {
                    R = a;
                    G = hsv.v;
                    B = c;
                } break;
                case 3: {
                    R = a;
                    G = b;
                    B = hsv.v;
                } break;
                case 4: {
                    R = c;
                    G = a;
                    B = hsv.v;
                } break;
                case 5: {
                    R = hsv.v;
                    G = a;
                    B = b;
                } break;
                default:
                    break;
            }
        }
        mSRELutInfo[i].out.r = R * 16;
        mSRELutInfo[i].out.g = G * 16;
        mSRELutInfo[i].out.b = B * 16;
    }
}

void MiStcImpl::timerCallback(int caseId, int generation, int value)
{
    Timer t;
    uint32_t numOfEntries = 1024;
    uint32_t entriesWidth = 10;
    uint32_t temp = 0;
    STC_LOGV("%s caseId %d, generation %d, mTimerGeneration %d",
                 __func__, caseId, generation, mTimerGeneration[caseId]);
    if (generation != mTimerGeneration[caseId]) {
        return;
    }
    switch (caseId) {
        case TIMER_GC: {
            if (abs(value - mCurDimIndex[GC]) > DIMMING_MAX_THREAD_CNT) {
                ALOGE("Too many gc threads wait, skip %d, cur index %d", value, mCurDimIndex[GC]);
                break;
            }
            Mutex::Autolock autoLock(mGCDimmingLock);
            Gamma_type target;
            STC_LOGV("%s TIMER GC value %d, curIndex %d", __func__, value, mCurDimIndex[GC]);
            if (mSreEnable) {
                if (mSreGCCorrectDone)
                    return;
                if (mSreGcIndex != -1 || mSreGcIndexOriginalMode != -1) {
                    int sreGcIndex = -1;
                    if (mSreGcIndexOriginalMode != -1)
                        sreGcIndex = mSreGcIndexOriginalMode;
                    else
                        sreGcIndex = mSreGcIndex;

                    if (gcDataBuf.r.size() < GcConfig.num_of_entries * (sreGcIndex + 1)) {
                        STC_LOGE("Invalid gcDataBuf!");
                        mCurDimIndex[GC] = mDimStep[GC] + 1;
                        return;
                    }
                    target.r.assign(gcDataBuf.r.begin() + sreGcIndex * GcConfig.num_of_entries,
                        gcDataBuf.r.begin() + (sreGcIndex + 1) * GcConfig.num_of_entries);
                    target.g.assign(gcDataBuf.g.begin() + sreGcIndex * GcConfig.num_of_entries,
                        gcDataBuf.g.begin() + (sreGcIndex + 1) * GcConfig.num_of_entries);
                    target.b.assign(gcDataBuf.b.begin() + sreGcIndex * GcConfig.num_of_entries,
                        gcDataBuf.b.begin() + (sreGcIndex + 1) * GcConfig.num_of_entries);
                } else {
                    target = targetGcData;
                }
            } else {
                mSreGCCorrectDone = false;
                target = targetGcData;
            }
            if (target.r.size() != GcConfig.num_of_entries) {
                STC_LOGE("Invalid target gc data size %d", target.r.size());
                mCurDimIndex[GC] = mDimStep[GC] + 1;
                break;
            }

            if (value != mCurDimIndex[GC])
                return;

            update_list[GC] = 0;
            if (value == 0 || mTargetChanged[GC]) {
                srcGcData = gcData;
                if (mTargetChanged[GC]) {
                    mCurDimIndex[GC] = 0;
                    mTargetChanged[GC] = false;
                }
            }
            mCurDimIndex[GC]++;
            if (mCurDimIndex[GC] > mDimStep[GC]) {
                update_list[GC] = 1;
                srcGcData = target;
                if (mSreEnable) {
                    mSreGCCorrectDone = true;
                }
                if (mGcEnable == 0) {
                    enable_list_mi[GC] = 0;
                    t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
                    needs_update_ = true;
                }
                mDimStep[GC] = 20;
                return;
            }
            if (GcConfig.num_of_entries != 0) {
                numOfEntries = GcConfig.num_of_entries;
                entriesWidth = GcConfig.entries_width;
            }
            gcData.r.clear();
            gcData.g.clear();
            gcData.b.clear();

            if (srcGcData.r.size() != numOfEntries) {
                STC_LOGE("GC dimming, invalid src size %d", srcGcData.r.size());
                srcGcData = target;
            }
            for (int i = 0; i < numOfEntries; i++) {
                temp = srcGcData.r[i] + mCurDimIndex[GC]
                        * (int)(target.r[i] - srcGcData.r[i]) / mDimStep[GC];
                gcData.r.push_back(temp);
                temp = srcGcData.g[i] + mCurDimIndex[GC]
                        * (int)(target.g[i] - srcGcData.g[i]) / mDimStep[GC];
                gcData.g.push_back(temp);
                temp = srcGcData.b[i] + mCurDimIndex[GC]
                        * (int)(target.b[i] - srcGcData.b[i]) / mDimStep[GC];
                gcData.b.push_back(temp);
            }
            update_list[GC] = 1;
            enable_list_mi[GC] = 1;
            gcDataBackup = gcData;
            needs_update_ = true;
            mTimerGeneration[TIMER_REFRESH]++;
            t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
        } break;
        case TIMER_PA: {
            if (abs(value - mCurDimIndex[PA]) > DIMMING_MAX_THREAD_CNT) {
                ALOGE("Too many pa threads wait, skip %d, cur index %d", value, mCurDimIndex[PA]);
                break;
            }
            Mutex::Autolock autoLock(mPaDimmingLock);
            STC_LOGV("%s TIMER PA value %d, curIndex %d", __func__, value, mCurDimIndex[PA]);
            update_list[PA] = 0;
            if (value != mCurDimIndex[PA])
                return;
            if (value == 0 || mTargetChanged[PA]) {
                mSrcPaData = mPaData;
                if (mTargetChanged[PA]) {
                    mCurDimIndex[PA] = 0;
                    mTargetChanged[PA] = false;
                }
            }
            mCurDimIndex[PA]++;
            if (mCurDimIndex[PA] > mDimStep[PA]) {
                update_list[PA] = 1;
                mSrcPaData = mTargetPaData;
                if (mPaEnable == 0) {
                    enable_list_mi[PA] = 0;
                    t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
                    needs_update_ = true;
                }
                return;
            }

            mPaData.hue = mSrcPaData.hue + mCurDimIndex[PA]
                * (mTargetPaData.hue - mSrcPaData.hue) / mDimStep[PA];
            mPaData.saturation = mSrcPaData.saturation + mCurDimIndex[PA]
                * (mTargetPaData.saturation - mSrcPaData.saturation) / mDimStep[PA];
            mPaData.value = mSrcPaData.value + mCurDimIndex[PA]
                * (mTargetPaData.value - mSrcPaData.value) / mDimStep[PA];
            mPaData.contrast = mSrcPaData.contrast + mCurDimIndex[PA]
                * (mTargetPaData.contrast - mSrcPaData.contrast) / mDimStep[PA];
            mPaData.sat_thresh = mSrcPaData.sat_thresh + mCurDimIndex[PA]
                * (mTargetPaData.sat_thresh - mSrcPaData.sat_thresh) / mDimStep[PA];

            mTimerGeneration[TIMER_PA]++;
            t.AsyncWait(mDimTimeMs, timerCallbackFunc, this, TIMER_PA, mTimerGeneration[TIMER_PA], mCurDimIndex[PA]);
            update_list[PA] = 1;
            enable_list_mi[PA] = 1;
            needs_update_ = true;
            mTimerGeneration[TIMER_REFRESH]++;
            t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
        } break;
        case TIMER_LUT: {
            if (abs(value - mCurDimIndex[LUT]) > DIMMING_MAX_THREAD_CNT) {
                ALOGE("Too many lut threads wait, skip %d, cur index %d", value, mCurDimIndex[LUT]);
                break;
            }
            Mutex::Autolock autoLock(mLutDimmingLock);
            struct rgb_entry lutInfo;
            std::vector<rgb_entry> target;
            STC_LOGV("%s TIMER LUT value %d, curIndex %d", __func__, value, mCurDimIndex[LUT]);
            if (mTargetLutInfo.size() != LUT3D_ENTRIES_SIZE) {
                STC_LOGE("Invalid target lut data size %d", mTargetLutInfo.size());
                mCurDimIndex[LUT] = mDimStep[LUT];
                break;
            }
            if (value != mCurDimIndex[LUT]) {
                return;
            }
            update_list[LUT] = 0;
            if (mSreUpdate) {
                sreProcess();
                mSreUpdate = 0;
            }
            if (value == 0 || mTargetChanged[LUT]) {
                mSrcLutInfo.assign(mCurLutInfo.begin(), mCurLutInfo.end());
                if (mTargetChanged[LUT]) {
                    mCurDimIndex[LUT] = 0;
                    mTargetChanged[LUT] = false;
                }
                if (mEyeCareEnable) {
                    AssignEyeCare();
                }
                if (mPicHDREnable) {
                    AssignPicHDR();
                }
            }
            mCurDimIndex[LUT]++;
            if (mCurDimIndex[LUT] > mDimStep[LUT]) {
                update_list[LUT] = 1;
                mSrcLutInfo.assign(mTargetLutInfo.begin(), mTargetLutInfo.end());
                if (mLutEnable == 0 && mSreEnable == 0) {
                    enable_list_mi[LUT] = 0;
                    mTimerGeneration[TIMER_REFRESH]++;
                    t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
                    needs_update_ = true;
                }
                mDimStep[LUT] = 20;
                mSreClosing = 0;
                return;
            }
            mCurLutInfo.clear();
            if (mEyeCareEnable) {
                if (mEyeCareTargetLut.size() != LUT3D_ENTRIES_SIZE) {
                    STC_LOGE("Invalid sre lut size %d", mSRELutInfo.size());
                    mCurDimIndex[LUT] = mDimStep[LUT] + 1;
                    update_list[LUT] = 1;
                    return;
                }
                target.assign(mEyeCareTargetLut.begin(), mEyeCareTargetLut.end());
            }
            else if (mPicHDREnable) {
                if (mPicHDRTargetLut.size() != LUT3D_ENTRIES_SIZE) {
                    STC_LOGE("Invalid pichdr lut size %d", mPicHDRTargetLut.size());
                    mCurDimIndex[LUT] = mDimStep[LUT] + 1;
                    update_list[LUT] = 1;
                    return;
                }
                target.assign(mPicHDRTargetLut.begin(), mPicHDRTargetLut.end());
            }
            else {
                if (mSreEnable) {
                    if (mSRELutInfo.size() != LUT3D_ENTRIES_SIZE) {
                        STC_LOGE("Invalid sre lut size %d", mSRELutInfo.size());
                        mCurDimIndex[LUT] = mDimStep[LUT] + 1;
                        update_list[LUT] = 1;
                        return;
                    }
                    target.assign(mSRELutInfo.begin(), mSRELutInfo.end());
                } else {
                    if (mTargetLutInfo.size() != LUT3D_ENTRIES_SIZE) {
                        STC_LOGE("Invalid target lut size %d", mTargetLutInfo.size());
                        mCurDimIndex[LUT] = mDimStep[LUT] + 1;
                        update_list[LUT] = 1;
                        return;
                    }
                    target.assign(mTargetLutInfo.begin(), mTargetLutInfo.end());
                }
            }

            if (mSrcLutInfo.size() != LUT3D_ENTRIES_SIZE) {
                STC_LOGE("Invalid src lut size %d", mSrcLutInfo.size());
                mSrcLutInfo.assign(target.begin(), target.end());
            }
            for (int i = 0; i < LUT3D_ENTRIES_SIZE; i++) {
                lutInfo.in = target[i].in;
                lutInfo.out.r = mSrcLutInfo[i].out.r + mCurDimIndex[LUT]
                        * (int)(target[i].out.r - mSrcLutInfo[i].out.r) /mDimStep[LUT];
                lutInfo.out.g = mSrcLutInfo[i].out.g + mCurDimIndex[LUT]
                        * (int)(target[i].out.g - mSrcLutInfo[i].out.g) /mDimStep[LUT];
                lutInfo.out.b = mSrcLutInfo[i].out.b + mCurDimIndex[LUT]
                        * (int)(target[i].out.b - mSrcLutInfo[i].out.b) /mDimStep[LUT];
                mCurLutInfo.push_back(lutInfo);
            }
            if (mEyeCareEnable) {
                mEyeCareLut.assign(mCurLutInfo.begin(), mCurLutInfo.end());
            }
            if (mPicHDREnable) {
                mPicHDRLut.assign(mCurLutInfo.begin(), mCurLutInfo.end());
            }
            mTimerGeneration[TIMER_LUT]++;
            t.AsyncWait(mDimTimeMs, timerCallbackFunc, this, TIMER_LUT, mTimerGeneration[TIMER_LUT], mCurDimIndex[LUT]);
            update_list[LUT] = 1;
            enable_list_mi[LUT] = 1;
            mLutInfoBackup.clear();
            mLutInfoBackup.assign(mCurLutInfo.begin(), mCurLutInfo.end());
            if (mEyeCareEnable)
                mEyeCareLutBackup.assign(mEyeCareLut.begin(), mEyeCareLut.end());
            if (mPicHDREnable)
                mPicHDRLutBackup.assign(mPicHDRLut.begin(), mPicHDRLut.end());
            needs_update_ = true;
            mTimerGeneration[TIMER_REFRESH]++;
            t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
        } break;
        case TIMER_IGC: {
            if (abs(value - mCurDimIndex[IGC]) > DIMMING_MAX_THREAD_CNT) {
                ALOGE("Too many igc threads wait, skip %d, cur index %d", value, mCurDimIndex[IGC]);
                break;
            }
            Mutex::Autolock autoLock(mIGCDimmingLock);
            STC_LOGV("%s TIMER IGC value %d, curIndex %d", __func__, value, mCurDimIndex[IGC]);
            if (targetIgcData.r.size() != IgcConfig.num_of_entries) {
                STC_LOGE("Invalid target igc data size %d", targetIgcData.r.size());
                mCurDimIndex[IGC] = mDimStep[IGC] + 1;
                break;
            }
            if (value != mCurDimIndex[IGC])
                return;
            update_list[IGC] = 0;
            if (value == 0) {
                srcIgcData = igcData;
                if (mTargetChanged[IGC]) {
                    mCurDimIndex[IGC] = 0;
                    mTargetChanged[IGC] = false;
                }
            }

            mCurDimIndex[IGC]++;
            if (mCurDimIndex[IGC] > mDimStep[IGC]) {
                update_list[IGC] = 1;
                srcIgcData = targetIgcData;
                if (mIgcEnable == 0) {
                    enable_list_mi[IGC] = 0;
                    t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
                    needs_update_ = true;
                }
                return;
            }
            if (IgcConfig.num_of_entries != 0) {
                numOfEntries = IgcConfig.num_of_entries;
                entriesWidth = IgcConfig.entries_width;
            }
            if (srcIgcData.r.size() != IgcConfig.num_of_entries) {
                STC_LOGE("Invalid src igc data size %d", srcIgcData.r.size());
                srcIgcData = targetIgcData;
            }
            igcData.r.clear();
            igcData.g.clear();
            igcData.b.clear();
            for (int i = 0; i < numOfEntries; i++) {
                temp = srcIgcData.r[i] + mCurDimIndex[IGC]
                        * (int)(targetIgcData.r[i] - srcIgcData.r[i]) / mDimStep[IGC];
                igcData.r.push_back(temp);
                temp = srcIgcData.g[i] + mCurDimIndex[IGC]
                        * (int)(targetIgcData.g[i] - srcIgcData.g[i]) / mDimStep[IGC];
                igcData.g.push_back(temp);
                temp = srcIgcData.b[i] + mCurDimIndex[IGC]
                        * (int)(targetIgcData.b[i] - srcIgcData.b[i]) / mDimStep[IGC];
                igcData.b.push_back(temp);
            }

            update_list[IGC] = 1;

            igcDataBackup = igcData;
            needs_update_ = true;
            mTimerGeneration[TIMER_REFRESH]++;
            t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
        } break;
        case TIMER_6ZONE: {
            if (abs(value - mCurDimIndex[SIX_ZONE]) > DIMMING_MAX_THREAD_CNT) {
                ALOGE("Too many 6zone threads wait, skip %d, cur index %d", value, mCurDimIndex[SIX_ZONE]);
                break;
            }
            Mutex::Autolock autoLock(m6ZoneDimmingLock);
            STC_LOGV("%s TIMER 6zone value %d, curIndex %d", __func__, value, mCurDimIndex[SIX_ZONE]);
            update_list[SIX_ZONE] = 0;
            if (value != mCurDimIndex[SIX_ZONE])
                return;
            if (value == 0 || mTargetChanged[SIX_ZONE]) {
                mSrc6Zone = mCur6Zone;
                if (mTargetChanged[SIX_ZONE]) {
                    mCurDimIndex[SIX_ZONE] = 0;
                    mTargetChanged[SIX_ZONE] = false;
                }
            }
            mCurDimIndex[SIX_ZONE]++;
            if (mCurDimIndex[SIX_ZONE] > mDimStep[SIX_ZONE]) {
                update_list[SIX_ZONE] = 1;
                mSrc6Zone = mTarget6Zone;
                enable_list_mi[SIX_ZONE] = 0;
                t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
                needs_update_ = true;
                return;
            }
            mCur6Zone.flags = mTarget6Zone.flags;
            mCur6Zone.num_entries = mTarget6Zone.num_entries;
            for (int i = 0; i < SIX_ZONE_ENTRIES_SIZE; i++) {
                mSrc6Zone.entries[i].hue =
                    (double)transformUnsignedToSigned((int)mSrc6Zone.entries[i].hue, 4096);
                mTarget6Zone.entries[i].hue =
                    (double)transformUnsignedToSigned((int)mTarget6Zone.entries[i].hue, 4096);
                mSrc6Zone.entries[i].saturation =
                    (double)transformUnsignedToSigned((int)mSrc6Zone.entries[i].saturation, 4096);
                mTarget6Zone.entries[i].saturation =
                    (double)transformUnsignedToSigned((int)mTarget6Zone.entries[i].saturation, 4096);
                mSrc6Zone.entries[i].value =
                    (double)transformUnsignedToSigned((int)mSrc6Zone.entries[i].value, 4096);
                mTarget6Zone.entries[i].value =
                    (double)transformUnsignedToSigned((int)mTarget6Zone.entries[i].value, 4096);

                mCur6Zone.entries[i].hue = mSrc6Zone.entries[i].hue + mCurDimIndex[SIX_ZONE]
                    * (mTarget6Zone.entries[i].hue - mSrc6Zone.entries[i].hue) / mDimStep[SIX_ZONE];
                mCur6Zone.entries[i].saturation = mSrc6Zone.entries[i].saturation + mCurDimIndex[SIX_ZONE]
                    * (mTarget6Zone.entries[i].saturation - mSrc6Zone.entries[i].saturation) / mDimStep[SIX_ZONE];
                mCur6Zone.entries[i].value = mSrc6Zone.entries[i].value + mCurDimIndex[SIX_ZONE]
                    * (mTarget6Zone.entries[i].value - mSrc6Zone.entries[i].value) / mDimStep[SIX_ZONE];

                mSrc6Zone.entries[i].hue =
                    (double)transformSignedToUnsigned((int)mSrc6Zone.entries[i].hue, 4096);
                mSrc6Zone.entries[i].saturation =
                    (double)transformSignedToUnsigned((int)mSrc6Zone.entries[i].saturation, 4096);
                mSrc6Zone.entries[i].value =
                    (double)transformSignedToUnsigned((int)mSrc6Zone.entries[i].value, 4096);
                mCur6Zone.entries[i].hue =
                    (double)transformSignedToUnsigned((int)mCur6Zone.entries[i].hue, 4096);
                mCur6Zone.entries[i].saturation =
                    (double)transformSignedToUnsigned((int)mCur6Zone.entries[i].saturation, 4096);
                mCur6Zone.entries[i].value =
                    (double)transformSignedToUnsigned((int)mCur6Zone.entries[i].value, 4096);
                mTarget6Zone.entries[i].hue =
                    (double)transformSignedToUnsigned((int)mTarget6Zone.entries[i].hue, 4096);
                mTarget6Zone.entries[i].saturation =
                    (double)transformSignedToUnsigned((int)mTarget6Zone.entries[i].saturation, 4096);
                mTarget6Zone.entries[i].value =
                    (double)transformSignedToUnsigned((int)mTarget6Zone.entries[i].value, 4096);
            }
            mTimerGeneration[TIMER_6ZONE]++;
            t.AsyncWait(mDimTimeMs, timerCallbackFunc, this,
                TIMER_6ZONE, mTimerGeneration[TIMER_6ZONE], mCurDimIndex[SIX_ZONE]);
            update_list[SIX_ZONE] = 1;
            enable_list_mi[SIX_ZONE] = 1;
            needs_update_ = true;
            m6ZoneBackup = mCur6Zone;
            mTimerGeneration[TIMER_REFRESH]++;
            t.AsyncWait(0, timerCallbackFunc, this, TIMER_REFRESH, mTimerGeneration[TIMER_REFRESH], 0);
        } break;
        case TIMER_REFRESH: {
            forceScreenRefresh();
        } break;
        case TIMER_REFRESH_TIMEOUT: {
            if (mWaitRefresh) {
                mWaitRefresh = 0;
                if (mRefreshRetryCnt > 0) {
                    needs_update_ = true;
                    forceScreenRefresh();
                    mRefreshRetryCnt--;
                } else {
                    mRefreshRetryCnt = 3;
                }
            }
        } break;
        case TIMER_SYNC: {
            mTimerGeneration[TIMER_SYNC]++;
            if (mCurDimIndex[IGC] > mDimStep[IGC]
                && mCurDimIndex[GC] > mDimStep[GC]
                && mCurDimIndex[LUT] > mDimStep[LUT]
                && mCurDimIndex[PA] > mDimStep[PA]
                && mCurDimIndex[SIX_ZONE] > mDimStep[SIX_ZONE]) {
                mTimerGeneration[TIMER_SYNC] = 0;
                mDimStep[IGC] = 20;
                break;
            }
            for (int i = PCC; i < UPDATE_LIST_MAX; i++) {
                if (mCurDimIndex[i] <= mDimStep[i] && mDimEnable[i]) {
                    mTimerGeneration[i]++;
                    t.AsyncWait(0, timerCallbackFunc, this,
                        i, mTimerGeneration[i], mCurDimIndex[i]);
                }
            }

            t.AsyncWait(mDimTimeMs, timerCallbackFunc, this,
                TIMER_SYNC, mTimerGeneration[TIMER_SYNC],0);
        } break;
        default:
            break;
    }
}

/*6zone HSV ranges 0~4095, 0~2047 are positive value and 2048~4095 are negative,
  should transform value to signed value*/
int MiStcImpl::transformUnsignedToSigned(int value, int max)
{
    value += (max/2);
    value = value % max;
    value -= (max/2);

    return value;
}

int MiStcImpl::transformSignedToUnsigned(int value, int max)
{
    value += max;
    value = value % max;

    return value;
}
} //namespace snapdragoncolor

