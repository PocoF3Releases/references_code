/************************************************************************/
/* Copyright 2014-2018 NXP Semiconductors                               */
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

#ifndef TFA_ERROR_H_
#define TFA_ERROR_H_

/**
 * tfa error return codes
 */
enum tfa_error {
        tfa_error_ok,       /**< no error */
        tfa_error_device,   /**< no response from device */
        tfa_error_bad_param,/**< parameter no accepted */
        tfa_error_noclock,  /**< required clock not present */
        tfa_error_timeout,  /**< a timeout occurred */
        tfa_error_dsp,      /**< a DSP error was returned */
        tfa_error_container,/**< no or wrong container file */
        tfa_error_max       /**< impossible value, max enum */
};

enum Tfa98xx_Error {
	Tfa98xx_Error_Ok = 0,
	Tfa98xx_Error_Device,			/* 1. Currently only used to keep in sync with tfa_error */
	Tfa98xx_Error_Bad_Parameter,	/* 2. */
	Tfa98xx_Error_Fail,             /* 3. generic failure, avoid mislead message */
	Tfa98xx_Error_NoClock,          /* 4. no clock detected */
	Tfa98xx_Error_StateTimedOut,	/* 5. */
	Tfa98xx_Error_DSP_not_running,	/* 6. communication with the DSP failed */
	Tfa98xx_Error_AmpOn,            /* 7. amp is still running */
	Tfa98xx_Error_NotOpen,	        /* 8. the given handle is not open */
	Tfa98xx_Error_InUse,	        /* 9. too many handles */
	Tfa98xx_Error_Buffer_too_small, /* 10. if a buffer is too small */
	/* the expected response did not occur within the expected time */
	Tfa98xx_Error_RpcBase = 100,
	Tfa98xx_Error_RpcBusy = 101,
	Tfa98xx_Error_RpcModId = 102,
	Tfa98xx_Error_RpcParamId = 103,
	Tfa98xx_Error_RpcInvalidCC = 104,
	Tfa98xx_Error_RpcInvalidSeq = 105,
	Tfa98xx_Error_RpcInvalidParam = 106,
	Tfa98xx_Error_RpcBufferOverflow = 107,
	Tfa98xx_Error_RpcCalibBusy = 108,
	Tfa98xx_Error_RpcCalibFailed = 109,
	Tfa98xx_Error_Not_Implemented,
	Tfa98xx_Error_Not_Supported,
	Tfa98xx_Error_I2C_Fatal,	/* Fatal I2C error occurred */
	/* Nonfatal I2C error, and retry count reached */
	Tfa98xx_Error_I2C_NonFatal,
	Tfa98xx_Error_Other = 1000
};

#endif
