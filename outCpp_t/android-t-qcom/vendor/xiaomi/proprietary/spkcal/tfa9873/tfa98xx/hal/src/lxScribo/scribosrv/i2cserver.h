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

#ifndef __I2CSERVER_H__
#define __I2CSERVER_H__

int i2c_GetSpeed(int bus);
void i2c_SetSpeed(int bus, int bitrate);

int i2c_WriteRead(int addrWr, void* dataWr, int sizeWr, void* dataRd, int sizeRd, int* nRd);
int i2c_Write(int bus, int addrWr, void* dataWr, int sizeWr);

#endif /* __I2CSERVER_H__ */
