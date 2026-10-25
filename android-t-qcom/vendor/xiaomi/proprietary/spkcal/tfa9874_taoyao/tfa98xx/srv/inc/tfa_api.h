#ifndef _TFA_API_H_
#define _TFA_API_H_

#include "tfa_error.h"
#include "tfa98xxCalculations.h"

/**
 * Diagnostic test description structure.
 */
enum tfa_diag_group {
	tfa_diag_all, 		/**< all tests, container needed, destructive */
	tfa_diag_i2c,	 	/**< I2C register writes, no container needed, destructive */
	tfa_diag_dsp, 	/**< dsp interaction, container needed, destructive */
	tfa_diag_sb, 		/**< SpeakerBoost interaction, container needed, not destructive */
	tfa_diag_func, 	/**< functional tests,  destructive */
	tfa_diag_pins  	/**< Pin tests, no container needed, destructive */
};

typedef enum Tfa98xx_Error Tfa98xx_Error_t;

/**
* If needed, this API function can be used to get a text version of the error code
* @param error: An error code of type TFA98xx_Error_t
* @return Pointer to a text version of the error code
*/
const char *Tfa98xx_GetErrorString(Tfa98xx_Error_t error);

/*
* loads the container file
*/
enum Tfa98xx_Error tfa98xx_cnt_loadfile(char *fname, int cnt_verbose);

/*
 * Funtion to split the container file into individual files and ini file
 */
int tfa98xx_cnt_split(char *fileName, char *directoryName);

/* Type containing all the possible errors that can occur
*
*/

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

/**
 * Resets tfa98xx device (this is proper reset by using I2C register bit)
 *
 * @param dev_idx device index to indicate tfa98xx instance
 *
 * @return Tfa98xx_Error_Ok when successful, error otherwise.
 */
Tfa98xx_Error_t Tfa98xx_Reset(int dev_idx);

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
* This API function allows writing an arbitrary I2C register of the TFA98XX.
* @param dev_idx to select the device index
* @param subaddress: 8 bit subaddress of the I2C register
* @param value: 16 bit value to be stored in the selected I2C register
*/
Tfa98xx_Error_t Tfa98xx_WriteRegister16(int dev_idx,
	unsigned char subaddress,
	unsigned short value);

/**
* This API function allows reading an arbitrary I2C register of the TFA98xx.
* @param dev_idx to select the device index
* @param subaddress: 8 bit subaddress of the I2C register
* @param pValue the value read from the I2C register
* @return Tfa98xx_Error_t 16 bit value that was stored in the selected I2C register
*/
Tfa98xx_Error_t Tfa98xx_ReadRegister16(int dev_idx, unsigned char subaddress, unsigned short *pValue);

/*
* run the calibration sequence
*
* @param device index
* @param once=1 or always=0
* @return Tfa98xx Errorcode
*/
Tfa98xx_Error_t tfa98xxCalibration(int dev_idx, int once, int profile);

/* Get the calibration result */
enum Tfa98xx_Error tfa98xx_dsp_get_calibration_impedance(int dev_idx, float *p_re25);

enum Tfa98xx_Error Tfa98xx_stopplayback(int dev_idx);

enum Tfa98xx_Error Tfa98xx_startplayback(int dev_idx, char *data);

Tfa98xx_Error_t Tfa98xx_set_pin(int dev_idx, int pin, int value);

Tfa98xx_Error_t Tfa98xx_get_pin(int dev_idx, int pin, int *value);

int tfa98xxPrintSpeakerModel(int dev_idx, float *strings, int maxlength, int xmodel);

#define int24_t int

int tfa98xxCalculateMBDrcModel(int dev_idx, int index_subband, s_subBandDynamics *sbDynamics, float *SbFilt_buf, float *CombGain_buf, int maxlength, int *subBandFilterTypes);

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
/**
* @brief select file that HAL layer traces into (could be stdout).
*
* @remark this operation is device independent. It could be called always.
*
* @param[in]  io handle to output stream
*
* @return 0 when successful, error code otherwise @ref hal_error_codes.
*
*/
int tfa_hal_set_tracefile(void *io);

/**
* @brief start playback on target, use file given in buffer 'buf'.
*
* @param[in]  dev       handle to plugin
* @param[in]  buf  file name of audio file to playback
* @param[in]  len  length of string stored in buf, expressed in bytes
*
* @return 0 when successful, error code otherwise @ref hal_error_codes.
*
*/
int tfa_hal_set_trace(unsigned mask);

