/************************************************************************/
/* Copyright 2014-2017 NXP Semiconductors                               */
/* Copyright 2020 GOODIX                                                */
/*                                                                      */
/* GOODIX Confidential. This software is owned or controlled by GOODIX  */
/* and may only be used strictly in accordance with the applicable      */
/* license terms.  By expressly accepting such terms or by downloading, */
/* installing, activating and/or otherwise using the software, you are  */
/* agreeing that you have read, and that you agree to comply with and   */
/* are bound by, such license terms.                                    */
/* If you do not agree to be bound by the applicable license terms,     */
/* then you may not retain, install, activate or otherwise use the      */
/* software.                                                            */
/*                                                                      */
/************************************************************************/

/************************************************************************
 *  Module:       tbase_al_impl_generic.h
 *  Description:
 *     abstraction layer implementation for common compilers                
 ************************************************************************/

#ifndef __tbase_al_impl_generic_h__
#define __tbase_al_impl_generic_h__


/* memcpy, memmove and memset are defined by the C99 standard and should be declared in string.h */
#include <string.h>

/* note: lint complains about these symbols already defined as functions */

/*lint -esym(652,TbCopyMemory) */
#define TbCopyMemory(dst,src,count) memcpy((dst),(src),(count))

/*lint -esym(652,TbMoveMemory) */
#define TbMoveMemory(dst,src,count) memmove((dst),(src),(count))

/*lint -esym(652,TbSetMemory) */
#define TbSetMemory(mem,val,count)  memset((mem),(val),(count))

/*lint -esym(652,TbZeroMemory) */
#define TbZeroMemory(mem,count)			memset((mem),0,(count))

/*lint -esym(652,TbIsEqualMemory) */
#define TbIsEqualMemory(buf1,buf2,count)	( 0 == memcmp((buf1),(buf2),(count)) )


#endif  /* __tbase_al_impl_generic_h__ */

/*************************** EOF **************************************/
