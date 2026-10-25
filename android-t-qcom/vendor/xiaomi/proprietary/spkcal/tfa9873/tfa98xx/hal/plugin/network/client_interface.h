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

#ifndef HAL_INC_CLIENT_INTERFACE_H_
#define HAL_INC_CLIENT_INTERFACE_H_
#include "tfa_device.h"
/*lx scribo functions for tcp */
int lx_scribo_tcp_init(char *server);
int lx_scribo_tcp_write_read(int fd, int wsize, const uint8_t *wbuffer, int rsize,uint8_t *rbuffer, TFA_I2C_Error_t *pError);
int lx_scribo_tcp_version(char *buffer, int fd);
int lx_scribo_tcp_set_pin(int fd, int pin, int value);
int lx_scribo_tcp_get_pin(int fd, int pin);
int lx_scribo_tcp_close(int fd);
int lx_scribo_tcp_start_playback(char *buffer, int fd);
int lx_scribo_tcp_stop_playback(int fd);
TFA_I2C_Error_t lx_scribo_tcp_execute_message(int slave, int command_length, void *command_buffer,int read_length, void *read_buffer);
TFA_I2C_Error_t lx_scribo_tcp_init_message(int slave,void *buffer, int length);

/*scribo function*/
int lx_scribo_udp_init(char *server);
int lx_scribo_udp_write_read(int fd, int wsize, const uint8_t *wbuffer, int rsize,uint8_t *rbuffer, TFA_I2C_Error_t *pError);
int lx_scribo_udp_version(char *buffer, int fd);
int lx_scribo_udp_set_pin(int fd, int pin, int value);
int lx_scribo_udp_get_pin(int fd, int pin);
int lx_scribo_udp_close(int fd);
int lx_scribo_udp_start_playback(char *buffer, int fd);
int lx_scribo_udp_stop_playback(int fd);
TFA_I2C_Error_t lx_scribo_udp_execute_message(int slave, int command_length, void *command_buffer,
		int read_length, void *read_buffer);
TFA_I2C_Error_t lx_scribo_udp_init_message(int slave, void *buffer, int length);

struct udp_socket {
	int in_use;
	int fd;
	struct sockaddr_in remaddr;	/* remote address */
};

#endif /* HAL_INC_CLIENT_INTERFACE_H_ */
