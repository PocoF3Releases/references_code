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

#ifndef HAL_INC_SERVER_INTERFACE_H_
#define HAL_INC_SERVER_INTERFACE_H_

int udp_init_socket(char *dev);
int udp_read_data(int fd, unsigned char *wbuffer, int wsize);
int udp_write_data(int fd, unsigned char *wbuffer, int wsize);
int udp_close_socket(int fd);

int tcp_init_socket(char *dev);
int tcp_read_data(int fd, unsigned char *wbuffer, int wsize);/*int rsize, unsigned char *rbuffer, unsigned int *pError);*/
int tcp_write_data(int fd, unsigned char *wbuffer, int wsize);
int tcp_close_socket(int fd);

int udp_socket_init(char *server_add, int port);
/*
* Unix variant of udp_read()
*/
int udp_read(int fd, char *inbuf, int len, int waitsec);

/*
* Unix variant of udp_write()
*/
int udp_write(int fd, char *outbuf, int len);
int udp_close(int fd, int option);
/*
* test if data available
*/
int udp_is_readable(int sd, int * error, int timeOut); // milliseconds
#endif
