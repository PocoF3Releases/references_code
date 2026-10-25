/*
* Copyright (c) 2022 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#ifndef __MI_CLSTC_IMPL_H__
#define __MI_CLSTC_IMPL_H__

#include <map>
#include <vector>
#include <mutex>

#include "clstc_interface.h"
#include "hdr_interface.h"
#include "mi_clstc_service.h"
#include "parseXml.h"
using namespace android;
namespace clstc {

enum MiImplType {
    kMiImplPrimary = 0,
    kMiImplSecondaryBuiltIn,
    kMaxNumImpl,
};

#define LOGV(fmt, args...) ({\
                if(MiClstc::log_level <= 0)\
                    ALOGD("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define LOGD(fmt, args...) ({\
                if(MiClstc::log_level <= 1)\
                    ALOGD("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define LOGI(fmt, args...) ({\
                if(MiClstc::log_level <= 2)\
                    ALOGI("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define LOGW(fmt, args...) ({\
                if(MiClstc::log_level <= 3)\
                    ALOGW("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define LOGE(fmt, args...) ({\
                if(MiClstc::log_level <= 4)\
                    ALOGE("%s " fmt, __func__, ##args);\
                else\
                    do{}while(0);\
                })

#define ARRAY_SIZE 3

class MiClstc;

typedef int (MiClstc::*GetPropFunc)(sdm::GenericPayload *payload);
typedef int (MiClstc::*SetPropFunc)(const sdm::GenericPayload &payload);
typedef int (MiClstc::*OpsFunc)(const sdm::GenericPayload &input, sdm::GenericPayload *output);

enum LayerStatus {
  kNewLayer = 0,
  kUpdatedLayer,
  kNoUpdatedLayer,
  kStatusMax = 0xFF,
};

struct CacheLayerStatus {
  std::shared_ptr<const ClstcLayerDetails> layer_details;
  LayerStatus layer_status;
};

class MiClstc: public ClstcInterface, public qClient::BnQClient {
public:
    MiClstc();
    ~MiClstc() {}
    int Init();
    int Deinit();
    int SetParameter(uint32_t prop, const sdm::GenericPayload &payload);
    int GetParameter(uint32_t prop, sdm::GenericPayload *payload);
    int ProcessOps(uint32_t op, const sdm::GenericPayload &input, sdm::GenericPayload *output);
    static int log_level;
    static int mInitNum;
private:
    int GetInterfaceDetails(sdm::GenericPayload *payload);
    int GetSupportedColorFeatures(sdm::GenericPayload *payload);
    int GetSupportedColorTransforms(sdm::GenericPayload *payload);
    int GetLibraryConfig(sdm::GenericPayload *payload);
    int QueryLibraryRequest(sdm::GenericPayload *payload);
    int UpdateStatus(sdm::GenericPayload *payload);
    int SetHwCapabilities(const sdm::GenericPayload &payload);
    int SetDisplayCapabilities(const sdm::GenericPayload &payload);
    int SetTmFeatureControl(const sdm::GenericPayload &payload);
    int SetLibraryConfig(const sdm::GenericPayload &payload);
    int SetBlendInfo(const sdm::GenericPayload &payload);
    int SetTargetState(const sdm::GenericPayload &payload);
    int StageLayers(const sdm::GenericPayload &payload);
    int DropLayers(const sdm::GenericPayload &payload);
    int ResetStack(const sdm::GenericPayload &payload);
    int RetrieveConfig(const sdm::GenericPayload &input, sdm::GenericPayload *output);
    void showDMAPipeConfig(Clstc1DCsc1DLut * config);
    int SetDisplayInfo(const ClstcDisplayInfo &info);
    int PackColorSpaceToConfig(ClstcColorConfiguration* config,
                             const ClstcColorSpaceInfo &input,
                             const ClstcColorSpaceInfo &output);
    status_t notifyCallback(uint32_t command,
                                   const Parcel *input_parcel,
                                   Parcel *output_parcel);
    std::string simplifyLayerName(int layerId);
    int forceScreenRefresh();
    int readLutData(char* path);
    int InitLut(MiImplType disp_id);
    void InitGamma(MiImplType disp_id);
    std::map<ClstcProperty, GetPropFunc> get_func_;
    std::map<ClstcProperty, SetPropFunc> set_func_;
    std::map<ClstcOp, OpsFunc> op_func_;

    MiClstcService *miclstcservice_ = nullptr;
    std::mutex lock_;
    bool init_done_ = false;
    ClstcDisplayType display_type_ = kDisplayTypeUnknown;
    ClstcColorSpaceInfo blend_space_ = {ColorPrimaries_Max, Transfer_Max};
    ClstcBacklightState backlight_state_ = {0};
    std::map<uint32_t, CacheLayerStatus> layer_stack_;
    std::map<std::string, ClstcLayerAnalysis> mLayerUpdateStatus;
    std::map<std::string, std::array<int64_t, ARRAY_SIZE*ARRAY_SIZE>> mPccInfo;
    std::map<std::string, uint32_t>  mLutInfo, mIGCInfo, mGCInfo;

    // HW capabilities
    LutCapabilities dma_igc_caps_;
    MatrixCapabilities dma_csc_caps_;
    LutCapabilities dma_gc_caps_;
    DeCapabilities vig_de_caps_;
    LutCapabilities vig_3Dlut_caps_;
    sp<qService::IQService> qservice_;
    sp<MiParseXml> mParseXml[kMaxNumImpl];
    int LutFileNum = 0;
    std::vector<rgb_entry> LutInfo;
    bool mLutInitialized = false;
    std::vector<uint32_t> igcDataBuf, gcDataBuf;
    int mIgcFileCnt = 0;
    int mGcFileCnt = 0;
};
} // namespace clstc
#endif // __MI_CLSTC_IMPL_H__
