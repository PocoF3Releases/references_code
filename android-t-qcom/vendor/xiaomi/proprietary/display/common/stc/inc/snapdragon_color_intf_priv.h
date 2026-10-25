/*
 * Copyright (c) 2020-2021 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef __SNAPDRAGON_COLOR_PRIV_H__
#define __SNAPDRAGON_COLOR_PRIV_H__

#include <string>
#include <utility>
#include <vector>
#include "disp_color_apis.h"

namespace snapdragoncolor {

//<! Hardware assets strings
const std::string kPbPCC = "PostBlendPCC";
const std::string kPbPa = "PostBlendPa";
const std::string kPbDither = "PostBlendDither";
const std::string kPbPaDither = "PostBlendPaDither";
const std::string kPbDE = "PostBlendDE";
const std::string kPbGameBlob = "PostBlendGameBlob";
const std::string kPbMixerGc = "PostBlendMixerGc";

const uint32_t kPrivatePostBlendHwConfigSupport = 2048;
const uint32_t kPrivatePostBlendPccHwConfig = 2049;
const uint32_t kPrivatePostBlendPaHwConfig = 2050;
const uint32_t kPrivatePostBlendDitherHwConfig = 2051;
const uint32_t kPrivatePostBlendPaDitherHwConfig = 2052;
const uint32_t kPrivatePostBlendDEHwConfig = 2053;
const uint32_t kPrivateDimmingSupport = 2054;
const uint32_t kPrivateSetDimmingPcc = 2055;

//<! PCC
struct PccConfig {
  const uint32_t version = sizeof(struct PccConfig);
  bool enabled = false;
  pcc_coeff_data pcc_info;
};

struct PostBlendPccHwConfig {
  uint32_t pcc_version = sizeof(struct PostBlendPccHwConfig);
  std::vector<std::string> hw_caps;
};

//<! PA

//<! Memory color type
//<!  MEM_COL_SKIN    -- Skin Tone Color
//<!  MEM_COL_SKY,    -- Sky Color
//<!  MEM_COL_FOLIAGE -- Foliage Color
typedef enum {
  MEM_COL_SKIN = 0,
  MEM_COL_SKY,
  MEM_COL_FOLIAGE,
  MEM_COL_MAX,
  MEM_COL_FORCE32BIT = 0x7FFFFFFF
} disp_mem_col_type;

//<! Structure for storing memory color data
//<!  hue        -- Hue setting, units of degrees
//<!  saturation -- Saturation setting, units of percentage
//<!  value      -- Value setting, units of percentage
struct disp_mem_color_config_data {
  int32_t hue;
  float saturation;
  float value;
};

//<! Structure for storing memory color config
//<!  flags -- Reserved
//<!  col   -- Indicates memory color region
//<!  data  -- Memory color region configuration data
struct disp_mem_color_config {
  uint32_t flags;
  struct disp_mem_color_config_data data;
};

//<! Structure for setting the offsets of memory color config
//<!  flags             -- Reserved
//<!  hue_offset        -- Offset of the Hue Trapezoid
//<!  saturation_offset -- Offset of the Saturation Trapezoid
//<!  value_offset      -- Offset of the Value Trapezoid
struct disp_mem_color_config_offset {
  uint32_t flags;
  uint32_t hue_offset;
  uint32_t saturation_offset;
  uint32_t value_offset;
  trapezoid_edge_type val_edge_type;
  trapezoid_edge_type sat_edge_type;
};

//<! Structure for getting the ranges of memory color
//<!  flags      -- Reserved
//<!  hue        -- Supported range and step sizes for hue
//<!  saturation -- Supported range and step sizes for saturation
//<!  value      -- Supported range and step sizes for value
struct disp_mem_color_range {
  uint32_t flags;
  struct disp_range hue;
  struct disp_range_float saturation;
  struct disp_range_float value;
};

//<! Structure for memory color config
struct PAMemColorConfig {
  disp_mem_col_type col;
  disp_mem_color_config mc_cfg;
  disp_mem_color_config_offset mc_cfg_offset;
  disp_mem_color_range mc_range;
};

//<! Structure for PA config
struct DisplayPAConfig {
  uint32_t enable_flags;  // bitmap indicating individual sub-feature enabled or not
  uint32_t op_mode_cache;
  disp_pa_config global_pa_config;
  PAMemColorConfig mem_color_config[MEM_COL_MAX];
  struct {
    disp_six_zone_threshold sz_threshold;
    disp_six_zone_config sz_cfg;
  } six_zone_config;
};

struct PaConfig {
  const uint32_t version = sizeof(struct PaConfig);
  bool enabled = false;
  DisplayPAConfig pa_info;
};

struct PostBlendPaHwConfig {
  uint32_t pa_version = sizeof(struct PostBlendPaHwConfig);
  std::vector<std::string> hw_caps;
};

//<! Dither
struct DitherConfig {
  const uint32_t version = sizeof(struct DitherConfig);
  bool enabled = false;
  dither_coeff_data dither_info;
};

struct PostBlendDitherHwConfig {
  uint32_t dither_version = sizeof(struct PostBlendDitherHwConfig);
  uint32_t num_of_entries = 16;
  std::vector<std::string> hw_caps;
};

//<! PA dither
struct pa_dither_cfg_data {
  uint32_t flags;
  uint32_t strength;
  uint32_t offset_en;
  uint32_t num_entries;
  uint32_t entries[DITHER_ENTRIES_SIZE];
};

struct PaDitherConfig {
  const uint32_t version = sizeof(struct PaDitherConfig);
  bool enabled = false;
  pa_dither_cfg_data pa_dither_info;
};

struct PostBlendPaDitherHwConfig {
  uint32_t pa_dither_version = sizeof(struct PostBlendPaDitherHwConfig);
  uint32_t num_of_entries = 16;
  std::vector<std::string> hw_caps;
};

//<! DetailedEnhancer
struct DETuningConfig {
  const uint32_t version = sizeof(struct DETuningConfig);
  bool enabled = false;
  PPDETuningCfgData de_tuning_info;
};

struct PostBlendDEHwConfig {
  uint32_t de_version = sizeof(struct PostBlendDEHwConfig);
  std::vector<std::string> hw_caps;
};

struct HdrConfig {
  const uint32_t version = sizeof(HdrConfig);
  //<! hdr blob
  std::string blob;
};

struct GameConfig {
  const uint32_t version = sizeof(GameConfig);
  bool enabled = false;
  //<! game blob
  std::string blob;
};

}  // namespace snapdragoncolor
#endif  // __SNAPDRAGON_COLOR_PRIV_H__
