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

#ifndef __CONFIG_WINDOWS_INC__
#define  __CONFIG_WINDOWS_INC__

#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "TFA_I2C.h"

#include <winsock.h> /* htons() */

#define PAGE_SIZE	4096

#if BYTE_ORDER == LITTLE_ENDIAN
  #define htobe16(x) htons(x)
#elif BYTE_ORDER == BIG_ENDIAN
  #define htobe16(x) (x)
#else
  #error byte order not supported
#endif

#ifndef snprintf
#define snprintf(str, size, format, ...) _snprintf((str), (size), (format), __VA_ARGS__)
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#define tfa98xx_trace_printk PRINT
#define pr_debug PRINT
#define pr_info PRINT
#define pr_err ERRORMSG

#define GFP_KERNEL 0

typedef unsigned gfpt_t;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define BIT(x) (1 << (x))

#ifndef cpu_to_be16
#define cpu_to_be16 htobe16
#endif

/* tfa/src/tfa_osal.c */
void *kmalloc(size_t size, gfpt_t flags);
void kfree(const void *ptr);

#define kmem_cache_alloc(cachep, flags)    kmalloc(PAGE_SIZE, flags)
#define kmem_cache_free(cachep, ptr)       kfree(ptr)

/* tfa/src/tfa_osal.c */
unsigned long msleep_interruptible(unsigned int msecs);

/** 
 * Obtain the calculated crc.
 * tfa/src/tfa_container_crc32.c 
 */
uint32_t crc32_le(uint32_t crc, unsigned char const *buf, size_t len);

#endif /*  __CONFIG_WINDOWS_INC__ */

