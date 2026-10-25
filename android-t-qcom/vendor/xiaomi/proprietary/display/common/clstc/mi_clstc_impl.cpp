/*
 * Copyright (c) 2022 Xiaomi Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Xiaomi Technologies, Inc.
 */

#include <errno.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <log/log.h>

#include "mi_clstc_impl.h"

//using namespace clstc;
using namespace sdm;
using namespace android;

#undef LOG_TAG
#define LOG_TAG "MiClstc"
namespace clstc {

static inline const char* getPipeType(int pipe_type) {
    switch (pipe_type) {
        case kVigBlock : return "VIG";
        case kDmaBlock : return "DMA";
        default : return "Unknown";
    }
}

extern "C" ClstcInterface* GetClstcInterface(uint32_t major_version, uint32_t minor_version) {
    (void)major_version;
    (void)minor_version;
    ClstcInterface *intf = new MiClstc;
    return intf;
}

int MiClstc::log_level = 1;
int MiClstc::mInitNum = 0;

MiClstc::MiClstc() {
    get_func_[kClstcInterfaceDetails] = &MiClstc::GetInterfaceDetails;
    get_func_[kClstcSupportedColorFeatures] = &MiClstc::GetSupportedColorFeatures;
    get_func_[kClstcSupportedColorTransforms] = &MiClstc::GetSupportedColorTransforms;
    get_func_[kClstcLibraryConfig] = &MiClstc::GetLibraryConfig;
    get_func_[kClstcLibraryQueryRequest] = &MiClstc::QueryLibraryRequest;
    get_func_[kClstcUpdateStatus] = &MiClstc::UpdateStatus;

    set_func_[kClstcHwCapabilities] = &MiClstc::SetHwCapabilities;
    set_func_[kClstcDisplayCapabilities] = &MiClstc::SetDisplayCapabilities;
    set_func_[kClstcFeatureControl] = &MiClstc::SetTmFeatureControl;
    set_func_[kClstcLibraryConfig] = &MiClstc::SetLibraryConfig;
    set_func_[kClstcBlendInfo] = &MiClstc::SetBlendInfo;
    set_func_[kClstcTargetState] = &MiClstc::SetTargetState;
    set_func_[kClstcStageLayers] = &MiClstc::StageLayers;
    set_func_[kClstcDropLayers] = &MiClstc::DropLayers;
    set_func_[kClstcResetStack] = &MiClstc::ResetStack;

    op_func_[kClstcRetrieveConfig] = &MiClstc::RetrieveConfig;
}

int MiClstc::Init() {
    std::lock_guard<std::mutex> guard(lock_);
    LOGI("Init: init_done_ %d", init_done_);
    if (init_done_)
        return 0;
    qservice_ = interface_cast<qService::IQService>(
          defaultServiceManager()->getService(String16("display.qservice")));
    if (qservice_ == NULL) {
        LOGE("Failed to acquire display.qservice");
    } else {
        LOGI("Getting IQService...done!");
    }
    init_done_ = true;
    LOGE("Init: finished");
    return 0;
}

int MiClstc::Deinit() {
    std::lock_guard<std::mutex> guard(lock_);

    if (init_done_ == false)
        return 0;

    display_type_ = kDisplayTypeUnknown;
    blend_space_ = {ColorPrimaries_Max, Transfer_Max};
    backlight_state_ = {0};
    layer_stack_.clear();
    init_done_ = false;
    return 0;
}

int MiClstc::InitLut(MiImplType disp_id)
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
    sprintf(path, "/odm/etc/disp%d/%s/clstc/ClstcLutInfo.txt",disp_id, panel_name);
    if (access(path, F_OK|R_OK) < 0) {
        LOGE("can not access file %s", path);
        return -1;;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        LOGE("open file error %s", path);
        return -1;
    }
    ret = fscanf(fp, "%d", &LutFileNum);
    fclose(fp);
    if (ret < 0)
        return -1;
    // parse factory calibration lut files
    for (int i = 0; i < LutFileNum; i++) {
        sprintf(path, "/odm/etc/disp%d/%s/clstc/ClstcLut%d.txt", disp_id, panel_name, i);
        LOGD("reading %s", path);
        if (access(path, F_OK|R_OK) < 0) {
            LOGE("can not access file %s", path);
            return -1;
        }
        ret = readLutData(path);
        if (ret < 0) {
            LOGE("Failed to parse calib lut srgb file");
            LutInfo.clear();
            return -1;
        }
    }

    mLutInitialized = true;
    return 0;
}

