/* 
* Copyright (c) 2000-2012 China IWNCOMM Co., Ltd. All rights reserved.
*
* Module:        ECC Algorithm           
* Description:   Public key infrastructure application technology 
*                application interface of cryptography algorithm.
* Writer:        zhangguoqiang@DST <zhanggq@iwncomm.com>;
*                wanhongtao@DST <wanht@iwncomm.com>
* Version:       $Id:mint.h,v 1.0 2011/12/09 09:00:00 zhangguoqiang Exp $;
*                $Id:mint.h,v 1.1 2012/11/21 09:00:00 wanhongtao Exp $ 
*
* History: 
* zhangguoqiang    2011/12/09    v1.0    Created.
* wanhongtao       2012/11/21    v1.1    Modify.
*/


#ifndef _ECC_ECDSA_ALG_API_H
#define _ECC_ECDSA_ALG_API_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "mint.h"
#include "ecc.h"

int ECDSA_ExternalSign_ECC(EccCurveHandle *Curve, MINT *PrivateKey, unsigned char *Data, unsigned int DataLength, MINT *r, MINT *s, int HashType);
int ECDSA_ExternalVerify_ECC(EccCurveHandle *Curve, ECC_FpPOINT *PublicKey, unsigned char *Data, unsigned int DataLength, MINT *r, MINT *s);
int ECES_ExternalEncrytp_ECC(EccCurveHandle *Curve, ECC_FpPOINT *PublicKey, unsigned char *Data, unsigned int DataLength, unsigned int FixLen, 
							 int HashType, unsigned char *XData, unsigned char *YData, unsigned char *CipherData,unsigned char *MData);
int ECES_ExternalDecrypt_ECC(EccCurveHandle *Curve, MINT *PrivateKey, unsigned int FixLen, unsigned char *XData, 
							 unsigned char *YData, unsigned char *CipherData, unsigned char *Data, unsigned int *DataLength);
int EcdhKeyAgreement(EccCurveHandle *Curve, MINT *PrivateKey, ECC_FpPOINT *PublicKey, unsigned int FixLen, unsigned char *KeyData);


#ifdef  __cplusplus
}
#endif

#endif /*_ECC_ECDSA_ALG_API_H*/