////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015, 2017 Cirrus Logic International (UK) Ltd.  All rights reserved.
//
// This software as well as any related documentation is furnished under 
// license and may only be used or copied in accordance with the terms of the 
// license. The information in this file is furnished for informational use 
// only, is subject to change without notice, and should not be construed as 
// a commitment by Cirrus Logic International (UK) Ltd.  Cirrus Logic International
// (UK) Ltd assumes no responsibility or liability for any errors or inaccuracies
// that may appear in this document or any software that may be provided in
// association with this document. 
//
// Except as permitted by such license, no part of this document may be 
// reproduced, stored in a retrieval system, or transmitted in any form or by 
// any means without the express written consent of Cirrus Logic International
// (UK) Ltd or affiliated companies.
//
/// @file   ftw.cpp
/// @brief  Implementation of file tree walk module.
///
/// @version \$Id: ftw.cpp 19856 2017-12-14 14:16:28Z stankic $
///
/// @warning
///   This software is specifically written for Cirrus Logic devices.
///   It may not be used with other devices.
///
////////////////////////////////////////////////////////////////////////////////

/* $NetBSD: ftw.c,v 1.1 2005/12/30 23:07:32 agc Exp $ */

/*	From OpenBSD: ftw.c,v 1.2 2003/07/21 21:15:32 millert Exp 	*/

/*
 * Copyright (c) 2003 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Sponsored in part by the Defense Advanced Research Projects
 * Agency (DARPA) and Air Force Research Laboratory, Air Force
 * Materiel Command, USAF, under agreement number F39502-99-1-0512.
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: ftw.c,v 1.1 2005/12/30 23:07:32 agc Exp $");
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fts.h>
#include <ftw.h>
#include <limits.h>

/////////////////////////////////////////////////////////////////////////
// From NDK version 14b, include directories structure is updated and 
// OPEN_MAX and __UNCONST macros are not available.
/////////////////////////////////////////////////////////////////////////
// OPEN_MAX macro was part of <arch>/usr/include/sys/limits.h
// Following macro is taken from limits.h header from NDK 12b
// NOTE: multiple limits.h files for different platforms has OPEN_MAX
// set to same value (256)
#define OPEN_MAX      (256)
// __UNCONST macro was part of cdefs.h header file and in latest
// version of NDK this macro is removed from file
// Following macro is taken from  cdefs.h header from NDK 12b
#define __UNCONST(a)  ((void *)(unsigned long)(const void *)(a))
/////////////////////////////////////////////////////////////////////////

int
ftw(const char *path, int (*fn)(const char *, const struct stat *, int),
    int nfds)
{
	/* LINTED */
    //
    // MODIFIED NOT TO COMPLAIN ABOUT INVALID CONVERSION
    //
	char * const paths[2] = { (char*)__UNCONST(path), NULL };
	FTSENT *cur;
	FTS *ftsp;
	int fnflag, error, sverrno;

	/* XXX - nfds is currently unused */
	if (nfds < 1 || nfds > OPEN_MAX) {
		errno = EINVAL;
		return (-1);
	}

	ftsp = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, NULL);
	if (ftsp == NULL)
		return (-1);
	error = 0;
	while ((cur = fts_read(ftsp)) != NULL) {
		switch (cur->fts_info) {
		case FTS_D:
			fnflag = FTW_D;
			break;
		case FTS_DNR:
			fnflag = FTW_DNR;
			break;
		case FTS_DP:
			/* we only visit in preorder */
			continue;
		case FTS_F:
		case FTS_DEFAULT:
			fnflag = FTW_F;
			break;
		case FTS_NS:
		case FTS_NSOK:
		case FTS_SLNONE:
			fnflag = FTW_NS;
			break;
		case FTS_SL:
			fnflag = FTW_SL;
			break;
		case FTS_DC:
			errno = ELOOP;
			/* FALLTHROUGH */
		default:
			error = -1;
			goto done;
		}
		error = fn(cur->fts_path, cur->fts_statp, fnflag);
		if (error != 0)
			break;
	}
done:
	sverrno = errno;
	if (fts_close(ftsp) != 0 && error == 0)
		error = -1;
	else
		errno = sverrno;
	return (error);
}
