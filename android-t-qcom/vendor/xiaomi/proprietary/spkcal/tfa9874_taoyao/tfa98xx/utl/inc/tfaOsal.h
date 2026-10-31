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

#ifndef TFAOSAL_H_
#define TFAOSAL_H_

/* load hal plugin from dynamic lib */
void * tfaosal_plugload(char *libarg);
/*generic function to load library and symbol from the library*/
extern void * load_library(char *libname);
extern void * load_symbol(void *dlib_handle, char *dl_sym);

int tfaosal_filewrite(const char *fullname, unsigned char *buffer, int filelenght );

#define isSpaceCharacter(C) (C==' '||C=='\t')

#if defined (WIN32) || defined(_X64)
//suppress warnings for unsafe string routines.
#pragma warning(disable : 4996)
#pragma warning(disable : 4054)
char *basename(const char *fullpath);
#define bzero(ADDR,SZ)	memset(ADDR,0,SZ)
#endif

void tfaRun_Sleepus(int us);

/*
 * Read file
 */
int  tfaReadFile(char *fname, void **buffer);

#endif
