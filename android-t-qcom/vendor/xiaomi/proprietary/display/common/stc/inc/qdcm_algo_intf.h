/*
 * Copyright (c) 2022 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef __QDCM_ALGO_INTF_H__
#define __QDCM_ALGO_INTF_H__

#include <private/generic_intf.h>
#include <private/generic_payload.h>
#include "disp_color_apis.h"

namespace qdcm {

//<! PCC merge
struct PccMergeInputParams {
  const uint32_t version = sizeof(struct PccMergeInputParams);
  //<! left pcc input
  pcc_coeff_data pcc_info_left;
  //<! right pcc input
  pcc_coeff_data pcc_info_right;
};

struct PccCoeffOutputParams {
  const uint32_t version = sizeof(PccCoeffOutputParams);
  //<! merged pcc output
  pcc_coeff_data pcc_info;
};

struct GamutMergeInputParams {
  const uint32_t version = sizeof(struct GamutMergeInputParams);
  //<! left gamut input
  lut3d_info gamut_info_left;
  //<! right gamut input
  lut3d_info gamut_info_right;
};

struct GamutOutputParams {
  const uint32_t version = sizeof(GamutOutputParams);
  //<! merged gamut output
  lut3d_info gamut_info;
};

enum QdcmAlgoOps {
  kPccMerge,
  kGamutMerge,
  kQdcmAlgoOpsMax = 0xFF
};

enum QdcmAlgoParams {
  kQdcmAlgoParamsMax = 0xFF
};

using QdcmAlgoIntf = sdm::GenericIntf<QdcmAlgoParams, QdcmAlgoOps, sdm::GenericPayload>;

extern "C" QdcmAlgoIntf* GetQdcmAlgoIntf();

}  // namespace qdcm
#endif  // __QDCM_ALGO_INTF_H__