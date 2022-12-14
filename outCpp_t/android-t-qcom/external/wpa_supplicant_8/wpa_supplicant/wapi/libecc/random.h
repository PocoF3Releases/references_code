/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        Random number.           
* Description:   Interface of generat random number.
* Writer:        zhangguoqiang@TEVG <zhanggq@iwncomm.com>
* Version:  $Id:random.h,v 1.0 2012/02/17 09:00:00 zhangguoqiang Exp $     
*
* History: 
* zhangguoqiang    2012/02/17    v1.0    Created.
*/

#ifndef _RANDOM_H
#define _RANDOM_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "mint.h"

int gen_random(unsigned char *val,unsigned int len,int type);
int gen_random_number(MINT *val,MINT *max,int type);
extern int gen_exrand(unsigned char *val,int len);

#ifdef  __cplusplus
}
#endif

#endif

