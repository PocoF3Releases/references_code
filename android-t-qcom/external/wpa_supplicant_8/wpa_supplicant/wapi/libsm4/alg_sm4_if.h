/* 
 * Copyright (C) 2016-2020 China IWNCOMM Co., Ltd. All rights reserved.
 * 
 * Contains function declarations for sm4 alg
 * 
 * Authors:  
 * chen weigang
 * 
 * History: 
 * chen weigang  07/04/2016 v1.0 Created
 * 
 * Related documents:
 * -GB/T 15629.11-2003/XG1-2006 
 * -ISO/IEC 8802.11:1999,MOD
 * 
*/ 

#ifndef _ALG_SM4_IF_H_
#define _ALG_SM4_IF_H_

int wpi_encrypt(unsigned char *pofbiv_in, unsigned char *pbw_in, 
		unsigned int plbw_in, unsigned char *pkey, unsigned char *pcw_out);

int wpi_decrypt(unsigned char *pofbiv_in, unsigned char *pbw_in, 
		unsigned int plbw_in, unsigned char *pkey, unsigned char *pcw_out);

#endif