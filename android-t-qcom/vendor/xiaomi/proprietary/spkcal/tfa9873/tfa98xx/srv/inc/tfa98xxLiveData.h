/************************************************************************/
/* Copyright 2014-2017 NXP Semiconductors                               */
/* Copyright 2020 GOODIX                                                */
/*                                                                      */
/* GOODIX Confidential. This software is owned or controlled by GOODIX  */
/* and may only be used strictly in accordance with the applicable      */
/* license terms.  By expressly accepting such terms or by downloading, */
/* installing, activating and/or otherwise using the software, you are  */
/* agreeing that you have read, and that you agree to comply with and   */
/* are bound by, such license terms.                                    */
/* If you do not agree to be bound by the applicable license terms,     */
/* then you may not retain, install, activate or otherwise use the      */
/* software.                                                            */
/*                                                                      */
/************************************************************************/

/**
 * @file tfa98xxLiveData.h
 *  @brief Life Data:
 *
 *    This code provides support and reference to read out the actual values from the Tfa98xx:
 *      -gain
 *      -output level
 *      -speaker temperature
 *      -speaker excursion
 *      -speaker resistance
 *      -various statuss flags.
 *
 *      It is similar to how the TFA9887 API is  used  to fill in the bulk of the traces and flags in the Pico GUI.
 *      This code will be implemented as a separate source file and added to climax.
 *
 *      These options will be added to record  the LiveData:
 *       - --record=msInterval
 *       - --count=count
 *       - --output=file.bin (TBS)  //TODO
 *
 *       If an output  file is specified the records will be stored with a header indicating interval, count and version.
 *       Else a line will be printed to the screen per record.
 *
 *  Created on: Jun 7, 2012
 *      Author: Wim Lemmers
 */

#ifndef TFA98XXLIFEDATA_H_
#define TFA98XXLIFEDATA_H_

#include "Tfa98API.h"
#include "tfa_service.h"
#include "tfa98xx_parameters.h"
#include "tfa98xxCalculations.h"

#define API_PARAMETRIC_MODEL            (22) /**< Starting with API version 22 the parametric model is used */
#define MAJOR_API_VERSION_FOR_9888      (2)  /**<For 9888, Starting with API version 44 the parametric model is used */

// Values derived from ITF.xml file
// These indexes and scales are for DSP based X model calculation.
#define BQ_OVERRIDE_IDX       0
#define DS_OVERRIDE_IDX       1
#define BQ_EXP_IDX            2
#define BQ_IDX                3
#define DS_NB_IDX             8
#define DS_EXP_IDX            9
#define DS_IDX                10
#define CONST_SCALE           (double)1
#define XMODEL_SCALE          (double)8388608
#define EXFILTERS_SIZE        25
#define DSP_READBUF_SIZE      510//512

#define PARAMETRIC_X_FILTER_COUNT       (4u)     /**< There are 4 parametric X filters. */
#define PARAMETRIC_X_FILTER_COEFFS      (6u)     /**< Each parametric X filter has 6 coeffs. */
#define PARAMETRIC_X_FILTERS_SIZE       (PARAMETRIC_X_FILTER_COEFFS * PARAMETRIC_X_FILTER_COUNT)

#define PARAMETRIC_Z_FILTER_COUNT       (1u)     /**< There is only 1 parametric Z filter. */
#define PARAMETRIC_Z_FILTER_COEFFS      (128u)   /**< Each parametric Z filter has 128 coeffs. */
#define PARAMETRIC_Z_FILTERS_SIZE       (PARAMETRIC_Z_FILTER_COEFFS * PARAMETRIC_Z_FILTER_COUNT)

/**
 * DRC extension
 */
typedef struct tfa98xx_DrcStateInfo {
	float GRhighDrc1[2];
	float GRhighDrc2[2];
	float GRmidDrc1[2];
	float GRmidDrc2[2];
	float GRlowDrc1[2];
	float GRlowDrc2[2];
	float GRpostDrc1[2];
	float GRpostDrc2[2];
	float GRblDrc[2];
} tfa98xx_DrcStateInfo_t;

