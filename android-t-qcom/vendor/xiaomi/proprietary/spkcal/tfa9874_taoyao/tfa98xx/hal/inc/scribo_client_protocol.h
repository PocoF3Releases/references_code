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

#ifndef SCRIBO_CLIENT_PROTOCOL_H
#define SCRIBO_CLIENT_PROTOCOL_H

#include <string.h>

#ifndef TFA_HAL
  #define TFA_HAL
#endif
#include "tfa_hal.h"

typedef int (*write_read_func_t)(int fd, char *cmd, int cmdlen, char *response, int rcvbufsize, int *writeStatus);

int scribo_client_version(write_read_func_t write_read_func, struct tfa_hal_dev *dev, char *buf, size_t len);
int scribo_client_set_pin(write_read_func_t write_read_func, struct tfa_hal_dev *dev, int pin, int value);
int scribo_client_get_pin(write_read_func_t write_read_func, struct tfa_hal_dev *dev, int pin, int *value);
int scribo_client_start_playback(write_read_func_t write_read_func, struct tfa_hal_dev *dev, const char *buf, size_t len) ;
int scribo_client_stop_playback(write_read_func_t write_read_func, struct tfa_hal_dev *dev);
int scribo_client_init_write_message(write_read_func_t write_read_func, struct tfa_hal_dev *dev, int address, const char *buffer, int length);
int scribo_client_execute_message(write_read_func_t write_read_func, struct tfa_hal_dev *dev, unsigned address, const char *cmd_buf, size_t cmd_len, char *res_buf, size_t res_len);
int scribo_client_write_read(write_read_func_t write_read_func, struct tfa_hal_dev *dev, unsigned address, const char *in_buf, size_t in_len, char *out_buf, size_t out_len);
int scribo_client_check_version(struct tfa_hal_dev *dev);

#endif /* SCRIBO_CLIENT_PROTOCOL_H */
