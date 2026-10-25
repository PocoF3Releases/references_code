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
 *  Module:       tbase_pack1.h 
 *  Description:
 *     save current packing setting, set packing to 1 byte                
 ************************************************************************/

/* NOTE: No include guards must be used here! */

#if !defined(TBASE_COMPILER_DETECTED)
	#error Compiler not detected. tbase_platform must be included before.
#endif

/* do not complain about missing include guard */
/*lint -efile(451,tbase_pack1.h) */


#if defined(TBASE_COMPILER_MICROSOFT)
/* MS Windows */
#include <pshpack1.h>

#elif defined(TBASE_COMPILER_GNU)
/* GNU compiler */
/* save current setting and set struct alignment = 1 byte */
#pragma pack(push,1)

#elif defined(TBASE_COMPILER_ARM)
/* ARM (Keil) compiler */
/* save current setting and set struct alignment = 1 byte */

#pragma push
#pragma pack(1) 


#ifndef _lint
/* S.W. ATTENTION!!! ARM COMPILER bug:
   if there is no declarations between a #pragma pack() and a #pragma pop, then the #pragma pop is ignored.
   There is currently no fix for this.
   Below is a workaround
*/  
extern int dummy_int_for_armcc_pragma_pop_bug;
#endif


#elif defined(TBASE_COMPILER_TMS470)
/* ARM Optimizing C/C++ Compiler (Texas Instruments/TMS470) */
/* TI compilers do not currently support the #pragma pack construct */
/* packing must be applied individually */

#elif defined(TBASE_COMPILER_GHS)
/* Green Hills C/C++ Compiler */
#pragma pack(1) 

#elif defined(TBASE_COMPILER_DIAB)   
/* DiabData / WindRiver C/C++ Compiler */
/* set alignment to 1 byte, current setting is NOT saved */
#pragma pack(1,1,0)

#elif defined(TBASE_COMPILER_PIC32)
/* Microchip PIC32 */
#pragma pack(push,1)

#else
#error Compiler not detected. This file needs to be extended.
#endif

/******************************** EOF ****************************************/
