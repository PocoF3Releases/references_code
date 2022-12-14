/*--------------------------------------------------------------------------
Copyright (c) 2019 Qualcomm Technologies, Inc.
All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

Copyright (c) 2009, 2015, 2018 The Linux Foundation. All rights reserved.
--------------------------------------------------------------------------*/
/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file QOMX_AudioIndexExtensions.h
  This module contains the index extensions for Audio

*//*========================================================================*/


#ifndef __H_QOMX_AUDIOINDEXEXTENSIONS_H__
#define __H_QOMX_AUDIOINDEXEXTENSIONS_H__

/*========================================================================

                     INCLUDE FILES FOR MODULE

========================================================================== */
#include <OMX_Core.h>

/*========================================================================

                      DEFINITIONS AND DECLARATIONS

========================================================================== */

#if defined( __cplusplus )
extern "C"
{
#endif /* end of macro __cplusplus */

/**
 * Enumeration used to define vendor extensions for
 * audio. The audio extensions occupy a range of
 * 0x7F100000-0x7F1FFFFF, inclusive.
 */
typedef enum QOMX_AUDIO_EXTENSIONS_INDEXTYPE
{
    QOMX_IndexParamAudioAmrWbPlus       = 0x7F200000,
    QOMX_IndexParamAudioWma10Pro        = 0x7F200001,
    QOMX_IndexParamAudioSessionId       = 0x7F200002,
    QOMX_IndexParamAudioVoiceRecord     = 0x7F200003,
    QOMX_IndexConfigAudioDualMono       = 0x7F200004,
    QOMX_IndexParamAudioAc3             = 0x7F200005,
    QOMX_IndexParamAudioAc3PostProc     = 0x7F200006,
    QOMX_IndexParamAudioAacSelectMixCoef = 0x7F200007,
    QOMX_IndexParamAudioAlac            = 0x7F200008,
    QOMX_IndexParamAudioApe             = 0x7F200009,
    QOMX_IndexParamAudioFlacDec         = 0x7F20000A,
    QOMX_IndexParamAudioDsdDec          = 0x7F20000B,
    QOMX_IndexParamAudioMpegh           = 0x7F20000C,
    QOMX_IndexParamAudioMpeghHeader     = 0x7F20000D,
    QOMX_IndexParamAudioChannelMask     = 0x7F20000E,
    QOMX_IndexParamAudioBinauralMode    = 0x7F20000F,
    QOMX_IndexParamAudioRotation        = 0x7F200010,
    QOMX_IndexParamAudioUnused          = 0x7F2FFFFF
} QOMX_AUDIO_EXTENSIONS_INDEXTYPE;

#if defined( __cplusplus )
}
#endif /* end of macro __cplusplus */

#endif /* end of macro __H_QOMX_AUDIOINDEXEXTENSIONS_H__ */
