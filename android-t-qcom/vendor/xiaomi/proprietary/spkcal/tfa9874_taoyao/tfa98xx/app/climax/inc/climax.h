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

#ifndef CLIMAX_H_
#define CLIMAX_H_

#include "tfaContUtil.h"
#include "cmdline.h"

char *cliInit(int argc, char **argv);
#ifndef WIN32
void cliSocketServer(char *socket);
void cliClientServer(char *socket);
#endif

struct gengetopt_args_info gCmdLine; /* Globally visible command line args */

/*
 *  globals for output control
 */
extern int cli_verbose;	/* verbose flag */
extern int cli_trace;	/* message trace flag from bb_ctrl */

static const char *defined_pins[] = {
	"GPIO_LED_HEARTBEAT", // ID = 1
	"GPIO_LED_PLAY",
	"GPIO_LED_REC",
	"GPIO_LED_CMD",
	"GPIO_LED_AUDIO_ON",
	"GPIO_LED_USER0",
	"GPIO_LED_USER1",
	"GPIO_INT_L",
	"GPIO_INT_R",
	"GPIO_RST_L",
	"GPIO_RST_R",
	"GPIO_PWR_1V8",
	"GPIO_LED_RED",
	"GPIO_LED_BLUE",
	"GPIO_LED_UDA_ACTIVE",
	"GPIO_UP",
	"GPIO_DOWN",
	"GPIO_X65",
	"GPIO_LED_POWER_N",
	"GPIO_ISP_SELECT_N",
	"GPIO_BOOTSW4",
	"GPIO_BOOTSW3",
	"GPIO_PROFILE_SELECT",
	"GPIO_BOOTSW1",
	"GPIO_SD_ENABLE_POWER",
	"GPIO_USB_VBUS_ENABLE_N",
	"GPIO_LPC_VBUS_ENABLE_N",
	"GPIO_ENABLE_USB_FAST_CHARGE",
	"GPIO_ENABLE_I2S_N",
	"GPIO_LPC_ENABLE_RID",
	"GPIO_USB_VBUS_ACK_N",
	"GPIO_LPC_VBUS_ACK_N",
	"GPIO_USB_ENABLE_RID_GND",
	"GPIO_USB_ENABLE_RID_PD1",
	"GPIO_USB_ENABLE_RID_PD2",
	"GPIO_USB_ENABLE_RID_PD3",
	"GPIO_USB_ENABLE_RID_PD4",
	"GPIO_DETECT_ANA",
	"GPIO_DETECT_I2S2",
	"GPIO_DETECT_I2S1",
	"GPIO_DETECT_SPDIF",
	"GPIO_ENABLE_ANA",
	"GPIO_ENABLE_I2S1",
	"GPIO_ENABLE_LPC",
	"GPIO_ENABLE_SPDIF",
	"GPIO_PHY_PWDN",
	"GPIO_RSV2",
	"GPIO_RSV3",
	"GPIO_RSV4",
	"VIRT_CLOCK_STOP",
	"VIRT_NO_AUDIO_OUT_ENABLE",
	"VIRT_CLEAR_CALIBRATION",
	"VIRT_I2S_SLAVE_ENABLE",
	"VIRT_I2S_32FS_ENABLE",
	"VIRT_BOARD_RESET",
	0
};
#define MAX_DEVICES 256
#define TRACEIN(F)  if(cli_trace) printf("Enter %s\n", F);
#define TRACEOUT(F) if(cli_trace) printf("Leave %s\n", F);

#endif /* CLIMAX_H_ */
