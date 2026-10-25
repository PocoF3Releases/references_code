#include <jni.h>
#include <string>
#include <dlfcn.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include "openssl/evp.h"
#include "openssl/aes.h"

#define LOG_TAG "thermalconfig"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
using namespace std;

/*
 * Function: en_de_crypt
 *@src filepath of sourcefile
 *@target filepath of targetfile
 *@en_or_de 0 represent decrypt file, 1 represent encrypt file
 * return 0 represent fail, 1 represent success
 */

extern "C" JNIEXPORT jint JNICALL Java_com_miui_powerkeeper_thermalconfig_ThermalConfigCrypt_en_1de_1crypt(JNIEnv * env, jobject jobj, jstring src, jstring target, jint en_or_de) {
    const char * c_src = env-> GetStringUTFChars(src, NULL);
    const char * c_target = env -> GetStringUTFChars(target, NULL);
    FILE * ifp = NULL;
    FILE * ofp = NULL;
    int ret = 0;
    unsigned char ckey[] = "thermalopenssl.h";
    unsigned char ivec[] = "thermalopenssl.h";
    const unsigned BUFSIZE = 4096;
    unsigned char *read_buf = NULL;
    unsigned char *cipher_buf = NULL;
    unsigned blocksize = 0;
    int out_len = 0;
    EVP_CIPHER_CTX ctx;

    ifp = fopen(c_src, "rb");
    if (ifp == NULL) {
        LOGD("cannot open src file %s", c_src);
        goto ifp_null;
    }
    ofp = fopen(c_target, "wb");
    if (ofp == NULL) {
        LOGD("cannot open target file %s", c_target);
        goto ofp_null;
    }

    EVP_CipherInit(&ctx, EVP_aes_128_cbc(), ckey, ivec, en_or_de);
    if(ctx.cipher)
        blocksize = EVP_CIPHER_CTX_block_size(&ctx);
    else {
        LOGD("CipherInit fail");
        goto init_fail;
    }
    cipher_buf = static_cast<unsigned char *>(malloc(BUFSIZE + blocksize));
    read_buf = static_cast<unsigned char *>(malloc(BUFSIZE));
    if(cipher_buf == NULL || read_buf == NULL) {
        LOGD("memory short!");
        goto mem_short;
    }

    while (1) {
        // Read in data in blocks until EOF. Update the ciphering with each read.
        int numRead = fread(read_buf, sizeof(unsigned char), BUFSIZE, ifp);
        EVP_CipherUpdate(&ctx, cipher_buf, &out_len, read_buf, numRead);
        fwrite(cipher_buf, sizeof(unsigned char), out_len, ofp);
        if (numRead < BUFSIZE) {
            break;
        }
    }

    // Now cipher the final block and write it out.
    EVP_CipherFinal_ex(&ctx, cipher_buf, &out_len);
    fwrite(cipher_buf, sizeof(unsigned char), out_len, ofp);

    // Free resource
    ret = 1;
mem_short:
    free(cipher_buf);
    free(read_buf);
init_fail:
    EVP_CIPHER_CTX_cleanup(&ctx);
    fclose(ofp);
ofp_null:
    fclose(ifp);
ifp_null:
    return ret;
}
