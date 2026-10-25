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

#ifndef CLICOMMANDS_H_
#define CLICOMMANDS_H_

#include "tfaContUtil.h"
#include "cmdline.h"

int cliCommands(char *xarg, struct tfa_device **devs, int profile);
int cliTargetDevice(char *devname);

extern struct gengetopt_args_info gCmdLine; /* Globally visible command line args */

/*
 *  globals for output control
 */
extern int cli_verbose;	/* verbose flag */
extern int cli_trace;	/* message trace flag from bb_ctrl */

#define MAX_DEVICES 256
#define TRACEIN(F)  if(cli_trace) printf("Enter %s\n", F);
#define TRACEOUT(F) if(cli_trace) printf("Leave %s\n", F);

#endif /* CLICOMMANDS_H_ */
