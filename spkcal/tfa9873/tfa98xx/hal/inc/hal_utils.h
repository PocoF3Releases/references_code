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

#ifndef HAL_UTILS_H_
#define HAL_UTILS_H_

int hal_add(int var1);

void hexdump(char *str, const unsigned char *data, int num_write_bytes);

void hexdump_to_file(FILE *filetype, char *str, const unsigned char *data, int num_write_bytes);

#endif /* HAL_UTILS_H_ */