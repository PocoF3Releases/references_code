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

#ifndef SRV_INC_TFACONTAINERWRITE_H_
#define SRV_INC_TFACONTAINERWRITE_H_

#include "tfa_container.h"
#include "tfaContUtil.h"
#include "Tfa98API.h"

#define DEVICENAME_MAX 64
#define PROFILENAME_MAX 64

typedef struct tfaLocationInifile{
	char locationIni[FILENAME_MAX];
} tfaLocationInifile_t;

tfa98xxParamsType_t cliParseFiletype(char *filename);
/*
 * loads the container file
 */
enum Tfa98xx_Error tfa98xx_cnt_loadfile(char *fname, int cnt_verbose);
/*
 * Get customer header information
 */
void tfaContGetHdr(char *inputname, tfaHeader_t *hdrbuffer, tfaMsgHdrInfo_t *pHeaderInfo);


/*
 * Specify the device type for service layer support
 */
int tfa_cont_dev_type(int revid);

/*
 * Get customer header speaker file information
 */
void tfa_cnt_get_spk_hdr(char *inputname, struct tfaSpkHeader *hdrbuffer);
void tfa_cont_write_verbose(int verbose);
/* display */
void tfaContShowSpeaker(tfaSpeakerFile_t *spk);
void tfaContShowEq(tfaEqualizerFile_t *eqf);
void tfaContShowDrc(tfaDrc_t *file);
void tfaContShowMsg(tfaMsgFile_t *file);
void tfaContShowVstep2( tfaVolumeStep2File_t *vp);
char *tfaContFileTypeName(tfaFileDsc_t *file);

/**
 * read or write current bitfield value
 * @param bf_name the string of the bitfield name
 * @param bf_value the value that the bitfield should be set to (if write > 0)
 * @param slave_arg the slave address or device index to write bf, 
 *	slave_arg = -1 indicates write bf on all available slaves.
 * @param tfa_arg the device type, -1 if not specified
 * @param write indicates wether to write or read (write > 0 means write)
 * @return Tfa98xx_Error
 */
enum Tfa98xx_Error tfa_rw_bitfield(const char *bf_name, uint16_t bf_value, int slave_arg, int tfa_arg, int write);

/*
 * show the contents of the local container
 */
enum Tfa98xx_Error  tfaContShowContainer(char *strings, int maxlength);
int tfa_search_profile(tfaDescPtr_t *dsc, char *profile_name);
enum Tfa98xx_Error  tfaContShowItem(tfaDescPtr_t *dsc, char *strings, int maxlength, int devidx);
enum Tfa98xx_Error  tfaContShowDevice(int idx, char *strings, int maxlength);
enum Tfa98xx_Error  tfa98xx_header_check(char *fname, tfaHeader_t *buf);
enum Tfa98xx_Error  tfa98xx_show_headers(tfaHeader_t *buf, tfaHeaderType_t type, char *buffer);
int parse_vstep(tfaVolumeStepMax2_1File_t *vp, int header);

/**
 * Append a substring to a char buffer.
 * Make sure it doesn't write outside the buffer with maxlength
 * Keep track of an offset that indicates from what index to append
 * @param substring string to append to buffer
 * @param str_buff string buffer that gets appended
 * @param maxlength max length of the string buffer
 * @return Tfa98xx_Error
 */
enum Tfa98xx_Error tfa_append_substring(char *substring, char *str_buff, int maxlength);

int tfaContCrcCheck(tfaHeader_t *hdr) ;

/**
 * Split the (binary) container file into human readable ini and individual files
 * @return 0 if all files are generated
 */
int tfa98xx_cnt_split(char *fileName, char *directoryName);

/* write functions */
int tfaContSave(tfaHeader_t *hdr, char *filename);
int tfaContBin2Hdr(char *iniFile, int argc, char *argv[]);
int tfaContIni2Container(char *iniFile);
int tfaCont2Hdr(int numInputnames, char *inputname[], char *pOutputFile, int argc, char *argv[]);


/*
 * create the containerfile as descibred by the input 'ini' file
 * return the size of the new file
 */
int tfaContParseIni(char *iniFile, char *outFileName, tfaLocationInifile_t *loc);

int tfaContCreateContainer(tfaContainer_t *contIn, char *outFileName, tfaLocationInifile_t *loc);
tfaProfileList_t *tfaContFindProfile(tfaContainer_t *cont,const char *name);
tfaLiveDataList_t *tfaContFindLiveData(tfaContainer_t *cont,const char *name);
tfaProfileList_t *tfaContGet1stProfList(tfaContainer_t *cont);
tfaLiveDataList_t *tfaContGet1stLiveDataList(tfaContainer_t *cont);
tfaDescPtr_t *tfaContSetOffset(tfaDescPtr_t *dsc,int idx);
tfaDescriptorType_t parseKeyType(char *key);

/*
 * write a parameter file to the device
 * @param dev_idx to select the device index
 * @param fname the filename pointer to the file
 * @param VstepIndex to select the vstep index
 * @param VstepMsgIndex to select the vstep message index
 * @return Tfa98xx_Error error
 */
enum Tfa98xx_Error tfaContWriteFileByname(int dev_idx,  char *fname, int VstepIndex, int VstepMsgIndex);

/**
 * print all the bitfields of the register
 * @param dev_idx to select the device index
 * @param reg address
 * @param regvalue the register value
 * @return 0 if at least 1 name was found
 */
int tfaRunBitfieldDump(int dev_idx, unsigned char reg, uint16_t regvalue);

/**
 * set the dev revid in the module global tfa_dev_type[]
 */
int tfa_cont_set_patch_dev(char *configpath, char *patchfile, int devidx);

/*
 * Get the information of the profile
 */
enum Tfa98xx_Error getProfileInfo(char *profile_arg, int *profile);

void filter_coeff_anti_alias(tfaAntiAliasFilter_t *self, int tfa98xx_family_type);
void filter_coeff_equalizer(tfaEqFilter_t *self, int tfa98xx_family_type);
void filter_coeff_integrator(tfaIntegratorFilter_t *self, int tfa98xx_family_type);

const char *tfa_cont_eqtype_name(enum tfaFilterType t);

/**
 * Get number of matches in the header data
 *
 * @param hdr pointer to file header data
 * @param t typenumber for tfaheader
 * @return number of matches
 */
int HeaderMatches (tfaHeader_t *hdr, tfaHeaderType_t t);

/**
 * get information regarding vtsep file. 
 *
 * @param dev_idx to select the device index
 * @return tfa_vstep_msg_t
 */
tfa_vstep_msg_t *tfa_vstep_info(int dev_idx);
/**
 * get information regarding speaker file. 
 *
 * @param dev_idx to select the device index
 * @return length of speaker parameter
 */
int tfa_speaker_info(int dev_idx);

/**
* get information regarding speaker file.
*
* @param tfa_device to select the device 
* @return dev_idx to select the device index
*/
char *get_bus_info(struct tfa_device *tfa, int dev_idx);

#endif /* SRV_INC_TFACONTAINERWRITE_H_ */

