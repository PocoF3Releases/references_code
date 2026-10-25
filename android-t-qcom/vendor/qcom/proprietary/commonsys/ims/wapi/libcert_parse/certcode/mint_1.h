/* 
 * Copyright ? 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * Module:        Bigint arithmetic methods.           
 * Description:   Interface of Bigint arithmetic methods,includes 
 *                add,aubtract,multiply,divide,modular,inverse.
 * Writer:        zhangguoqiang@DST <zhanggq@iwncomm.com>
 * Version:  $Id:mint.h,v 1.0 2011/12/09 09:00:00 zhangguoqiang Exp $     
 *
 * History: 
 * zhangguoqiang    2011/12/09    v1.0    Created.
 */

#ifndef _MINT_1_H
#define _MINT_1_H

#ifdef  __cplusplus
extern "C"
{
#endif

#include "typedef_1.h"
    /*--------------------------------------------------------------------*/
#define MAX_MINTLENGTH		18

#define MAX_DWORD_VALUE		(dword)0xffffffff
#define ONE_DWORD_VALUE		(dword)0x00000001
#define MAX_HALF_DWORD		0xffff
#define HALF_HIGH_DWORD		0xffff0000

#define HALF_DWORD_BITS		16
#define BITS_PER_BYTE		8
#define BITS_DWORD_SIZE		(sizeof(dword)*BITS_PER_BYTE)

#define LOW_HALF_DWORD(x)	((x) & MAX_HALF_DWORD)
#define HIGH_HALF_DWORD(x)	(((x) >> HALF_DWORD_BITS) & MAX_HALF_DWORD)
#define LOW_TO_HIGH(x)		(((dword)(x))<< HALF_DWORD_BITS)

#define MINT_MASK ((((dword)1)<<((dword)BITS_DWORD_SIZE))-((dword)1))
#define MIN(x,y) ((x)<(y)?(x):(y))

#define MI_BITS_PER_BYTE    8
#define MI_BYTES_PER_DWORD   (sizeof (dword))

#define MI_DWORDTOMINT(dest_mint,src_dword)	{if(src_dword==0) (dest_mint)->len = 0; else (dest_mint)->len = 1; (dest_mint)->value[0] = src_dword;}
#define MINT_ISZERO(ptr) (((((ptr)->len == 1) && ((ptr)->value[0] == 0)))|| ((ptr)->len==0))
    /*--------------------------------------------------------------------*/
    typedef struct
    {
        unsigned int value [ MAX_MINTLENGTH ];
        int len;
    } MINT;

    /*--------------------------------------------------------------------*/
    int
    octetstring_to_mint_c ( const unsigned char *octstr_val ,
            unsigned int octstr_len , MINT *mint_val );
    int
    mint_to_octetstring_c ( const MINT *mint_val , int buflen ,
            unsigned char *octstr_val , unsigned int *octstr_len );
    int
    mint_copy_c ( MINT *dest , MINT *src );
    int
    mint_adjust_len_c ( MINT *mint_val , int len );
    int
    mint_add_c ( MINT *addend1 , MINT *addend2 , MINT *sum );
    int
    get_mint_bit_len_c ( MINT *x );
/*--------------------------------------------------------------------*/
#ifdef  __cplusplus
}
#endif

#endif

