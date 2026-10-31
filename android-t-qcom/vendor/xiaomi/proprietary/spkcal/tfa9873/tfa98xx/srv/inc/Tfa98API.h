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

#ifndef TFA98API_H
#define TFA98API_H

#include "tfa_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/* the number of biquads supported */
#define TFA98XX_BIQUAD_NUM              10

typedef enum Tfa98xx_Error Tfa98xx_Error_t;
typedef enum Tfa98xx_AmpInputSel Tfa98xx_AmpInputSel_t;
typedef enum Tfa98xx_OutputSel Tfa98xx_OutputSel_t;
typedef enum Tfa98xx_StereoGainSel Tfa98xx_StereoGainSel_t;
typedef enum Tfa98xx_DAI Tfa98xx_DAI_t;
typedef enum Tfa98xx_Channel Tfa98xx_Channel_t;
typedef enum Tfa98xx_Mode Tfa98xx_Mode_t;

typedef enum Tfa98xx_Mute Tfa98xx_Mute_t;
typedef enum Tfa98xx_SpeakerBoostStatusFlags Tfa98xx_SpeakerBoostStatusFlags_t;

typedef struct Tfa98xx_DrcStateInfo Tfa98xx_DrcStateInfo_t;
typedef struct Tfa98xx_StateInfo Tfa98xx_StateInfo_t;

/* possible memory values for DMEM in CF_CONTROLs */
typedef enum Tfa98xx_DMEM Tfa98xx_DMEM_e;

/* register definition structure */
typedef struct regdef regdef_t;

typedef unsigned char Tfa98xx_SpeakerParameters_t[TFA2_SPEAKERPARAMETER_LENGTH];

#define TFA_MAX_DEVICES  6
#define TFA_NAME_MAX_LEN  20
#define TFA_FWAPI_MAX_LEN  4

#define TFA_HEADER_INFO_LEN   16

typedef struct tfa{
    unsigned char TfaName[TFA_NAME_MAX_LEN];
    unsigned char FWAPIVersion[TFA_FWAPI_MAX_LEN];
    unsigned char FS;
}tfa_t;

typedef struct tfaDetails_t
{
  unsigned short         numOfDevices;
  unsigned short         numOfChannels;
  tfa_t    tfaDetails[TFA_MAX_DEVICES];
    
}tfaDetails_t;

typedef struct tfaMsgHdrInfo
{
  char API_Version[TFA_HEADER_INFO_LEN];
  char SamplingRate[TFA_HEADER_INFO_LEN];
  char CustomerID[TFA_HEADER_INFO_LEN];
    
}tfaMsgHdrInfo_t;



/**
 * Ensures the I2C registers are loaded with good default values for optimal operation.
 * @param dev_idx to indicate the device index
 */
Tfa98xx_Error_t Tfa98xx_Init(int dev_idx);


/**
 * Resets tfa98xx device (this is proper reset by using I2C register bit)
 *
 * @param dev_idx device index to indicate tfa98xx instance
 *
 * @return Tfa98xx_Error_Ok when successful, error otherwise.
 */
Tfa98xx_Error_t Tfa98xx_Reset(int dev_idx);

/**
 * Control the volume I2C register.
 * @param dev_idx to select the device index
 * @param vollevel volume in level.  must be between 0 and 255
 */
Tfa98xx_Error_t Tfa98xx_SetVolumeLevel(int dev_idx, unsigned short vollevel);

/**
 * Select the I2S input channel the DSP should process.
 * The TFA98xx defaults to TFA98xx_Channel_L.
 * @param dev_idx to select the device index
 * @param channel, channel selection: see Tfa98xx_Channel_t in the header file
 *   - TFA98xx_Channel_L: I2S left channel
 *   - TFA98xx_Channel_R: I2S right channel
 *   - TFA98xx_Channel_L_R: I2S (left + right channel)/2
 */
Tfa98xx_Error_t Tfa98xx_SelectChannel(int dev_idx, Tfa98xx_Channel_t channel);

