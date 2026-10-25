/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        Hash value calculation.           
* Description:   Interface of Hash algorithm,use hash_type distinguish
*				 concrete algorithm.
* Writer:        zhangguoqiang@DST <zhanggq@iwncomm.com>
* Version:  $Id:hash.h,v 1.0 2011/04/01 09:00:00 zhangguoqiang Exp $     
*
* History: 
* zhangguoqiang    2011/04/01    v1.0    Created.
*/

#ifndef _HASH_H
#define _HASH_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "mint.h"

#ifndef HASH_TYPE_SM3
#define HASH_TYPE_SM3		0
#endif
#ifndef HASH_TYPE_SHA256
#define HASH_TYPE_SHA256	1
#endif

typedef struct {
	int hash_type;
	const unsigned char *data;
	unsigned int len_high;
	unsigned int len_low;
}HASH_CONTX;

int hash_mint_value(HASH_CONTX *ctx,MINT *max,MINT *value);
int hash_byte(HASH_CONTX *ctx,unsigned char *digest);

#ifdef  __cplusplus
}
#endif

#endif

