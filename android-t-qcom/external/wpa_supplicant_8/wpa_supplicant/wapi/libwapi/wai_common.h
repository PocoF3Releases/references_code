/*
 * Copyright (C) 2015-2020 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * Contains the common data structures and function declarations
 *
 * Authors:
 * iwncomm
 *
 * History:
 * yucun tian  09/10/2015 v1.0 maintain the code.
 * yucun tian  07/20/2016 v1.1 fix for change code style.
 *
 * Related documents:
 * -GB/T 15629.11-2003/XG1-2006
 * -ISO/IEC 8802.11:1999,MOD
 *
*/

#ifndef WAI_COMMON_H_
#define WAI_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/common.h"
#include "common/defs.h"

#include "alg_sm4_if.h"
#include "alg_ecc_if.h"

#if 0
/* ECC */
#define EC962_PRIVKEY_LEN	24
#define EC962_SIGN_LEN		48
#endif

/* Alloc buffer */
void *iwn_get_buffer(int len);
void *iwn_free_buffer(void *buffer, int len);

/* Network bytes order */
u16 iwn_ntohs(u16 v);
u16 iwn_htons(u16 v);

#define GETSHORT(frm, v) \
	do { (v) = (((frm[0]) <<8) | (frm[1])) & 0xffff; } \
	while (0)

#define GETSHORT1(frm, v) \
	do { (v) = (((frm[1]) <<8) | (frm[0])) & 0xffff; } \
	while (0)

#define SETSHORT(frm, v) \
	do{(frm[0]) = ((v)>>8) & 0xff; (frm[1])=((v)) &0xff; } \
	while (0)

/* Random alg */
void get_random(unsigned char *buffer, int len);

/*int hex2num(char c);*/
int str2byte(const unsigned char *str, int len,  char *byte_out);



#endif /* WAI_COMMON_H_ */

