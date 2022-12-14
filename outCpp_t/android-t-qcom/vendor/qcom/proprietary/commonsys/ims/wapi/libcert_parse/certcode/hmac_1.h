/*
 * Copyright (C) 2016-2020 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * This file contains functions for sha256&hmac.
 * this code changed from RFC 4634
 *
 * Authors:
 * <iwncomm@iwncomm.com>
 *
 * History:
 *   06/06/2016 v1.0 add by iwncomm
 *
 * Related documents:
 * hmac_1.h
 *
 * Description:
 *
 */

#ifndef _IWN_HMAC_1
#define _IWN_HMAC_1

#ifdef  __cplusplus
extern "C"
{
#endif

    int
    mhash_sha256_1 ( unsigned char* data , unsigned length ,
            unsigned char* digest );

#ifdef  __cplusplus
}
#endif

#endif /*_IWN_HMAC_20090216*/