/**
 * A soft mute/unmute is executed, the duration of which depends on an advanced parameter.
 * NOTE: before going to powerdown mode, the amplifier should be stopped first to ensure clean transition without artifacts.
 * Digital silence or mute is not sufficient.
 * The TFA98xx defaults to TFA98xx_Mute_Off.
 * @param dev_idx to select the device index
 * @param mute state: see Tfa98xx_Mute_t type in the header file
 *   - TFA98xx_Mute_Off: leave the muted state.
 *   - TFA98xx_Mute_Digital: go to muted state, but the amplifier keeps running.
 *   - TFA98xx_Mute_Amplifier: go to muted state and stop the amplifier.  This will consume lowest power.
 *	 This is also the mute state to be used when powering down or changing the sample rate.
 */
Tfa98xx_Error_t Tfa98xx_SetMute(int dev_idx, Tfa98xx_Mute_t mute);

/**
 * Check whether the DSP supports DRC.
 * @param dev_idx to select the device index
 * @param *pbSupportDrc: =1 when DSP supports DRC,
 *        *pbSupportDrc: =0 when DSP doesn't support it
 */
Tfa98xx_Error_t Tfa98xx_DspSupportDrc(int dev_idx, int *pbSupportDrc);


/**
 * Set or clear DSP reset signal.
 * @param dev_idx to select the device index
 * @param state requested state of the reset signal
 */
Tfa98xx_Error_t Tfa98xx_DspReset(int dev_idx, int state);

/**
 * Check the state of the DSP subsystem to determine if the subsystem is ready for access.
 * Normally this function is called after a powerup or reset when the higher layers need to assure that the subsystem can be safely accessed.
 * @param dev_idx to select the device index
 * @param ready pointer to state flag
 * @return Non-zero if stable
 */
Tfa98xx_Error_t Tfa98xx_DspSystemStable(int dev_idx, int *ready);

/**
 * The TFA98XX has provision to patch the ROM code. This API function provides a means to do this.
 * When a patch is needed, TFA will provide a file. The contents of that file have to be passed this this API function.
 * As patching requires access to the DSP, this is only possible when the TFA98xx has left powerdown mode.
 * @param dev_idx to select the device index
 * @param patchLength: size of the patch file
 * @param patchBytes array of bytes: the contents of the patch file
 */
Tfa98xx_Error_t Tfa98xx_DspPatch(int dev_idx, int patchLength, const unsigned char *patchBytes);

/**
 * Use this for a free speaker or when saving the output of the online estimation process across cold starts (see use case in chapter 4).
 * The exact content is considered advanced parameters and detailed knowledge of this is not needed to use the API.
 * TFA will give support for determining this for 3rd party speakers.
 * @param dev_idx to select the device index
 * @param length: the size of the array in bytes
 * @param *pSpeakerBytes: Opaque array of Length bytes, read from a .speaker file that was generated by the Pico GUI
 */
Tfa98xx_Error_t Tfa98xx_DspWriteSpeakerParameters(int dev_idx, int length, const unsigned char *pSpeakerBytes);

/**
 * This API function loading a predefined preset from a file.
 * The parameters are sample rate independent, so when changing sample rates it is no required to load a different preset.
 * It is allowed to be called while audio is playing, so not needed to mute.
 * For more details about the preset parameters, see §6.1.
 * @param dev_idx to select the device index
 * @param length: the size of the array in bytes
 * @param *pPresetBytes: Opaque array of Length bytes, e.g. read from a .preset file that was generated by the Pico GUI
 */
Tfa98xx_Error_t Tfa98xx_DspWritePreset(int dev_idx, int length, const unsigned char *pPresetBytes);

/**
 * This API function allows reading an arbitrary I2C register of the TFA98xx.
 * @param dev_idx to select the device index
 * @param subaddress: 8 bit subaddress of the I2C register
 * @param pValue the value read from the I2C register
 * @return Tfa98xx_Error_t 16 bit value that was stored in the selected I2C register
 */
