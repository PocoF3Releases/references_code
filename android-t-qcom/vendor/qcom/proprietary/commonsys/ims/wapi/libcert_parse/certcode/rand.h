/* 
 * Copyright ? 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * Module:        Random number.           
 * Description:   Interface of generat random number.
 * Writer:        zhangguoqiang@TEVG <zhanggq@iwncomm.com>
 * Version:  $Id:random.h,v 1.0 2012/02/17 09:00:00 zhangguoqiang Exp $     
 *
 * History: 
 * zhangguoqiang    2012/02/17    v1.0    Created.
 */

#ifndef _RAND_H
#define _RAND_H

#ifdef  __cplusplus
extern "C"
{
#endif

    int
    gen_random_1 ( unsigned char *val , unsigned int len );

#ifdef  __cplusplus
}
#endif

#endif

