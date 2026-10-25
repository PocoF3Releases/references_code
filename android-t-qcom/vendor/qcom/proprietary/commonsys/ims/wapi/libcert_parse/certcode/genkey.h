/*
 * Copyright (C) 2016-2020 China IWNCOMM Co., Ltd. All rights reserved.
 *
 * This module is wapi cert parse function.
 * this function parse cert.
 *
 * Authors:
 * <iwncomm@iwncomm.com>
 *
 * History:
 *   06/06/2016 v1.0 add by iwncomm
 *
 * Related documents:
 * genkey.h
 *
 * Description:
 *
 */
#ifndef _GENKEY_H
#define _GENKEY_H

#ifdef  __cplusplus
extern "C"
{
#endif

    int
    pass_gen_key ( const char *password , int p_len , unsigned char *salt ,
            int s_len , int iter , int id_tag , unsigned char *data ,
            int data_len );

    int
    unicode_gen_key ( unsigned char *password , int p_len ,
            unsigned char *salt , int s_len , int iter , int id_tag ,
            unsigned char *data , int data_len );

    unsigned char *
    ascii_to_unicode ( const char *asc_data , int asc_datalen ,
            unsigned char **uni_data , int *uni_datalen );
#ifdef  __cplusplus
}
#endif

#endif /*_GENKEY_H*/