typedef struct LiveDataList {
	char *address;
	int isMemtrackItem;
} LiveDataList_t;

/**
 * life data structure
 *  note the order is in line with the graphs on the pico GUI
 */
typedef struct tfa98xx_LiveData {
        unsigned short statusRegister;/**  status flags (from i2c register ) */
        unsigned short statusFlags;   /**  Masked bit word, see Tfa98xx_SpeakerBoostStatusFlags */
        float agcGain;                /**  Current AGC Gain value */
        float limitGain;              /**  Current Limiter Gain value */
        float limitClip;              /**  Current Clip/Lim threshold */
        float batteryVoltage;         /**  battery level (from i2c register )*/
        int   speakerTemp;            /**  Current Speaker Temperature value */
        short icTemp;                 /**  Current ic/die Temperature value (from i2c register ) */
        float boostExcursion;         /**  Current estimated Excursion value caused by Speakerboost gain control */
        float manualExcursion;        /**  Current estimated Excursion value caused by manual gain setting */
        float speakerResistance;      /**  Current Loudspeaker blocked resistance */
        int shortOnMips;			  /**  increments each time a MIPS problem is detected on the DSP */
        int DrcSupport;				  /**  Is the DRC extension valid */
        struct Tfa98xx_DrcStateInfo drcState;   /**  DRC extension */       
} tfa98xx_LiveData_t;

/** Speaker Model structure. */
/* All parameters related to the Speaker are collected into this structure*/
typedef struct SPKRBST_SpkrModel {
        double pFIR[128];       /* Pointer to Excurcussion  Impulse response or 
                                   Admittance Impulse response (reversed order!!) */
        int Shift_FIR;          /* Exponent of HX data */
        float leakageFactor;    /* Excursion model integration leakage */
        float ReCorrection;     /* Correction factor for Re */
        float xInitMargin;      /*(1)Margin on excursion model during startup */
        float xDamageMargin;    /* Margin on excursion modelwhen damage has been detected */
        float xMargin;          /* Margin on excursion model activated when LookaHead is 0 */
        float Bl;               /* Loudspeaker force factor */
        int fRes;               /*(1)Estimated Speaker Resonance Compensation Filter cutoff frequency */
        int fResInit;           /* Initial Speaker Resonance Compensation Filter cutoff frequency */
        float Qt;               /* Speaker Resonance Compensation Filter Q-factor */
        float xMax;             /* Maximum excursion of the speaker membrane */
        float tMax;             /* Maximum Temperature of the speaker coil */
        float tCoefA;           /*(1)Temperature coefficient */
} SPKRBST_SpkrModel_t;          /* (1) this value may change dynamically */

/*** +++ Added specific to Distortion filter support +++ ****/

/** The CC Bits in the Command ID: DC, DP, DS and SC. 
   * These correspond to bits 16 till 19 in the command ID respectively. 
   */
#define TFADSP_CC_BIT_DC (1) /** DC or Disable common configuration: skip applying configuration relevant for both primary and secondary channels */
#define TFADSP_CC_BIT_DP (2) /** DP or Disable Primary channel configuration: skip applying primary channel specific configuration */
#define TFADSP_CC_BIT_DS (4) /** DS or Disable Secondary channel configuration: skip applying secondary channel specific configuration */
#define TFADSP_CC_BIT_SC (8) /** SC or Same Configuration: apply same configuration to both channels, only one set of configuration parameters will be sent */

/* TODO: Move the M-IDs and P-IDs from tfadsp2_dsp_fw.h to this file*/

/** P-ID or ParamID for the commands in the firmware. 
  * P-ID identifies the action to be executed by the FW module. The P-ID is 
  * tightly linked to the M-ID, and its signification may be different for  
  * each module. There are basically 2 classes of actions 'set' and 'get'.
  */
#define SB_PARAM_GET_DISTO_DYNAMICS    (0x80 + 17)

/** Basic type definitions */
typedef int plma_int;
typedef int plma_fix;

#define DISTOREDUCTION_GETDYN_MAX_BQ_SLOTS (5)
#define BIQUAD_COEFFS_PER_SECTION_PLUSHR (BIQUAD_COEFF_SIZE) 

