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

#ifndef TFA98XXCALIBRATION_H_
#define TFA98XXCALIBRATION_H_

#include "Tfa98API.h" /* legacy API */

/*
 * run the calibration sequence
 *
 * @param device index
 * @param once=1 or always=0
 * @return Tfa98xx Errorcode
 */
Tfa98xx_Error_t tfa98xxCalibration(int dev_idx, int once, int profile);

/*
* run the haptic calibration calibration procedure
*
* @param device index
* @param once=1 or always=0
* @return Tfa98xx Errorcode
*/
Tfa98xx_Error_t tfa98xxHapticCalibration(int dev_idx, int once, int profile);
Tfa98xx_Error_t tfa98xxHapticCalibrationF0R0Check(int dev_idx);

/**
 * reset mtpEx bit in MTP
 */
Tfa98xx_Error_t tfa98xxCalResetMTPEX(int dev_idx);
Tfa98xx_Error_t tfa98xxCalSetCalibrateOnce(int dev_idx);
Tfa98xx_Error_t tfa98xxCalSetCalibrationAlways(int dev_idx);

/* set calibration impedance for selected idx */
Tfa98xx_Error_t tfa98xxSetCalibrationImpedance(int dev_idx, float re0);

/* Get the calibration result */
enum Tfa98xx_Error tfa98xx_dsp_get_calibration_impedance(int dev_idx, float *p_re25);
enum Tfa98xx_Error tfa98xx_haptic_dsp_get_calibration_impedance(int dev_idx, float *f0);

#endif /* TFA98XXCALIBRATION_H_ */
