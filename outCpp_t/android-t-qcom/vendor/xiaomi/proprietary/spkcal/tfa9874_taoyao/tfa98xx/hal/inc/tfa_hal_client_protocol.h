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

#ifndef TFA_HAL_CLIENT_PROTOCOL_H
#define TFA_HAL_CLIENT_PROTOCOL_H

#include "scribo_client_protocol.h"

int tfa_hal_client_startplayback(write_read_func_t write_read_func, struct tfa_hal_dev *dev, const char *buf);
int tfa_hal_client_stopplayback(write_read_func_t write_read_func, struct tfa_hal_dev *dev);
int tfa_hal_client_setpin(write_read_func_t write_read_func, struct tfa_hal_dev *dev, int pin, int value);
int tfa_hal_client_getpin(write_read_func_t write_read_func, struct tfa_hal_dev *dev, int pin, int *value);
int tfa_hal_client_i2c(write_read_func_t write_read_func, struct tfa_hal_dev *dev,
		unsigned address, const char *in_buf, size_t in_len, char *out_buf, size_t out_len);
int tfa_hal_client_dsp(write_read_func_t write_read_func, struct tfa_hal_dev *dev,
		unsigned address, const char *in_buf, size_t in_len, char *out_buf, size_t out_len);
int tfa_hal_client_reset(write_read_func_t write_read_func, struct tfa_hal_dev *dev, const char *buffer, int length);
#endif /* TFA_HAL_CLIENT_PROTOCOL_H */
