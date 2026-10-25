/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        Bigint arithmetic expansion methods.           
* Description:   Interface of Bigint arithmetic expansion methods, 
*                includes exponent arithmetic and sqrt arthmetic.
* Writer:        zhangguoqiang@TEVG <zhanggq@iwncomm.com>
* Version:  $Id:mintex.h,v 1.0 2012/03/26 09:00:00 zhangguoqiang Exp $     
*
* History: 
* zhangguoqiang    2012/03/26    v1.0    Created.
*/

#ifndef _MINTEX_H
#define _MINTEX_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "random.h"

int mint_exp(MINT *g,MINT *a,MINT *p,MINT *x);
int mint_sqrt(MINT *g,MINT *p,MINT *x);

#ifdef  __cplusplus
}
#endif

#endif

