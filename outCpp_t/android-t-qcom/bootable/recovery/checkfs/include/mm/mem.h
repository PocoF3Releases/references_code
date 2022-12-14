/**
 * Copyright (c) 2015 The CheckFs Open Source Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _MM_MEM_H_
#define _MM_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>

#include <stdlib.h>
#include <string.h>

#include "err_code.h"

/* The inlined functions. */
#ifdef __GNUC__
// MIUI MOD : START
// #define _INLINE_ extern __inline__
 #define _INLINE_ __inline__
// END
#else
#defin _INLINE_ extern inline
#endif /* __GNUC__ */

_INLINE_ int checkfs_alloc_mem(unsigned long size, void *ptr)
{
    void *pp;

    pp = malloc(size);
    if (!pp)
        return CHECKFS_ERR_NO_MEMORY;
    memcpy(ptr, &pp, sizeof (pp));
    return 0;
}

_INLINE_ int checkfs_free_mem(void *ptr)
{
    void *p;

    memcpy(&p, ptr, sizeof(p));
    free(p);
    p = 0;
    memcpy(ptr, &p, sizeof(p));
    return 0;
}

_INLINE_ int checkfs_alloc_memalign(unsigned long size,
                                    unsigned long align, void *ptr)
{
    int ret;

    if (align == 0) {
        align = 8;
    }

    if ((ret = posix_memalign((void **) ptr, align, size))) {
        if (ret == ENOMEM)
            return CHECKFS_ERR_NO_MEMORY;
        return ret;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _MM_MEM_H_ */