int MiClstc::readLutData(char* path)
{
    FILE *fp = NULL;
    struct rgb_entry temp;
    int cnt = 0;
    int ret = 0;
    int entries = pow(vig_3Dlut_caps_.entries,3);
    if (access(path, F_OK|R_OK) < 0) {
        LOGE("can not access file %s", path);
        return -1;
    }
    fp = fopen(path, "r");
    if (fp == NULL) {
        LOGE("open file error %s", path);
        return -1;
    }
    while (fscanf(fp, "%u %u %u %u %u %u", &temp.in.r, &temp.in.g, &temp.in.b,
            &temp.out.r, &temp.out.g, &temp.out.b) != EOF) {
        LutInfo.push_back(temp);
        cnt++;
        if (cnt > entries) {
            LOGE("Invalid lut file, need %d, receive %d", entries, cnt);
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    if (cnt != entries) {
         LOGE("Invalid Lut file %s, line num %d, need %d", path, cnt, entries);
         return -1;
    }
    return ret;
}

int MiClstc::SetParameter(uint32_t prop, const GenericPayload &payload) {
    std::lock_guard<std::mutex> guard(lock_);

    if (init_done_ == false) {
        LOGE("Invalid params : init_done_ %d\n", init_done_);
        return -EINVAL;
    }

    auto it = set_func_.find(ClstcProperty(prop));
    if (it == set_func_.end() || !it->second) {
        if (prop >= kClstcPrivateStart && prop < kClstcPropertyMax) {
            LOGI("Unsupported prop %d\n", prop);
            return -ENOTSUP;
        }

        LOGE("Invalid prop %d\n", prop);
        return -EINVAL;
    }

    return (this->*(it->second))(payload);
}

void MiClstc::InitGamma(MiImplType disp_id)
{
    int ret = 0;
    FILE *fp = NULL;
    int gammaData;
    int cnt = 0;
    char path[256];
    char panel_name[255];
    string file_name = disp_id == kMiImplPrimary ? PRIM_PANEL_INFO_PATH : SEC_PANEL_INFO_PATH;

    igcDataBuf.clear();
    gcDataBuf.clear();
    mIgcFileCnt = 0;
    mGcFileCnt = 0;

    if (mParseXml[disp_id].get()) {
        mParseXml[disp_id]->getPanelName(file_name.c_str(),panel_name);
    } else {
        LOGE("%s can not  get mParseXml[%d]", __func__, disp_id);
        return;
    }
    while(1) {
        cnt = 0;
        sprintf(path, "/odm/etc/disp%d/%s/clstc/clstc_igc_%d.txt", disp_id, panel_name, mIgcFileCnt);
        if (access(path, F_OK|R_OK) < 0) {
            LOGE("can not access file %s", path);
            break;
        }
        fp = fopen(path, "r");
        if (fp == NULL) {
            LOGE("open file error %s", path);
            break;
        }
        while (fscanf(fp, "%d", &gammaData) != EOF) {
            igcDataBuf.push_back(gammaData);
            cnt++;
        }
        fclose(fp);
        if (cnt != dma_igc_caps_.entries) {
             LOGE("Invalid gamma file %s, cnt %d, numOfEntries %d", path, cnt, dma_igc_caps_.entries);
             break;
        }
        mIgcFileCnt++;
    }
    while(1) {
        cnt = 0;
        sprintf(path, "/odm/etc/disp%d/%s/clstc/clstc_gc_%d.txt", disp_id, panel_name, mGcFileCnt);
        if (access(path, F_OK|R_OK) < 0) {
            LOGE("can not access file %s", path);
            break;
        }
        fp = fopen(path, "r");
        if (fp == NULL) {
            LOGE("open file error %s", path);
            break;
        }
        while (fscanf(fp, "%d", &gammaData) != EOF) {
            gcDataBuf.push_back(gammaData);
            cnt++;
        }
        fclose(fp);
        if (cnt != dma_gc_caps_.entries) {
             LOGE("Invalid gamma file %s, cnt %d, numOfEntries %d", path, cnt, dma_gc_caps_.entries);
             break;
        }
        mGcFileCnt++;
    }
}

int MiClstc::GetParameter(uint32_t prop, GenericPayload *payload) {
    std::lock_guard<std::mutex> guard(lock_);

    if (init_done_ == false) {
        LOGE("Invalid params : init_done_ %d\n", init_done_);
        return -EINVAL;
    }

    auto it = get_func_.find(ClstcProperty(prop));
    if (it == get_func_.end() || !it->second) {
        if (prop >= kClstcPrivateStart && prop < kClstcPropertyMax) {
            LOGI("Unsupported prop %d\n", prop);
            return -ENOTSUP;
        }

        LOGE("Invalid prop %d\n", prop);
        return -EINVAL;
    }

    return (this->*(it->second))(payload);
}

int MiClstc::ProcessOps(uint32_t op, const GenericPayload &input, GenericPayload *output) {
    std::lock_guard<std::mutex> guard(lock_);

    if (init_done_ == false) {
        LOGE("Invalid params : init_done_ %d\n", init_done_);
        return -EINVAL;
    }

    auto it = op_func_.find(ClstcOp(op));
    if (it == op_func_.end() || !it->second) {
        LOGE("Invalid ops %d\n", op);
        return -EINVAL;
    }

    return (this->*(it->second))(input, output);
}

int MiClstc::GetInterfaceDetails(GenericPayload *payload) {
    int rc;
    uint32_t sz = 0;
    ClstcInterfaceDetails *details = nullptr;

    rc = payload->GetPayload(details, &sz);
    if (rc) {
        LOGE("Failed to get payload, rc %d\n", rc);
        return rc;
    } else if (sz != 1 || !details) {
        LOGE("Invalid payload, array sz %d, exp sz %d, details %pK\n", sz,
         1, reinterpret_cast<void*>(details));
        return -EINVAL;
    }

    details->name = "Clstc_Sample_Library";
    details->interface_version = 1 << 16;
    details->configuration_version = 1 << 16;
    return 0;
}

int MiClstc::GetSupportedColorFeatures(GenericPayload *payload) {
    int rc = 0;
    uint32_t sz = 0;
    ClstcColorFeatures *features = nullptr;

    rc = payload->GetPayload(features, &sz);
    if (rc) {
        LOGE("Failed to get payload, rc %d\n", rc);
        return rc;
    } else if (sz != 1 || !features) {
        LOGE("Invalid payload, array sz %d, exp sz %d, details %pK\n", sz,
            1, reinterpret_cast<void*>(features));
        return -EINVAL;
     }

    ClstcColorFeatureDef *f;
    features->color_feature_definitions.clear();

    // Add the features this implement supports.
    // E.g., HDR10, HDR10+ and GCP
    features->color_feature_definitions.emplace_back();
    f = &features->color_feature_definitions.back();
    f->feature = kClstcHdr10Support;

    features->color_feature_definitions.emplace_back();
    f = &features->color_feature_definitions.back();
    f->feature = kClstcHdr10PlusSupport;

    features->color_feature_definitions.emplace_back();
    f = &features->color_feature_definitions.back();
    f->feature = kClstcGameColorPlusSupport;

    return 0;
}

int MiClstc::GetSupportedColorTransforms(GenericPayload *payload) {
    return 0;
}

int MiClstc::GetLibraryConfig(GenericPayload *payload) {
    return 0;
}

int MiClstc::QueryLibraryRequest(GenericPayload *payload) {
    return 0;
}

int MiClstc::UpdateStatus(GenericPayload *payload) {
    ClstcLayerStackAnalysis *info = nullptr;
    int rc = 0;
    uint32_t sz = 0;

    rc = payload->GetPayload(info, &sz);
    if (rc) {
        LOGE("GetPayload failed sz %d\n", sz);
        return rc;
    } else if (!info || sz != 1) {
        LOGE("Invalid param info %p, array sz %d, exp sz %uz\n", reinterpret_cast<void*>(info), sz, 1);
        return -EINVAL;
    }

    info->analyzed_layers.clear();
    info->vote_cpu_requested = false;

    // Mark the SDR layer with dynamic metadata as needs_update
    for (auto it : layer_stack_) {
        uint32_t layer_id = it.first;
        LayerStatus layer_status = it.second.layer_status;
        std::shared_ptr<const ClstcLayerDetails> layer = it.second.layer_details;
        if (!layer || !layer.get()) {
            LOGE("Invalid layer %d", layer_id);
            continue;
        }

        // Create layer analysis
        info->analyzed_layers.emplace_back();
        ClstcLayerAnalysis * layer_analysis = &info->analyzed_layers.back();
        layer_analysis->layer_id = layer_id;
        layer_analysis->rec = kConfigUpdateUnnecessary;

        if (layer_status == kNoUpdatedLayer) {
            LOGV("Layer %d: %s, analysis rec %d", layer_id, layer->layer_name.c_str(),
                layer_analysis->rec);
            continue;
        }

        // Check hdr or sdr layer type
        uint32_t sz = 0;
        ColorMetaData *metadata = nullptr;
        rc = RetrieveClstcDataBlock(kClstcDataLayerColorMetadata, layer.get(), metadata, &sz);
        if (rc || metadata == nullptr) {
            LOGE("Invalid metadata for layer %d, rc %d", layer_id, rc);
            continue;
        }

        bool is_hdr = false;
        if ((metadata->colorPrimaries == ColorPrimaries_BT2020) &&
            (metadata->transfer == Transfer_SMPTE_ST2084 || metadata->transfer == Transfer_HLG)) {
            is_hdr = true;
        }

        if (!is_hdr && metadata->dynamicMetaDataValid && layer->content_metadata_changed) {
        layer_analysis->rec = kConfigUpdateNeeded;
        info->vote_cpu_requested = true;
        }

        LOGV("Layer %d: %s, analysis rec %d", layer_id, layer->layer_name.c_str(),
            layer_analysis->rec);
    }

    LOGV("vote_cpu_requested %d", info->vote_cpu_requested);

    if (mLayerUpdateStatus.size() != 0) {
        for (auto it = mLayerUpdateStatus.begin(); it != mLayerUpdateStatus.end(); it++) {
            info->analyzed_layers.push_back(it->second);
        }
    }

    return 0;
}

int MiClstc::SetHwCapabilities(const GenericPayload &payload) {
    ClstcPipeCapabilities *cap = nullptr;
    int rc = 0;
    uint32_t cap_sz = 0, sz = 0;

    rc = payload.GetPayload(cap, &cap_sz);
    if (rc) {
        LOGE("GetPayload failed sz %d\n", sz);
        return rc;
    } else if (!cap || !cap_sz) {
        LOGE("Invalid param info %pK, array sz %d\n", reinterpret_cast<void*>(cap), cap_sz);
        return -EINVAL;
    }

    // The capabilities are cached.
    // Pls generate LUTs according to the capability definitions, like the entry size, dimension, etc.

    for (uint32_t i = 0; i < cap_sz; i++) {
        if (cap[i].pipe == kVigBlock) {
            for (auto &hw : cap[i].hw_details) {
                if (hw.feature == k3dLutFeature) {
                    LutCapabilities *lut_cap = nullptr;
                    rc = RetrieveClstcDataBlock(kClstcDataHwLutCap, &hw, lut_cap, &sz);
                    if (rc || !lut_cap) {
                        LOGI("Failed to retrieve block kClstcDataHwLutCap, rc %d\n", rc);
                    } else {
                        vig_3Dlut_caps_ = *lut_cap;
                        LOGI("VIG 3Dlut cap: bit_depth %d, dimension %d, entries %d\n",
                        vig_3Dlut_caps_.bit_depth, vig_3Dlut_caps_.dimension, vig_3Dlut_caps_.entries);
                    }
                } else if (hw.feature == kDetailEnhancer) {
                    DeCapabilities *de_cap = nullptr;
                    rc = RetrieveClstcDataBlock(kClstcDataHwDetailEnhancerCap, &hw, de_cap, &sz);
                    if (rc || !de_cap) {
                        LOGI("Failed to retrieve block kClstcDataHwDetailEnhancerCap, rc %d\n", rc);
                    } else {
                        vig_de_caps_ = *de_cap;
                        LOGI("VIG DE cap: version 0x%x\n", vig_de_caps_.version);
                    }
                } else {
                    LOGI("Unsupported feature capability %d for VIG block\n", hw.feature);
                }
            }
        } else if (cap[i].pipe == kDmaBlock) {
            for (auto &hw : cap[i].hw_details) {
                if (hw.feature == kIgcFeature || hw.feature == kGcFeature) {
                    LutCapabilities *lut_cap = nullptr;
                    rc = RetrieveClstcDataBlock(kClstcDataHwLutCap, &hw, lut_cap, &sz);
                    if (rc || !lut_cap) {
                        LOGI("Failed to retrieve block kClstcDataHwLutCap, rc %d\n", rc);
                    } else {
                        if (hw.feature == kIgcFeature) {
                            dma_igc_caps_ = *lut_cap;
                            LOGI("DMA IGC cap: bit_depth %d, dimension %d, entries %d\n",
                                dma_igc_caps_.bit_depth, dma_igc_caps_.dimension, dma_igc_caps_.entries);
                        } else {
                            dma_gc_caps_ = *lut_cap;
                            LOGI("DMA GC cap: bit_depth %d, dimension %d, entries %d\n",
                                dma_gc_caps_.bit_depth, dma_gc_caps_.dimension, dma_gc_caps_.entries);
                        }
                    }
                } else if (hw.feature == kCscFeature) {
                    MatrixCapabilities *matrix_cap = nullptr;
                    rc = RetrieveClstcDataBlock(kClstcDataHwMatrixCap, &hw, matrix_cap, &sz);
                    if (rc || !matrix_cap) {
                        LOGI("Failed to retrieve block kClstcDataHwMatrixCap, rc %d\n", rc);
                    } else {
                        dma_csc_caps_ = *matrix_cap;
                        LOGI("DMA CSC cap: bit_depth %d, width %d, height %d\n",
                            dma_csc_caps_.bit_depth, dma_csc_caps_.width, dma_csc_caps_.height);
                    }
                } else {
                    LOGE("Unsupported feature capability %d for DMA block\n", hw.feature);
                }
            }
        } else {
            LOGE("Invalid pipe type %d\n", cap[i].pipe);
        }
    }

    return 0;
}

int MiClstc::SetDisplayCapabilities(const GenericPayload &payload) {
    ClstcDisplayInfo *info = nullptr;
    int rc = 0;
    uint32_t sz = 0;

    rc = payload.GetPayload(info, &sz);
    if (rc) {
        LOGE("GetPayload failed sz %d\n", sz);
        return rc;
    } else if (!info || sz != 1) {
        LOGE("Invalid param info %pK, array sz %d, exp sz %uz\n", reinterpret_cast<void*>(info), sz, 1);
        return -EINVAL;
    }

    LOGI("Display type %d, display id %d, panel_name %s", info->type, info->display_id, info->panel_name.c_str());

    MiClstcService::init();

   sp<IMiClstcService> imiclstcservice = interface_cast<IMiClstcService>(
        defaultServiceManager()->getService(String16("miclstcservice")));
    LOGI("Getting IMiClstcService...done!");

    if (imiclstcservice.get()) {
        if(mInitNum==0)
        {
            imiclstcservice->connect(sp<qClient::IQClient>(this));
            miclstcservice_ = reinterpret_cast<MiClstcService *>(imiclstcservice.get());
            mInitNum++;
        }
    } else {
        LOGE("Failed to acquire display.mistcservice");
        return -EINVAL;
    }

    mParseXml[kMiImplPrimary] = new MiParseXml();
    InitLut(kMiImplPrimary);
    InitGamma(kMiImplPrimary);
    return rc;
}

int MiClstc::SetTmFeatureControl(const GenericPayload &payload) {
    return 0;
}

int MiClstc::SetLibraryConfig(const GenericPayload &payload) {
    return 0;
}

int MiClstc::SetBlendInfo(const GenericPayload &payload) {
    int rc;
    uint32_t sz = 0;
    ClstcBlendInfo *blend_info = nullptr;

    rc = payload.GetPayload(blend_info, &sz);
    if (rc) {
        LOGE("GetPayload failed rc %d\n", rc);
        return rc;
    } else if (!blend_info || sz != 1) {
        LOGE("Invalid blend_info %pK, array sz %u, exp %u\n",
            reinterpret_cast<void*>(blend_info), sz, 1);
        return -EINVAL;
    }

    ClstcColorSpaceInfo *space = nullptr;
    rc = RetrieveClstcDataBlock(kClstcDataOutputColorSpace, blend_info, space, &sz);
    if (rc) {
        LOGE("Failed to retrive data block kClstcDataOutputColorSpace, rc %d\n", rc);
        return rc;
    }


    // cache the blend space for later usage if need
    if (blend_space_.gamut != space->gamut || blend_space_.gamma != space->gamma) {
        blend_space_.gamut = space->gamut;
        blend_space_.gamma = space->gamma;
    }

    return rc;
}

int MiClstc::SetTargetState(const GenericPayload &payload) {
    int rc;
    uint32_t sz = 0;
    ClstcTargetState *target_state = nullptr;

    rc = payload.GetPayload(target_state, &sz);
    if (rc) {
        LOGE("GetPayload failed rc %d\n", rc);
        return rc;
    } else if (!target_state || sz != 1) {
        LOGE("Invalid param %pK, array sz %d, exp sz %uz\n",
            reinterpret_cast<void*>(target_state), sz, 1);
        return -EINVAL;
    }

    ClstcBacklightState* backlight = nullptr;
    rc = RetrieveClstcDataBlock(kClstcDataBacklightState, target_state, backlight, &sz);
    if (rc) {
        LOGE("Failed to retrive data block for ClstcBacklightState rc %d\n", rc);
        return rc;
    }

    // cache the backlight info for later usage if need
    backlight_state_.max_backlight = backlight->max_backlight;
    backlight_state_.curr_backlight = backlight->curr_backlight;

    return 0;
}

int MiClstc::StageLayers(const GenericPayload &payload) {
    int rc;
    uint32_t sz = 0;
    ClstcLayerStack *input_stack = nullptr;
    bool new_hdr_present = false;

    rc = payload.GetPayload(input_stack, &sz);
    if (rc) {
        LOGE("GetPayload failed rc %d\n", rc);
        return rc;
    } else if (!input_stack || sz != 1) {
        LOGE("Invalid param %pK, array sz %d, exp sz %u\n",
            reinterpret_cast<void*>(input_stack), sz, 1);
        return -EINVAL;
    }

    for (uint32_t i = 0; i < input_stack->layers.size(); i++) {
        std::shared_ptr<const ClstcLayerDetails> input_layer = input_stack->layers[i];
        const uint32_t layer_id = input_stack->layers[i]->layer_id;
        // Cache the layers into layer stack
        if (layer_stack_.find(layer_id) == layer_stack_.end()) {
            layer_stack_[layer_id].layer_details = input_layer;
            layer_stack_[layer_id].layer_status = kNewLayer;
            LOGI("cache new layer %d", layer_id);
        } else {
            layer_stack_[layer_id].layer_details = input_layer;
            layer_stack_[layer_id].layer_status = kUpdatedLayer;
            LOGI("update cached layer %d", layer_id);
        }
    }

    return 0;
}

int MiClstc::DropLayers(const GenericPayload &payload) {
    int rc;
    uint32_t sz = 0;
    uint32_t *layer_ids = nullptr;

    rc = payload.GetPayload(layer_ids, &sz);
    if (rc) {
        LOGE("GetPayload failed rc %d\n", rc);
        return rc;
    } else if (!layer_ids || !sz) {
        LOGE("Invalid param %p, invalid array sz %d\n", reinterpret_cast<void*>(layer_ids), sz);
        return -EINVAL;
    }
    for (size_t entry = 0; entry < sz; entry++) {
        // Remove the layers from layer stack
        layer_stack_.erase(layer_ids[entry]);
        LOGI("erase layer %d", layer_ids[entry]);
    }

    return 0;
}

int MiClstc::ResetStack(const GenericPayload &payload) {
    (void)payload;
    layer_stack_.clear();
    return 0;
}

int MiClstc::RetrieveConfig(const GenericPayload &input, GenericPayload *output) {
    int rc = 0;
    uint32_t sz = 0;
    uint32_t *layer_id = nullptr;
    ClstcColorConfiguration* config = nullptr;

    // 1. Parse input and output. Input is layer id, and output is layer config that includes LUTs
    rc = input.GetPayload(layer_id, &sz);
    if (rc) {
        LOGE("GetPayload for input failed rc %d\n", rc);
        return rc;
    } else if (!layer_id || sz != 1) {
        LOGE("Invalid input %p, sz %d, exp sz %lu\n", reinterpret_cast<void*>(layer_id),
            sz, sizeof(uint32_t));
        return -EINVAL;
    }

    rc = output->GetPayload(config, &sz);
    if (rc) {
        LOGE("GetPayload for output failed rc %d\n", rc);
        return rc;
    } else if (!config || sz != 1) {
        LOGE("Invalid output %p, array sz %d, exp sz %u\n", reinterpret_cast<void*>(config), sz, 1);
        return -EINVAL;
    }

    // 2. Skip the layer if the config is locked
    if (config->lock_config) {
        LOGI("Layer %d config is locked\n", *layer_id);
        return rc;
    }

    //3. Check the layer stack if the layer is exist
    auto it = layer_stack_.find(*layer_id);
    if (it == layer_stack_.end()) {
        LOGI("Unknown layer %u, skip\n", *layer_id);
        return rc;
    }
    std::string layerName = simplifyLayerName(*layer_id);
    // 4. Generate LUTs
    std::shared_ptr<const ClstcLayerDetails> layer_details = it->second.layer_details;
    LOGV("layer id %u, type %d, name %s", *layer_id, layer_details->pipe_type, layerName.c_str());
    if (layer_details->pipe_type == kVigBlock) {
        // generate LUTs here.

        // 5. Pack 1d LUT into the output config. Override the existed 1d LUT or DE config
        Clstc1DCsc1DLut *lut1d;
        rc = RetrieveClstcDataBlock(kClstcData1DCsc1DConfig, config, lut1d, &sz);
        if (!rc && lut1d) {
            uint32_t checksum = 0;
            lut1d->csc_data.valid = true;
            lut1d->csc_data.data[0] = lut1d->csc_data.data[0] / 2;
            lut1d->csc_data.data[1] = lut1d->csc_data.data[1] / 3;
            lut1d->csc_data.data[2] = lut1d->csc_data.data[2] / 2;

            auto lutId = mLutInfo.find(layerName);
            if (lutId != mLutInfo.end()) {
                mLayerUpdateStatus[layerName].rec = kConfigUpdateUnnecessary;
            }
        }
    } else if (layer_details->pipe_type == kDmaBlock) {
        // Override IGC/GC/pcc matrix here.
        Clstc1DCsc1DLut *dmaConfig;
        rc = RetrieveClstcDataBlock(kClstcData1DCsc1DConfig, config, dmaConfig, &sz);
        if (dmaConfig) {
            auto pccInfo = mPccInfo.find(layerName);
            if (pccInfo != mPccInfo.end()) {
                dmaConfig->csc_data.data.clear();
                for (int i = 0; i < ARRAY_SIZE; i++)
                    for (int j = 0; j < ARRAY_SIZE; j++) {
                       dmaConfig->csc_data.data.push_back(pccInfo->second[i*ARRAY_SIZE+j]);
                       LOGE("%d: %lld", i*ARRAY_SIZE + j, dmaConfig->csc_data.data[i*ARRAY_SIZE + j]);
                    }
                mLayerUpdateStatus[layerName].rec = kConfigUpdateUnnecessary;
            }

            auto igcInfo = mIGCInfo.find(layerName);
            if (igcInfo != mIGCInfo.end()) {
                int gammaId = igcInfo->second;
                uint32_t entries = dma_igc_caps_.entries;
                dmaConfig->igc_data.data.clear();
                dmaConfig->igc_data.data.assign(igcDataBuf.begin() + gammaId * entries,
                    igcDataBuf.begin() + (gammaId + 1) * entries);
                mLayerUpdateStatus[layerName].rec = kConfigUpdateUnnecessary;
            }
            auto gcInfo = mGCInfo.find(layerName);
            if (gcInfo != mGCInfo.end()) {
                int gammaId = gcInfo->second;
                uint32_t entries = dma_gc_caps_.entries;
                dmaConfig->gc_data.data.clear();
                dmaConfig->gc_data.data.assign(gcDataBuf.begin() + gammaId * entries,
                    gcDataBuf.begin() + (gammaId + 1) * entries);
                mLayerUpdateStatus[layerName].rec = kConfigUpdateUnnecessary;
            }
            if (log_level < 1)
                showDMAPipeConfig(dmaConfig);
        }
    }
    // 6. Pack the input and output color space into config
    ColorMetaData *metadata = nullptr;
    rc = RetrieveClstcDataBlock(kClstcDataLayerColorMetadata, layer_details.get(), metadata, &sz);
    if (rc || !metadata) {
        LOGE("Failed to retrieve metadata for layer %u\n", *layer_id);
        return -EINVAL;
    }

    ClstcColorSpaceInfo blend_space_input;
    blend_space_input.gamut = metadata->colorPrimaries;
    blend_space_input.gamma = metadata->transfer;
    rc = PackColorSpaceToConfig(config, blend_space_input, blend_space_);
    if (rc) {
        LOGE("Failed to pack color space to output config, rc %d\n", rc);
    }

    // 7. Lock the layer if needed
    config->lock_config = true;
    it->second.layer_status = kNoUpdatedLayer;
    return rc;
}

void MiClstc::showDMAPipeConfig(Clstc1DCsc1DLut * config)
{
    for (int i = 0; i < config->csc_data.data.size(); i++) {
        LOGD("CSC matrix %d: %lld", i, config->csc_data.data[i]);
    }
    for (int i = 0; i < config->igc_data.data.size(); i++)
        LOGD("IGC[%d]:%d", i, config->igc_data.data[i]);
    for (int i = 0; i < config->gc_data.data.size(); i++)
        LOGD("GC[%d]:%d", i, config->gc_data.data[i]);
}

int MiClstc::PackColorSpaceToConfig(ClstcColorConfiguration* config,
                                        const ClstcColorSpaceInfo &input,
                                        const ClstcColorSpaceInfo &output) {
    int rc = 0;
    uint32_t sz = 0;

    // Input color space and output color space
     ClstcColorSpaceInfo *input_cs;
     rc = RetrieveClstcDataBlock(kClstcDataInputColorSpace, config, input_cs, &sz);
     if (rc) {
        rc = AddClstcDataBlock(kClstcDataInputColorSpace, config, input_cs);
        if (rc) {
            LOGE("Failed to add data block kClstcDataInputColorSpace, rc %d", rc);
            return rc;
        }
    }

    input_cs->gamut = input.gamut;
    input_cs->gamma = input.gamma;

    ClstcColorSpaceInfo *output_cs;
    rc = RetrieveClstcDataBlock(kClstcDataOutputColorSpace, config, output_cs, &sz);
    if (rc) {
        rc = AddClstcDataBlock(kClstcDataOutputColorSpace, config, output_cs);
        if (rc) {
            LOGE("Failed to add data block kClstcDataOutputColorSpace, rc %d", rc);
            return rc;
        }
    }

    output_cs->gamut = output.gamut;
    output_cs->gamma = output.gamma;
    return 0;
}

std::string MiClstc::simplifyLayerName(int layerId)
{
    std::string layerName = layer_stack_[layerId].layer_details->layer_name;
    int nPos_start = layerName.find("[");
    int nPos_end = layerName.find("]");
    return layerName.substr(nPos_start + 1, nPos_end - nPos_start - 1);
}

status_t MiClstc::notifyCallback(uint32_t command, const Parcel *input_parcel,
                                             Parcel *output_parcel) {
    int layerId = -1;
    int enable = 0;
    std::string layerName;
    status_t status = -EINVAL;
    if (command != IMiClstcService::SHOW_LAYER_INFO &&
        command != IMiClstcService::SET_LOG) {
        layerId = input_parcel->readInt32();
        enable = input_parcel->readInt32();
        auto it = layer_stack_.find(layerId);
        if (it == layer_stack_.end()) {
            LOGI("Unknown layer %u, skip\n", layerId);
            return -1;
        }
        layerName = simplifyLayerName(layerId);
        mLayerUpdateStatus[layerName].layer_id = layerId;
        mLayerUpdateStatus[layerName].rec = kConfigUpdateNeeded;
        mLayerUpdateStatus[layerName].synchronous_calculation = false;
    }
    switch (command) {
        case IMiClstcService::SHOW_LAYER_INFO: {
            if (!output_parcel) {
                LOGE("MiClstcService command = %d: output_parcel needed.", command);
                break;
            }
            std::shared_ptr<const ClstcLayerDetails> layer_details;
            output_parcel->writeInt32(layer_stack_.size());
            for (auto it = layer_stack_.begin(); it != layer_stack_.end(); it++) {
                layer_details = it->second.layer_details;
                LOGD("layer[%d]: name %s, pipe type %s", it->first, it->second.layer_details->layer_name.c_str(), getPipeType(layer_details->pipe_type));
                output_parcel->writeInt32(it->first);
                output_parcel->writeCString(simplifyLayerName(it->first).c_str());
                output_parcel->writeCString(getPipeType(layer_details->pipe_type));
            }
        } break;
        case IMiClstcService::SET_PCC: {
            if (!input_parcel) {
                LOGE("MiClstcService command = %d: input_parcel needed.", command);
                break;
            }
            double coeff;
            int32_t temp;
            std::array<int64_t, ARRAY_SIZE*ARRAY_SIZE> pccCoeff = {0};
            if (enable) {
                for (int i = 0; i < ARRAY_SIZE; i++)
                    for (int j = 0; j < ARRAY_SIZE; j++) {
                        coeff = static_cast<double>(input_parcel->readDouble());
                        LOGV("Layer[%d] set clstc pcc i:%d, coeff:%f", layerId, i*ARRAY_SIZE + j, coeff);
                        temp = (int32_t)(coeff * 512 + 0.5);
                        pccCoeff[i * ARRAY_SIZE + j] = temp << 16;
                        LOGV("Layer[%d] Clstc Pcc[%d]: %llx", layerId, i*ARRAY_SIZE+j, pccCoeff[i*ARRAY_SIZE+j]);
                }
                mPccInfo[layerName] = pccCoeff;
            } else {
                mPccInfo.erase(layerName);
            }
            forceScreenRefresh();
        } break;
        case IMiClstcService::SET_LUT: {
            if (!input_parcel) {
                LOGE("MiClstcService command = %d: input_parcel needed.", command);
                break;
            }
            uint32_t lutId = input_parcel->readUint32();
            if (lutId + 1 > LutFileNum) {
                LOGE("invalid lut file id %d", lutId);
                break;
            }
            if (enable)
                mLutInfo[layerName] = lutId;
            else
                mLutInfo.erase(layerName);

            forceScreenRefresh();
        } break;
        case IMiClstcService::SET_IGC: {
            if (!input_parcel) {
                LOGE("MiClstcService command = %d: input_parcel needed.", command);
                break;
            }
            uint32_t igcId = input_parcel->readUint32();
            if (igcId + 1 > LutFileNum) {
                LOGE("invalid igc file id %d", igcId);
                break;
            }
            if (enable)
                mIGCInfo[layerName] = igcId;
            else
                mIGCInfo.erase(layerName);

            forceScreenRefresh();
        } break;
        case IMiClstcService::SET_GC: {
            if (!input_parcel) {
                LOGE("MiClstcService command = %d: input_parcel needed.", command);
                break;
            }
            uint32_t gcId = input_parcel->readUint32();
            if (gcId + 1 > LutFileNum) {
                LOGE("invalid gc file id %d", gcId);
                break;
            }
            if (enable)
                mGCInfo[layerName] = gcId;
            else
                mGCInfo.erase(layerName);

            forceScreenRefresh();
        } break;
        case IMiClstcService::SET_LOG: {
            log_level = input_parcel->readInt32();
        } break;
        default:
            break;
    }
    return 0;
}

int MiClstc::forceScreenRefresh()
{
    int ret = 0;
    Parcel inParcel, outParcel;
    inParcel.writeInt32(1);
    inParcel.setDataPosition(0);

    if(qservice_ != NULL) {
        int err = qservice_->dispatch(qService::IQService::TOGGLE_SCREEN_UPDATES, &inParcel , &outParcel);
        if (err) {
            LOGE("Failed to dispatch screen refresh");
            ret = -1;
        }
        LOGD("success");
    }
    return ret;
}
} // namespace clstc
