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

#ifndef CMD_H_
#define CMD_H_

void Cmd_Init(void);
void CmdProcess(void* buf, int len);
void set_socket_verbose(int verbose);
void set_socket(int socket);
const struct server_protocol *get_server(void);
int cmd_startplayback(char *buf);
int cmd_stopplayback(void);

#endif /* CMD_H_ */
