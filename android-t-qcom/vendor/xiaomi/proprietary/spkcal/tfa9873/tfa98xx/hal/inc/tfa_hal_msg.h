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

#ifndef HAL_PLUGIN_NETWORK_SCRIBO_IDX_H_
#define HAL_PLUGIN_NETWORK_SCRIBO_IDX_H_

#include <stdint.h>
#include "tfa_hal.h"

/*
 * tha hal message command must start with upper case ascii
 * lower case may conflict with scribo
 */
enum tfa_hal_msg_cmd {
	TFA_HAL_MSG_VERSION='V',	/** < version command > */
	TFA_HAL_MSG_RESET='R',		/** < reset command > */
	TFA_HAL_MSG_I2C='I',		/** < I2C write  read command > */
	TFA_HAL_MSG_DSP='D',		/** < RPC write  read command > */
	TFA_HAL_MSG_PIN='P',		/** < PIN write  read command > */
	TFA_HAL_MSG_PLAYBACK='S',	/** < playback start stop command > */
};

#pragma pack (push, 1)
struct tfa_hal_msg {
	char cmd;			/** < command msb byte */
	char cmd_lsb;		/** < command lsb byte */
	int16_t seq; 		/** < sequence number  */
	int32_t index;		/** < remote address or  index */
	int32_t crc32;		/** < payload crc */
	int16_t error;		/** < error code returned */
	int16_t wlength;	/** < write length */
	int16_t rlength;	/** < read length */
	char data[];		/** < data payload */
};
#pragma pack (pop)

/* display the contents of the msg buffer */
void tfa_hal_msg_dump(struct tfa_hal_msg *msg, int rcv);

/* check crc32 of the returned message */
int tfa_hal_crc_check(struct tfa_hal_msg *msg, int msg_size);

/* add crc32 to the outgoing message */
void tfa_hal_crc_add(struct tfa_hal_msg *msg, int msg_size);

#endif /* HAL_PLUGIN_NETWORK_SCRIBO_IDX_H_ */
