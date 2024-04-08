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
 
#include <stdint.h>
#include "TFA_I2C.h"

#ifndef LXSCRIBO_H_
#define LXSCRIBO_H_

#define LXSCRIBO_VERSION 	1.0

/** the default target device */
#define TFA_I2CDEVICE "hid"

/*
 * devkit pin assignments
 */

enum devkit_pinset {
	pinset_gpo_en_ana,
	pinset_gpo_en_i2s1,
	pinset_gpo_en_spdif,
	pinset_gpo_en_lpc,
	pinset_power_1v8, // 4
	pinset_led1,
	pinset_led2,
	pinset_led3,
	pinset_power_1v8_also, //8
	pinset_tfa_rst_l, // 9
	pinset_tfa_rst_r
};
enum devkit_pinget {
	pinget_gpi_det_i2s1,
	pinget_gpi_det_spdif,
	pinget_gpi_det_ana,
	pinget_gpi_det_i2s2,
	pinget_tfa_int_l, //4
	pinget_tfa_int_r
};

/* TFA_I2C interface plugin */
struct tfa_i2c_device {
	int (*init)(char *dev);
	int (*write_read)(int fd, int wsize, const unsigned char *wbuffer,
			int rsize, unsigned char *rbuffer, TFA_I2C_Error_t *pError);
	int (*version_str)(char *buffer, int fd);
	int (*set_pin)(int fd, int pin, int value);
	int (*get_pin)(int fd, int pin);
	int (*close)(int fd);
	int (*buffersize)(void);
	int (*startplayback)(char *buffer, int fd);
	int (*stopplayback)(int fd);
	TFA_I2C_IfType_t if_type;
	TFA_I2C_Error_t (*tfadsp_execute)(int slave, int command_length, void *command_buffer,
			                      int result_length, void *result_buffer);
	TFA_I2C_Error_t (*tfadsp_init)(int slave, int address, void *buffer, int length);
};

/* TFA_I2C interface plugin */
struct server_protocol {
	int (*init)(char *dev);
	int (*read)(int fd, unsigned char *wbuffer,int wsize);
	int (*write)(int fd, unsigned char *wbuffer,int wsize);
	int (*close)(int fd);
};
int  TFA_I2C_Interface(char *target, const struct tfa_i2c_device *dev);

/* lxScribo.c */
void  lxScriboVerbose(int level); // set verbose level.
int lxScriboRegister(char *dev);  // register target and return opened file desc.
int lxScriboUnRegister(void);
int lxScriboGetFd(void);          // return active file desc.
void lxScriboGetName(char *buf, int count);     // target name

int lxScriboNrHidDevices(void);
int lxScriboWaitOnPinEvent(int pin, int value, unsigned int timeout);

/* lxScriboSocket.c */
int lxScriboSocketInit(char *dev);
void lxScriboSocketExit(int status);

/*abstraction for server (TCP/UDP)*/
extern const struct server_protocol *server_dev;
extern int server_target_fd;						 /* file descriptor for target server */
int server_register(char *);
int server_interface(char *target, const struct server_protocol *interface_server);

#ifdef HAL_WIN_SIDE_CHANNEL
extern const struct tfa_i2c_device lxWinSideChannel_device;
#endif

extern const struct server_protocol tcp_server;
extern const struct server_protocol udp_server;

#if defined(WIN32) || defined(_X64) 
extern const struct tfa_i2c_device lxWindows_device;
#else  // posix/linux
extern const struct tfa_hal_plugin_funcs lxI2c_device;
#endif
extern const struct tfa_i2c_device lxScriboTCPIP_socket;
extern const struct tfa_i2c_device lxScriboUDP_socket;
int server_register(char *target);

extern int i2c_trace;

// for dummy
#define rprintf printf

//from gpio.h
#define I2CBUS 0

//from gui.h
/* IDs of menu items */
typedef enum
{
  ITEM_ID_VOLUME = 100,
  ITEM_ID_PRESET,
  ITEM_ID_EQUALIZER,

  ITEM_ID_STANDALONE,

  ITEM_ID_ENABLESCREENDUMP,
} menuElement_t;

#endif /* LXSCRIBO_H_ */
