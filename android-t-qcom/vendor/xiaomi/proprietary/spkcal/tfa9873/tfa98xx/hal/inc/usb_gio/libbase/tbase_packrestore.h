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
 *  Module:       tbase_packrestore.h 
 *  Description:
 *     restore previous packing setting                
 ************************************************************************/

/* NOTE: No include guards must be used here! */

#if !defined(TBASE_COMPILER_DETECTED)
	#error Compiler not detected. tbase_platform must be included before.
#endif

/* do not complain about missing include guard */
/*lint -efile(451,tbase_packrestore.h) */


#if defined(TBASE_COMPILER_MICROSOFT)
/* MS Windows */
#include <poppack.h>

#elif defined(TBASE_COMPILER_GNU)
/* GNU compiler */
/* restore previous alignment */
#pragma pack(pop)

#elif defined(TBASE_COMPILER_ARM)
/* ARM (Keil) compiler */
/* restore previous alignment */
#pragma pop 

#elif defined(TBASE_COMPILER_TMS470)
/* ARM Optimizing C/C++ Compiler (Texas Instruments/TMS470) */
/* TI compilers do not currently support the #pragma pack construct */
/* ### */

#elif defined(TBASE_COMPILER_GHS)
/* Green Hills C/C++ Compiler */
#pragma pack() 

#elif defined(TBASE_COMPILER_DIAB)
/* DiabData / WindRiver C/C++ Compiler */
/* set default alignment, last setting is NOT restored */
#pragma pack(0,0,0)

#elif defined(TBASE_COMPILER_PIC32)
/* Microchip PIC32 */
#pragma pack(pop)

#else
#error Compiler not detected. This file needs to be extended.
#endif

/******************************** EOF ****************************************/