typedef struct s_DistoReduction_DynamicInfo_Filter{   
	plma_int biquadCnt;   
	plma_fix sBiquadPar[DISTOREDUCTION_GETDYN_MAX_BQ_SLOTS][BIQUAD_COEFFS_PER_SECTION_PLUSHR];
} DistoReduction_DynamicInfo_Filter;

/* DistoReduction Dynamic Info structure */
typedef struct s_DistoReduction_DynamicInfo{   
	DistoReduction_DynamicInfo_Filter sDetectLowFilter;   
	DistoReduction_DynamicInfo_Filter sDetectHighFilter;   
	DistoReduction_DynamicInfo_Filter sGainFilter;
}DistoReduction_DynamicInfo;

/* Size of the DistDynamics response for a single channel in bytes */
#define DISTO_DYNAMICS_SIZE (sizeof(DistoReduction_DynamicInfo))
/* Size of the DistDynamics response for a single channel in number of 24-bits words */
#define DISTO_DYNAMICS_WIRE_SIZE (sizeof(DistoReduction_DynamicInfo) / sizeof(plma_fix) * 3)

/*** --- Added specific to Distortion filter support --- ****/

/**
 * Compare the item to a the pre-defined state info list for max1
 */
int state_info_memtrack_compare(const char* liveD_name, int* number);

/**
 * To get the names of items enumerated in the livedata section
 * all fields from the tfa98xx_LiveData structure.
 * @param dev_idx the device index
 * @param strings char array with list of names
 */
Tfa98xx_Error_t tfa98xx_get_live_data_items(int dev_idx, char *strings);

/**
 *  Get the raw device data from the device for those items refered in livedata section
 *  the devices will be opened //TODO check for conflicts if already open
 *  @param dev_idx to select the device index
 *  @param bytes array of floating point values. 
 *			Each index corresponds to location of livedata element to monitor
 *  @param length the number of items
 *  @return last error code
 */
Tfa98xx_Error_t tfa98xx_get_live_data_raw(int dev_idx, unsigned char *bytes, int length);

/**
 *  Set the human readable data from the device for those items refered in livedata section
 *  This is required so that the DSP knows which items to monitor with the tfa98xx_get_live_data function
 *  @param dev_idx to select the device index
 *  @return last error code
 */
Tfa98xx_Error_t tfa98xx_set_live_data(int dev_idx);

/**
 *  Get the human readable data from the device for those items refered in livedata section
 *  The raw device data is adjusted with scaling factor in the livedata item.
 *  @param dev_idx to select the device index
 *  @param live_data array with the memtrack data
 *  @param nr_of_items pointer with the number of items
 *  @return last error code
 */
Tfa98xx_Error_t tfa98xx_get_live_data(int dev_idx, float *live_data, int *nr_of_items);

typedef enum {
        SPEAKER_MODEL_Z = 0,
        SPEAKER_MODEL_X = 1,
        SPEAKER_MODEL_X_DSP = 2
} SpeakerModel_t;

/**
 * Return the frequency response of the selected speaker model as a list of strings.
 * @param dev_idx to select the device index
 * @param strings target to store the frequency response
 * @param maxlength the maximum number of floats that can be stored in strings
 * @param model the type of speaker model frequency response to calculate
 */
int tfa98xxPrintSpeakerModel(int dev_idx, float *strings, int maxlength, SpeakerModel_t model);

int tfa_print_speakermodel_from_speakerfile(int rev_id, tfaSpeakerFile_t *spk, float *strings, int maxlength, int xmodel);
int tfa_print_speakermodel_from_lsmodelMsg(int rev_id, tfaMsgFile_t *lsmodelMsg, float *strings, int maxlength, int xmodel);

/* (Deprecated, please use: tfa98xxCalculateMBDrcModel) Get the DRC Model Freq Range data from the device 
 * @param dev_idx to select the device index
 * @param subband_selector the subband that has been selected as hex value (bitmask)
 * @param SbFilt_buf the buffer for the SbFilt
 * @param CombGain_buf the buffer for the CombGain
 * @param maxlength indicated the maximum length of the buffers (currently all 3 should be the same length)
 */