Tfa98xx_Error_t Tfa98xx_ReadRegister16(int dev_idx,unsigned char subaddress, unsigned short *pValue);

/**
 * This API function allows writing an arbitrary I2C register of the TFA98XX.
 * @param dev_idx to select the device index
 * @param subaddress: 8 bit subaddress of the I2C register
 * @param value: 16 bit value to be stored in the selected I2C register
 */
Tfa98xx_Error_t Tfa98xx_WriteRegister16(int dev_idx,
				unsigned char subaddress,
				unsigned short value);

Tfa98xx_Error_t Tfa98xx_DspReadMem(int dev_idx,
				unsigned int start_offset,
				int num_words, int *pValues);

Tfa98xx_Error_t Tfa98xx_DspGetMem(int dev_idx,
				int memoryType, int offset,
				int length, unsigned char bytes[]);

Tfa98xx_Error_t Tfa98xx_DspSetMem(int dev_idx,
				int memoryType, int offset,
				int length, int value);

Tfa98xx_Error_t Tfa98xx_DspWriteMem(int dev_idx,
				unsigned short address,
                int value, int memtype);

/**
* This API function allows writing to memory with start offset and length as argument
* @param dev_idx to select the device index
* @param which_mem: type of memory 1: xmem 2: ymem 3: iomem
* @param start_offset: start of xmem write
* @param length: memory locations to be written
* @param bytedata: content of data to be written (e.g. 1 xmem is 3 bytes of data)
*/
Tfa98xx_Error_t Tfa98xx_mem_write(int dev_idx,
				enum Tfa98xx_DMEM mem_type,
				unsigned short start_offset,
				int length, const unsigned char *bytedata);

Tfa98xx_Error_t Tfa98xx_ReadData(int dev_idx,
				unsigned char subaddress,
				int num_bytes,
				unsigned char data[]);

Tfa98xx_Error_t Tfa98xx_DspReadSpeakerParameters(int dev_idx,
				int length,
				unsigned char *pSpeakerBytes);

Tfa98xx_Error_t Tfa98xx_DspReadExcursionModel(int dev_idx,
				int length,
				unsigned char *pSpeakerBytes);

Tfa98xx_Error_t Tfa98xx_DspReadExcursionModelCoeffs(int dev_idx,
				int length,
				unsigned char *pCoeffs);

Tfa98xx_Error_t Tfa98xx_DspReadExcursionFilters(int dev_idx,
				int length,
				unsigned char *pData);

Tfa98xx_Error_t Tfa98xx_DspReadXFilterCoeffs(int dev_idx,
                                             int length,
                                             unsigned char *pCoeffs);

Tfa98xx_Error_t Tfa98xx_DspReadZFilterCoeffs(int dev_idx,
                                             int length,
                                             unsigned char *pCoeffs);

Tfa98xx_Error_t Tfa98xx_WriteData(int dev_idx,
				 unsigned char subaddress, int num_bytes, unsigned char data[]);
/**
 * If needed, this API function can be used to get a text version of the error code
 * @param error: An error code of type TFA98xx_Error_t
 * @return Pointer to a text version of the error code
 */
const char *Tfa98xx_GetErrorString(Tfa98xx_Error_t error);

/**
 * This API function retreives the slave adress for the device at dev_idx in
 * the container file
 * @param dev_idx to select the device index
 * @param pslave_addr the device slave address
 */
Tfa98xx_Error_t Tfa98xx_ContGetSlave(int dev_idx, unsigned char *pslave_addr);

/**
 * This API function retreives the device name of the device at dev_idx in
 * the container file
 * @param dev_idx to select the device index
 * @param length length of buffer to contain the device name
 * @param pbuffer device name buffer
 */
Tfa98xx_Error_t Tfa98xx_ContDeviceName(int dev_idx, int length, char *pbuffer);

