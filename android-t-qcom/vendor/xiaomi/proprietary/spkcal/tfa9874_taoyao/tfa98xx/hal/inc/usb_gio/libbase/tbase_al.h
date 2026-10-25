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
 *  Module:       tbase_al.h
 *  Description:
 *     abstraction layer for basic compiler intrinsics and runtime functions
 ************************************************************************/

#ifndef __tbase_al_h__
#define __tbase_al_h__

#if defined (__cplusplus)
extern "C" {
#endif


/* AL functions are optionally implemented inline */
#ifdef NO_TBASE_AL_INLINE
#define TBASE_AL_INLINE
#else
#define TBASE_AL_INLINE	static TB_INLINE
#endif



/* copy memory from src to dst */
/* IMPORTANT: src and dst range MUST NOT overlap! */
void TBASE_AL_CALL TbCopyMemory(void* dst, const void* src, size_t count);

/* copy memory from src to dst */
/* src and dst range are allowed to overlap. */
void TBASE_AL_CALL TbMoveMemory(void* dst, const void* src, size_t count);

/* fill memory with the specified val */
void TBASE_AL_CALL TbSetMemory(void* mem, T_UINT8 val, size_t count);

/* fill memory with zero */
void TBASE_AL_CALL TbZeroMemory(void* mem, size_t count);

/* compare two memory buffers */
/* returns 1 if bytes are identical, returns 0 if bytes are not identical */
int TBASE_AL_CALL TbIsEqualMemory(const void* buf1, const void* buf2, size_t count);

/* IMPORTANT: 
   The following atomic operations require that the variable that is 
   changed is aligned to its natural boundary (4 bytes)! 
		
	 The atomic functions have to work in any context, including ISR.
	 For the implementation use atomic load/store instructions, or if those are not available,
	 use global DisableInterrupt and EnableInterrupt intrinsics.
	 
	 These functions are atomic with respect to calls to other TbAtomicXxx functions.
*/

/* Increments the variable pointed to by val as an atomic operation and returns the result (incremented value). */
TBASE_AL_INLINE
T_INT32 TBASE_AL_CALL TbAtomicIncrementInt32(volatile T_INT32* target);

/* Decrements the variable pointed to by val as an atomic operation and returns the result (decremented value). */
TBASE_AL_INLINE
T_INT32 TBASE_AL_CALL TbAtomicDecrementInt32(volatile T_INT32* target);

/* The TbAtomicExchangeUInt32 routine sets the unsigned integer variable pointed to by target to the given value 
   as an atomic operation and returns the old value (the value when the function is called) of the target variable. */
TBASE_AL_INLINE
T_UINT32 TBASE_AL_CALL TbAtomicExchangeUInt32(volatile T_UINT32* target, T_UINT32 val);

/* The TbAtomicOrUInt32 routine performs, as an atomic operation, a bitwise OR operation of the variable pointed 
   to by target and the given value, stores the result in the target variable 
   and returns the old value (the value when the function is called) of the target variable. */
TBASE_AL_INLINE
T_UINT32 TBASE_AL_CALL TbAtomicOrUInt32(volatile T_UINT32* target, T_UINT32 val);

#if defined (__cplusplus)
}
#endif


/* implementation file that optionally implements some of the functions as macros */
/* needs to be provided by the concrete platform */
#include "tbase_al_impl.h"


#endif  /* __tbase_al_h__ */

/*************************** EOF **************************************/