int tfa98xxPrintMBDrcModel(int dev_idx, int subband_selector,
	s_subBandDynamics *sbDynamics, float *SbFilt_buf, float *CombGain_buf, int maxlength);


/* 
 * @param subband_selector the subband that has been selected as hex value (bitmask)
 * @param SbFilt_buf the buffer for the SbFilt
 * @param CombGain_buf the buffer for the CombGain
 * @param maxlength indicated the maximum length of the buffers (currently all 3 should be the same length)
 * @param subBandFilterTypes 0 select peak type filter, any other value selects shelving type filter. 
 */
int tfa98xxCalculateMBDrcModel(int dev_idx, int subband_selector,
	s_subBandDynamics *sbDynamics, float *SbFilt_buf, float *CombGain_buf, int maxlength, int *subBandFilterTypes);


/* Computes the DRC transfer function based on the inputs 
 * @param s_DRC_Params the struct containing the GUI information 
 * @param DRC1 the buffer for the DRC1 results
 * @param maxlength indicated the maximum length of the buffers (currently all 3 are the same length)
 */
int tfa98xxCompute_DRC_time_domain(s_DRC_Params *paramsDRC, float *DRC1, int maxlength);

/**
 *  retrieve the speaker model and return an array of values
 *  @param dev_idx to select the device index
 *  @param data pointer to first element of a data array (int data[150])
 *  @param waveform pointer to first element of a waveform data array (double waveform[128])
 *  @param xmodel select 1 = xcursion model, 0 = impedance model
 *  @return last error code
 */
int tfa98xxGetWaveformValues(int dev_idx, int *data, double *waveform, int xmodel);

/**
 *  retrieve the speaker x-model from dsp and return an array of values
 *  @param dev_idx to select the device index
 *  @param data pointer to first element of a data array (int data[150])
 *  @param waveform pointer to first element of a waveform data array (double waveform[128])
 *  @return last error code
 */
int tfa98xxGetDSPXModelValues(int dev_idx, int *data, double *waveform);

/**
 * Compute the frequency response of the parametric x model filters.
 * @param dev_idx to select the device index
 * @param waveform pointer to first element of a waveform data array
 */
int tfa98xxGetDSPParametricXModelValues(int dev_idx, double *waveform);

/**
 * Compute the frequency response of the parametric z model filter.
 * @param dev_idx to select the device index
 * @param waveform pointer to first element of a waveform data array
 */
int tfa98xxGetDSPParametricZModelValues(int dev_idx, double *waveform);

int tfa98xxGetLoudSpeakerModelFS(int dev_idx);

/**
 *  Retrieve the magnitude of the frequency responses of the filters 
 *  in the distortion reduction module of the DSP algorithm.
 *  @param dev_idx to select the device index
 *  @param chan_idx to select the channel to return (0 for primary, 1 for secondary)
 *  @param freq_response_length determines the length of the frequency response returned
 *         This value is usually a power of 2. A default value is 64.
 *         freq_response_length needs to be larger than 0.
 *  @param detect_low_response pointer to a pre-allocated buffer of size freq_response_length.
 *         This buffer will be filled with the magnitude of the frequency response of the
 *         detect low filter
 *  @param detect_high_response pointer to a pre-allocated buffer of size freq_response_length.
 *         This buffer will be filled with the magnitude of the frequency response of the
 *         detect high filter
 *  @param gain_response pointer to a pre-allocated buffer of size freq_response_length.
 *         This buffer will be filled with the magnitude of the frequency response of the
 *         instantaneous gain filter
 *  @return last error code
 * 
 *  Note: The frequency response is calculated for frequencies distributed uniformly from 0Hz to SampleRate/2
 */
int tfa98xxGetDSPDistoFilterResponses(int dev_idx, int chan_idx, int freq_response_length,
    double *detect_low_response, double *detect_high_response, double *gain_response);


#endif /* TFA98XXLIFEDATA_H_ */
