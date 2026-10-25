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
 *  Module:       tbase_types.h
 *  Description:
 *     define basic integer types of known size
 ************************************************************************/

#ifndef __tbase_types_h__
#define __tbase_types_h__

#if !defined(TBASE_COMPILER_DETECTED)
	#error Compiler not detected. tbase_platform must be included before.
#endif


#ifdef TBASE_COMPILER_C99
/*
	C99 specifies a set of fixed-size integer types which we use here.
	They should be defined in stdint.h which comes with the compiler.
*/

#include <stdint.h>

typedef int8_t			T_INT8;
typedef uint8_t			T_UINT8;

typedef int16_t			T_INT16;
typedef uint16_t		T_UINT16;

typedef int32_t			T_INT32;
typedef uint32_t		T_UINT32;

typedef int64_t			T_INT64;
typedef uint64_t		T_UINT64;

#else
/*
	Compiler does not support the C99 types uint8_t etc.
	e.g. Microsoft compiler
*/
typedef signed char          T_INT8;
typedef unsigned char        T_UINT8;

typedef signed short         T_INT16;
typedef unsigned short       T_UINT16;

typedef signed int           T_INT32;
typedef unsigned int         T_UINT32;

typedef signed long long     T_INT64;
typedef unsigned long long   T_UINT64;

#endif


/* boolean type */
typedef T_UINT8 T_BOOL;
#define T_TRUE 1
#define T_FALSE 0

/* conversion from/to C++ bool */
#define bool_from_T_BOOL(x) ((x) != T_FALSE)
#define T_BOOL_from_bool(x) ((x) ? T_TRUE : T_FALSE)


/*
 * The following typedef aliases are used for better documentation in struct
 * definitions to indicate a fixed byte order, e.g. USB structs are fixed little endian and
 * ethernet structs are fixed big endian.
 */
typedef T_UINT16 T_BE_UINT16;
typedef T_UINT16 T_LE_UINT16;

typedef T_UINT32 T_BE_UINT32;
typedef T_UINT32 T_LE_UINT32;

typedef T_UINT64 T_BE_UINT64;
typedef T_UINT64 T_LE_UINT64;

typedef T_INT16 T_BE_INT16;
typedef T_INT16 T_LE_INT16;

typedef T_INT32 T_BE_INT32;
typedef T_INT32 T_LE_INT32;

typedef T_INT64 T_BE_INT64;
typedef T_INT64 T_LE_INT64;



/*
 * size checks
 */

TB_CHECK_SIZEOF(T_UINT8,1);
TB_CHECK_SIZEOF(T_UINT16,2);
TB_CHECK_SIZEOF(T_UINT32,4);
TB_CHECK_SIZEOF(T_UINT64,8);

TB_CHECK_SIZEOF(T_INT8,1);
TB_CHECK_SIZEOF(T_INT16,2);
TB_CHECK_SIZEOF(T_INT32,4);
TB_CHECK_SIZEOF(T_INT64,8);



/*
 * UNICODE character type
 */

#ifdef TBASE_COMPILER_MICROSOFT
// Microsoft uses 16-bit UNICODE characters
typedef wchar_t T_UNICHAR;
// Macro for defining string literals.
#define T_UNICHAR_TEXT(x)		L ## x
#endif

#ifdef TBASE_COMPILER_GNU_LINUX
// On Linux UNICODE is encoded as UTF-8
typedef char T_UNICHAR;
// Macro for defining string literals.
#define T_UNICHAR_TEXT(x)		x
#endif

#ifdef TBASE_COMPILER_APPLE
// On Mac UNICODE is encoded as UTF-8
typedef char T_UNICHAR;
// Macro for defining string literals.
#define T_UNICHAR_TEXT(x)		x
#endif

#endif  /* __tbase_types_h__ */

/*************************** EOF **************************************/
