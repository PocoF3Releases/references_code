/* 
 * Copyright ? 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
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

#ifndef _HASH_256_H
#define _HASH_256_H

#ifdef  __cplusplus
extern "C"
{
#endif

#ifndef HASH_TYPE_SHA256_1
#define HASH_TYPE_SHA256_1	1
#endif

    typedef struct
    {
        int hash_type;
        const unsigned char *data;
        unsigned int len_high;
        unsigned int len_low;
    } HASH_CONTX_1;

    int
    hash_byte_1 ( HASH_CONTX_1 *ctx , unsigned char *digest );

#ifdef  __cplusplus
}
#endif

#endif

