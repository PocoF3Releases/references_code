/* 
 * Copyright ? 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * Module:        Base Types Defines.           
 * Description:   Defines the cryptographic algorithm with the necessary
 *                base types.
 * Writer:        zhangguoqiang@DST <zhanggq@iwncomm.com>
 * Version:  $Id:typedef.h,v 1.0 2011/04/01 09:00:00 zhangguoqiang Exp $     
 *
 * History: 
 * zhangguoqiang    2011/04/01    v1.0    Created.
 */

#ifndef _TYPEDEF_1_H
#define _TYPEDEF_1_H

#ifdef  __cplusplus
extern "C"
{
#endif

    typedef unsigned long long word64;
    typedef unsigned int word32;
    typedef unsigned short word16;
    typedef unsigned char word8;
    typedef word8 byte;
    typedef word32 dword;
    typedef word16 word;

    typedef int BOOL;

    typedef unsigned char u8;
    typedef unsigned short u16;
    typedef unsigned int u32;

    typedef unsigned char* p8;
    typedef const unsigned char* c8;
    //UNUSED(x) (void) (x);
#ifdef  __cplusplus
}
#endif

#endif


