#ifndef CRYPTO_H
#define CRYPTO_H

#include <jni.h>
#include <string.h>
#include "common.h"


/**
 * eg:
 * char in[] = "7A6BCD";
 * uint8 out[] = {0x76, 0x6B, 0xCD}
 * in_len = strlen(in);
 */
char* jstringTochar(jstring jstr);
void hexStringToBytes(uint8 *out, char *in, int in_len);
int checkException(JNIEnv* jniEnv);
int isTestCard();
int getImsi(uint8 *imsi);
int getIccid(uint8 *iccid);
int getKi(uint8 *ki);
int getKiForSkb(uint8 **ki_addr);//for skb authentication
int getSkbDeviceId(uint8 **device_id_addr);
int getSkbIdentityKey(uint8 **plain_skb_identity_key_addr);
int getSkbPrivateKey(uint8 **skb_prikey_addr);
int getSkbVersion();
int getOpc(uint8 *opc);
int getPlmnwact(uint8 *plmnwact);
int getFplmn(uint8 *fplmn);
int getHrpdss(uint8 *hrpdss);
int getHrpdssForSkb(uint8 **hrpdss_addr);
int getR15(uint8 *r15);
jbyteArray decryptRsaOaep(jstring cipherKi_str);
int aes_decrypt(char* in, int in_len, char* key, char* out);
int aes_encrypt(char* in, int in_len, char* key, char* out);

#endif