/**
 * This API function retreives the profile name of the profile at prof_idx
 * for the device at dev_idx in the container file
 * @param dev_idx to select the device index
 * @param prof_idx to select the profile index
 * @param length length of buffer to contain the profile name
 * @param pbuffer profile name buffer
 */
Tfa98xx_Error_t Tfa98xx_ContProfileName(int dev_idx, int prof_idx, int length, char *pbuffer);

/**
 * This API function writes a parameter file to the device at dev_idx in
 * the container file
 * @param dev_idx to select the device index
 * @param pfile the file buffer
 * @param VstepIndex volume step index
 */
Tfa98xx_Error_t Tfa98xx_ContWriteFile(int dev_idx, void *pfile, int VstepIndex);

/**
 * This API function retrieves the value of a bitfield of a device at dev_idv in
 * the container file
 * @param dev_idx to select the device index
 * @param bitfield bitfield enum
 * @param pvalue bitfiled value
 */
Tfa98xx_Error_t Tfa98xx_GetBitfield(int dev_idx, const unsigned short bitfield, unsigned short *pvalue);

/*
 * the functions that used to be in the tfa layer but are not needed in the driver need to be available for test and diag
 *
 */
Tfa98xx_Error_t tfa98xx_dsp_read_drc(int dev_idx, int length, unsigned char *pDrcBy);
Tfa98xx_Error_t tfa98xx_dsp_config_parameter_count(int dev_idx, int *pParamCount);
Tfa98xx_Error_t tfa98xx_dsp_read_preset(int dev_idx, int length, unsigned char *pPresetBytes);
Tfa98xx_Error_t tfa98xx_supported_speakers(int dev_idx, int *spkr_count);
int tfa98xx_is_cold(int dev_idx);
int tfa98xx_cf_enabled(int dev_idx);

/**
 * This API function writing dsp messages.
 * The parameters are dev index, length of data and data
 * @param dev_idx to select the device index
 * @param length: the size of the buffer in bytes
 * @param *buffer: buffer to be written in dsp
 * @return Tfa98xx_Error_t error code returned
 */
Tfa98xx_Error_t Tfa98xx_dsp_msg_write(int dev_idx, int length, const unsigned char *buffer);

/**
* This API retrieves FW API info from DSP(e.g.coolflux or TFADSP)
* @param dev_idx
* @param firmware_version FW API version retrieved
* @return int error code returned
*/
Tfa98xx_Error_t Tfa98xx_get_dsp_fw_api_version(int dev_idx, unsigned char *firmware_version);

/**
* This API initializes the device based on information given in field tfahal in device structure of container file.
* Alternatively it takes device information as argument. Info from container file leading
* The parameters are device_arg.
* @param device_arg device name
* @return int error code returned
*/
int device_init(const char *device_arg);
/**
* This API initializes the hal plugin based on information given in field tfahal in device structure of container file.
* The parameters are device_arg.
* @param device_arg device name
* @return int error code returned
*/
int device_init_hal(const char *device_arg);

/**
* This API closes the device
* @param void
* @return int error code returned
*/
int device_close(void);

Tfa98xx_Error_t Tfa98xx_stopplayback(int dev_idx);

Tfa98xx_Error_t Tfa98xx_startplayback(int dev_idx, char *data);

Tfa98xx_Error_t Tfa98xx_writeread_dsp(int dev_idx , int command_length, void *command_buffer,
					int result_length, void *result_buffer);

Tfa98xx_Error_t Tfa98xx_set_pin(int dev_idx, int pin, int value);

Tfa98xx_Error_t Tfa98xx_get_pin(int dev_idx, int pin, int *value);

/* Tracing/logging API */

