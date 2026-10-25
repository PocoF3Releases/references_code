/******************************************************************************
 *
 *  Copyright (C) 2011-2012 Broadcom Corporation
 *  Copyright 2018-2019 NXP.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif

int GetNxpConfigStrValue(const char* name, char* p_value, unsigned long len);
int GetNxpConfigNumValue(const char* name, void* p_value, unsigned long len);
int GetNxpConfigByteArrayValue(const char* name, char* pValue,long bufflen, long *len);
int GetNxpConfigUciByteArrayValue(const char* name, char* pValue,long bufflen, long *len);
//xiaomi modify start
int GetCaliConfigByteArrayValue(const char* name, char* pValue, long bufflen, long *len);
int GetTofConfigByteArrayValue(const char* name, char* pValue, long bufflen, long *len);
int GetPDOAConfigByteArrayValue(const char* name, char* pValue,long bufflen, long *len);
//xiaomi modify end
#ifdef __cplusplus
};
#endif

#define NAME_UWB_BOARD_VARIANT_CONFIG "UWB_BOARD_VARIANT_CONFIG"
#define NAME_UWB_BOARD_VARIANT_VERSION "UWB_BOARD_VARIANT_VERSION"
#define NAME_UWB_CORE_EXT_DEVICE_DEFAULT_CONFIG "UWB_CORE_EXT_DEVICE_DEFAULT_CONFIG"
#define NAME_UWB_DEBUG_DEFAULT_CONFIG "UWB_DEBUG_DEFAULT_CONFIG"
#define NAME_ANTENNA_PAIR_SELECTION_CONFIG1 "ANTENNA_PAIR_SELECTION_CONFIG1"
#define NAME_ANTENNA_PAIR_SELECTION_CONFIG2 "ANTENNA_PAIR_SELECTION_CONFIG2"
#define NAME_ANTENNA_PAIR_SELECTION_CONFIG3 "ANTENNA_PAIR_SELECTION_CONFIG3"
#define NAME_ANTENNA_PAIR_SELECTION_CONFIG4 "ANTENNA_PAIR_SELECTION_CONFIG4"
#define NAME_ANTENNA_PAIR_SELECTION_CONFIG5 "ANTENNA_PAIR_SELECTION_CONFIG5"
#define NAME_NXP_CORE_CONF_BLK "NXP_CORE_CONF_BLK_"
#define NAME_UWB_FW_DOWNLOAD_LOG "UWB_FW_DOWNLOAD_LOG"
#define NAME_NXP_UWB_XTAL_38MHZ_CONFIG "NXP_UWB_XTAL_38MHZ_CONFIG"

#define NAME_NXP_UWB_PROD_FW_FILENAME "NXP_UWB_PROD_FW_FILENAME"
#define NAME_NXP_UWB_DEV_FW_FILENAME "NXP_UWB_DEV_FW_FILENAME"
//xiaomi modify start
#define NAME_UWB_FACTORY_CALIBRATION_CONFIG "DATA1"
#define NAME_UWB_FACTORY_TOF_CALIBRATION_CONFIG "TOF"
#define NAME_UWB_RSSI_CH5_CONFIG "NXP_UWB_RSSI_CH5_CONFIG"
#define NAME_UWB_RSSI_CH9_CONFIG "NXP_UWB_RSSI_CH9_CONFIG"
#define NAME_UWB_PDOA_CALIBRATION_CONFIG "PDOA"
//xiaomi modify end

/* default configuration */
#define default_storage_location "/data/vendor/uwb"

#endif
