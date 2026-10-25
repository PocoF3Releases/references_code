/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        Elliptic Curve Cryptography Algorithm F(p)           
* Description:   Interface of elliptic curve cryptography algorithm,
*				 The algorithm mainly includes Point doubling,Point
*                addition and Point multiplication.
* Writer:        zhangguoqiang@DST <zhanggq@iwncomm.com>;
*                wanhongtao@DST <wanht@iwncomm.com>
* Version:       $Id:ecc.h,v 1.0 2011/12/15 08:24:24 zhangguoqiang Exp $;
*                $Id:ecc.h,v 1.1 2012/11/21 09:00:00 wanhongtao Exp $ 
*
* History: 
* zhangguoqiang    2011/12/15    v1.0    Created.
* wanhongtao       2012/11/21    v1.1    Modify.
*/

#ifndef _ECC_H
#define _ECC_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "mint.h"

#define ECC_GDPT_LEN_1		17				
#define ECC_GPT_LEN_1		512
/*--------------------------------------------------------------------*/
typedef struct {
	int is_infinite;
	MINT x;          
	MINT y;          
}ECC_FpPOINT;

typedef struct {
	MINT x;          
	MINT y;
	MINT z;
}ECC_JacPOINT;

typedef struct {
	MINT P;
	MINT A;
	MINT B;
	ECC_FpPOINT G;
	MINT N;
	int bitslen;
}ECC_FpCURVE;

typedef struct EccCurveHandle_st
{
	ECC_FpCURVE g_fp_curve;
	ECC_FpPOINT g_GPt_1[520];
	MINT g_h;		
}EccCurveHandle;

/*----------------------------------------------------------------------------------------*/
int ecc_precomputation_point(ECC_FpPOINT *p,MINT *a,MINT *b,MINT *prime,ECC_FpPOINT g_GPt_1[ECC_GPT_LEN_1]);

int ecc_point_add(ECC_FpPOINT *x,ECC_FpPOINT *y,MINT *a,MINT *b,MINT *prime,ECC_FpPOINT *z);
int ecc_point_double(ECC_FpPOINT *pt,MINT *a,MINT *b,MINT *prime,ECC_FpPOINT *result);
int ecc_fix_point_mul(MINT *k,ECC_FpPOINT *x,MINT *a,MINT *b,MINT *prime,ECC_FpPOINT *y,ECC_FpPOINT g_GPt_1[ECC_GPT_LEN_1]);
int ecc_unknown_point_mul(MINT *k,ECC_FpPOINT *x,MINT *a,MINT *b,MINT *prime,ECC_FpPOINT *y);
int ecc_test_point(ECC_FpPOINT *x,MINT *a,MINT *b,MINT *prime);
int ecc_point_unpress(MINT *y,MINT *x,MINT *a,MINT *b,MINT *prime,unsigned int yp);
/*-----------------------------------------------------------------------------------------*/
int ecc_affine_jacobin(ECC_JacPOINT *dest,ECC_FpPOINT *src);
int ecc_jacobin_affine(ECC_FpPOINT *dest,MINT *prime,ECC_JacPOINT *src);
int ecc_jac_point_add(ECC_JacPOINT *addend1,ECC_FpPOINT *addend2,MINT *a,MINT *b,MINT *prime,ECC_JacPOINT *result);
int ecc_jac_point_add_1(ECC_JacPOINT *addend1,ECC_JacPOINT *addend2,MINT *a,MINT *b,MINT *prime,ECC_JacPOINT *result);
int ecc_jac_point_double(ECC_JacPOINT *operand,MINT *a,MINT *b,MINT *prime,ECC_JacPOINT *result);

int mint_mod_left_move_1(MINT *c, MINT *a, MINT *p);
int mint_mod_left_move_n(MINT *c, MINT *a, MINT *p, int n);

#ifdef  __cplusplus
}
#endif

#endif