/* TODO: Some of the leves are not connected yet in HostSDK */
/* Bits lower than LOG_SERVICE are reserved. They were used previously by Climax */
#define TFA98XX_VERBOSITY_SERVICE  0x0020  /**< Enable logging from service layer */
#define TFA98XX_VERBOSITY_TFA      0x0040  /**< Enable logging from tfa layer */
#define TFA98XX_VERBOSITY_HAL      0x0080  /**< Enable logging from HAL layer */
#define TFA98XX_VERBOSITY_PLUGINS  0x0100  /**< Enable logging from plugins */
#define TFA98XX_VERBOSITY_PROTOCOL 0x0200  /**< Enable logging from protocol level */
#define TFA98XX_VERBOSITY_SYSTEM   0x0400  /**< Enable logging from system services */
#define TFA98XX_VERBOSITY_TIMING   0x0800  /**< Enable timing logs */

/** Set output stream for traces
 *
 * Function sets output stream for tracing functionality delivered from HAL module.
 * Tracing messages covers operations directed to hardware. Messges format
 * is kept same as format on wire (I2C).
 *
 *  @param[in] stream I/O stream (standard C library) used to print messages.
 *                    Stream can be NULL. When set to NULL stdout will be used.
 *
 *  @return TFA_I2C_Ok when succeeded, error otherwise.
 */
Tfa98xx_Error_t Tfa98xx_tracing_set_file(FILE *stream);

/** Set tracing state
 *
 * Function sets tracing functionality state. When ON messages will be printed
 * to stream set by user (or stdout by default).
 *
 *  @param[in] state 0 for OFF. Any other value ON.
 *
 *  @return TFA_I2C_Ok when succeeded, error otherwise.
 */
Tfa98xx_Error_t Tfa98xx_tracing_toggle(unsigned state);

/** Set logging mask
 *
 * Function sets logging mask. Mask is used to enable and disable verbose output
 * from HostSDK submodules (tfa, service, etc.).
 * Items available are defined as TFA98XX_VERBOSITY_XXXX macros
 *
 *  @param[in] mask 32bit unsigned integer. Each bit controls
 *                  verbosity of different HostSDK submodule.
 *
 *  @return TFA_I2C_Ok when succeeded, error otherwise.
 */
Tfa98xx_Error_t Tfa98xx_verbosity_set(unsigned mask);

/** Indicates if device is haptic
 *
 * @param[in] dev_idx to select the device index
 * @param[out] is_haptic returns device id to indicate haptic is support
 *
 *  @return TFA_I2C_Ok when succeeded, error otherwise.
 */
Tfa98xx_Error_t Tfa98xx_is_haptic(int dev_idx, int *is_hapitc);

/** Get the Details of the attached TFA .
 * 
 *
 * Function returns the details of the attached TFA like TFA name and version(Eg: TFA9894N2A1), Firmware API version (Eg:5.15.0),
 * Number of devices and number of channels. 
 * Number of devices: Is the number of TFA chips connected. For stereo, the number of devices will be returned as '2' with each returning its FW API version.
 * Even haptic devices gets counted in the number of devices returned.
 * Number of channels represents the number of audio channels supported by the devices. Application shall choose mono or stereo based on the number of channels returned/.
 * Firmware API version is the ITF version. Application shall have a lookup table to map the Fw API version to the Firmware version. (Eg: FWAPIVersion= 6.21.0 maps to FWVersion 7.0.0 etc)
 * This API is supported only for Max2 devices
 *
 *  @param[in] device_arg The device connection type. Eg: hid, 198.168.7.2@9887 etc. 
 *               The application can loop through the different supported device connections and see which device connection is used
 *  @param[in] startSlaveAddr The start of the slave address range from which slaves will be polled.
 *  @param[in] endSlaveAddr The end slave address range upto which slaves will be polled.
 *  @param[in/out] pTfaDetails Structure contianing the TFA details
 *
 *  @return Tfa98xx_Error_Ok when succeeded, error otherwise.
 */

Tfa98xx_Error_t Tfa9xxx_getAttachedTfaDetails(const char *device_arg, int startSlaveAddr, 
                                                int endSlaveAddr, tfaDetails_t  *ptfaDetails);

#ifdef __cplusplus
}
#endif
#endif				/* TFA98API_H */