int get_device_count(void);
int  tfaReadFile(char *fname, void **buffer);
Tfa98xx_Error_t Tfa98xx_hal_version(int dev_idx, void *buffer, int buffer_length);
Tfa98xx_Error_t Tfa98xx_ContWriteFile(int dev_idx);
Tfa98xx_Error_t writemsgblob(int profile);
Tfa98xx_Error_t get_slave_address(int dev_idx, unsigned char *slave_address);
Tfa98xx_Error_t get_ext_dsp_status(int dev_idx, int *status);
Tfa98xx_Error_t set_ext_dsp_status(int dev_idx, int *status);
Tfa98xx_Error_t Tfa98xx_get_max_vstep(int dev_idx, int prof_idx);
Tfa98xx_Error_t Tfa98xx_get_prof_count(int dev_idx, int *prof_count);
int Tfa98xx_cnt_get_app_name(void *cnt, char *name);

/* create .cnt file from .ini file */
int tfaContIni2Container(char *iniFile);

/*calls related to calibrations*/
Tfa98xx_Error_t tfa98xxCalResetMTPEX(int dev_idx);

/* set calibration impedance for selected idx */
Tfa98xx_Error_t tfa98xxSetCalibrationImpedance(int dev_idx, float re0);

/**/
enum Tfa98xx_Error tfa98xx_supported_speakers(int dev_idx, int *spkr_count);

/*diag related APIs*/
/**
* run a testnumber
*
*  All the diagnostic test functions will return 0 if passed
*  on failure the return value may contain extra info.
* @param dev_idx to select the device index
* @param testnumber diagnostic test number
* @return if testnumber is too big an empty string is returned
*/
int tfa98xxDiag(int dev_idx, int testnumber);

/**
* translate group name argument to enum
* @param arg groupname
* @return enum tfa_diag_group
*/
enum tfa_diag_group tfa98xxDiagGroupname(char *arg);

/**
* run all tests in the group
*
*  All the diagnostic test functions will return 0 if passed
*  on failure the return value may contain extra info.
* @param dev_idx to select the device index
* @param group
* @return 0
* @return > 0 test  failure
* @return < 0 config data missing
*/
int tfa98xxDiagGroup(int dev_idx, enum tfa_diag_group group);

/**
*  print supported device features
* @param devidx to select the device index
* @return 0
* @return > 0 test  failure
* @return < 0 config data missing
*/
int tfa98xxDiagPrintFeatures(int devidx);
/**
* dump all known registers
*   returns:
*     0 if slave can't be opened
*     nr of registers displayed
* @param dev_idx to select the device index
*/
int tfa98xxDiagRegisterDump(int dev_idx);


/**
* run all tests
* @param dev_idx to select the device index
* @return last error code
*/
int tfa98xxDiagAll(int dev_idx);

/**
* return the number of the test that was executed last
* @return latest testnumber
*/
int tfa98xxDiagGetLatest(void);

/**
* return the errorstring of the test that was executed last
* @return last error string
*/
char *tfa98xxDiagGetLastErrorString(void);

/**
* Get the connected audio source
*/
int tfa98xxGetAudioSource(int dev_idx);

Tfa98xx_Error_t set_dev_verbose(int dev_idx, int verbose);

enum Tfa98xx_Error device_system_init(unsigned char *slave_address, int count, const char *device_arg);

int is_ext_dsp(int dev_idx);
enum Tfa98xx_Error tfa_send_calibration_values();
enum Tfa98xx_Error tfa_start(int next_profile, int vstep);
enum Tfa98xx_Error tfa_stop();
enum Tfa98xx_Error tfaVersion(char *buffer);
enum Tfa98xx_Error tfaVersions(int dev_idx, char *strings, int maxlength);
enum Tfa98xx_Error tfaGetDspFWAPIVersion(int dev_idx);
/*
* save dedicated device files. Depends on the file extension
*/
int tfa98xxSaveFileWrapper(int dev_idx, char *filename);
/**
* Specify the speaker configurations (cmd id) (Left, right, both, none)
* @param dev_idx index of the device
* @param configuration name string of the configuration
*/
/* Timer functions (currently only used by livedata) */
int start_timer(int, void(*)(void));
void stop_timer(void);
void extern_msleep_interruptible(int msec);

#endif
