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

#ifndef TFACONTUTIL_H_
#define TFACONTUTIL_H_

#ifdef __ANDROID__
//#include <utils/Log.h>
#else
#define LOGV if (0/*tfa98xx_verbose*/) printf //TODO improve logging
#endif

#include "tfa_dsp_fw.h"
#include "tfa98xx_parameters.h"

#if defined(WIN32) || defined(_X64)
#include <windows.h>
#else
#include <sys/time.h>
#include <signal.h>
#endif

#ifndef timer_INCLUDED
#define timer_INCLUDED
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#define TFA_MAXLINE		(256)       /* maximum string length */
#define TFA_MAXBUFFER	(50*1024)   /* maximum buffer size to hold the container */

/*
 * buffer types for setting parameters
 */
typedef enum tfa98xxParamsType {
    tfa_patch_params,
    tfa_speaker_params,
    tfa_preset_params,
    tfa_config_params,
    tfa_equalizer_params,
    tfa_drc_params,
    tfa_vstep_params,
    tfa_cnt_params,
    tfa_msg_params,
    tfa_no_params,
    tfa_info_params,
    tfa_algo_params,
    tfa_excursion_filter_params,
    tfa_lsmodel_params
} tfa98xxParamsType_t;

enum Tfa98xx_Error tfaVersions(int dev_idx, char *strings, int maxlength);
/*
* search for the first rpc cmd_id and return the length
*  or 0 if not found
*/
int Tfa98xx_ContainerGetMessageLength(tfaContainer_t *cnt, int dev_idx, uint32_t cmd_id);
int tfa98xxSaveFile(int dev_idx, char *filename, tfa98xxParamsType_t params);
int tfaGetRegMapVersion(int dev_idx, char *buffer);
int compare_strings(char *buffer, char *name, int buffersize);
enum Tfa98xx_Error tfaVersion(char *buffer);

#define TFA_MAX_LOOKUP  30

typedef struct tfaLookUp
{
  unsigned short rev_id;
  unsigned char TfaName[20];
}tfaLookUp_t;


/*
 * save dedicated device files. Depends on the file extension
 */
int tfa98xxSaveFileWrapper(int dev_idx, char *filename);

enum Tfa98xx_Error tfa98xx_verify_speaker_range(int idx, float imp[2], int spkr_count);

/* hex dump of cnt */
void tfa_cnt_hexdump(void);
void tfa_cnt_dump(void);

/* Timer functions (currently only used by livedata) */
int start_timer(int, void (*)(void));
void stop_timer(void);

void extern_msleep_interruptible(int msec);

enum Tfa98xx_Error tfa_open(int maxdev);
void tfa_close(int maxdev);

int is_ext_dsp(int dev_idx);
enum Tfa98xx_Error tfa_send_calibration_values();
enum Tfa98xx_Error tfa_start(int next_profile, int vstep);
enum Tfa98xx_Error tfa_stop();

enum Tfa98xx_Error tfa_system_open(unsigned char *slave_address, int count);
enum Tfa98xx_Error tfa_getAttachedTfaDetails(const char *device_arg, int startSlaveAddr, 
                                                int endSlaveAddr, void  *pTfaDetails);

#if defined(__cplusplus)
}  /* extern "C" */
#endif
#endif /* TFACONTUTIL_H_ */
