/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        Bigint arithmetic methods.           
* Description:   Interface of Bigint arithmetic methods,includes 
*                add,aubtract,multiply,divide,modular,inverse.
* Writer:        zhangguoqiang@DST <zhanggq@iwncomm.com>;
*                wanhongtao@DST <wanht@iwncomm.com>
* Version:       $Id:mint.h,v 1.0 2011/12/09 09:00:00 zhangguoqiang Exp $;
*                $Id:mint.h,v 1.1 2012/11/21 09:00:00 wanhongtao Exp $ 
*
* History: 
* zhangguoqiang    2011/12/09    v1.0    Created.
* wanhongtao       2012/11/21    v1.1    Modify.
*/

#ifndef _MINT_H
#define _MINT_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "typedef.h"
/*--------------------------------------------------------------------*/
#define MAX_MINTLENGTH		19

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

#define MINT_BIT	32
#define MINT_BIT2	64
#define mint_is_odd(a)	    (((a)->len > 0) && ((a)->value[0] & 1))
#define window_size(a)      ((int) ((a) >= 2000 ? 6 : (a) >= 800 ? 5 : (a) >= 300 ? 4 : (a) >= 70 ? 3 : (a) >= 20 ? 2 : 1))
/*--------------------------------------------------------------------*/
typedef struct {
	unsigned int value[MAX_MINTLENGTH];
	int len;
}MINT;

/*--------------------------------------------------------------------*/
int octetstring_to_mint(const unsigned char *octstr_val, unsigned int octstr_len,MINT *mint_val);
int mint_to_octetstring(const MINT *mint_val,int buflen,unsigned char *octstr_val,unsigned int *octstr_len);

int mint_copy(MINT *dest,MINT *src);
int mint_compare(MINT *a,MINT *b);
int mint_left_move(MINT *a,unsigned int bits);
int mint_right_move(MINT *a,unsigned int bits);
int mint_adjust_len(MINT *mint_val,int len);
int mint_to_fixedlen_os(const MINT *src_int, unsigned int fixedlen,unsigned int osbufsize, unsigned int *oslen,unsigned char* dstr);

int mint_add(MINT *addend1,MINT *addend2,MINT *sum);
int mint_sub(MINT *minuend,MINT *subtrahend,MINT *diff);
int mint_mul(MINT *x,MINT *y,MINT *t);
int mint_div(MINT *dividend,MINT *divisor,MINT *quotient,MINT *remainder);
int mint_mod(MINT *a,MINT *p,MINT *r);
int mint_mod_reduce(MINT *dividend,MINT *divisor,MINT *modular);

int mint_mod_add(MINT *a,MINT *b,MINT *p,MINT *c);
int mint_mod_sub(MINT *a,MINT *b,MINT *p,MINT *c);
int mint_mod_mul(MINT *a,MINT *b,MINT *p,MINT *c);
int mint_EEA_algo(MINT *a,MINT *p,MINT *c);
int mint_bit_set(MINT *a, int n);

int get_mint_bit_len(MINT *x);
/*--------------------------------------------------------------------*/
#ifdef  __cplusplus
}
#endif

#endif


