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
 
#ifndef VERSION_H
#define VERSION_H

#include "ProductVersion.h"

#define VER_FILEVERSION             TFA98XX_HAL_REV_MAJOR,TFA98XX_HAL_REV_MINOR,TFA98XX_HAL_REV_REVISION
#define VER_FILEVERSION_STR         TFA98XX_HAL_REV_STR "\0"

#ifdef _X64
#define VER_FILEDESCRIPTION_STR     "TFA98xx HAL 64 bit I2C interface\0"
#else   //_X64
#define VER_FILEDESCRIPTION_STR     "TFA98xx HAL 32 bit I2C interface\0"
#endif  //_X64

#define VER_INTERNALNAME_STR        "Tfa98xx_hal.dll\0"
#define VER_ORIGINALFILENAME_STR    "Tfa98xx_hal.dll\0"


#endif //VERSION_H
