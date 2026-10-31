/* Copyright 2014 The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#define LOG_TAG "keystore-engine"

#include <log/log.h>
#include <sys/types.h>

#include "keystore2_engine.h"

extern "C" {

EVP_PKEY* EVP_PKEY_from_keystore(const char* key_id) __attribute__((visibility("default")));

ssize_t wrapper_keystore2_get(const char* key, size_t keyLength, uint8_t** value) __attribute__((visibility("default")));
char** wrapper_keystore2_list(const char *key, size_t keyLength, int *count) __attribute__((visibility("default")));

/* EVP_PKEY_from_keystore returns an |EVP_PKEY| that contains either an RSA or
 * ECDSA key where the public part of the key reflects the value of the key
 * named |key_id| in Keystore and the private operations are forwarded onto
 * KeyStore. */
EVP_PKEY* EVP_PKEY_from_keystore(const char* key_id) {
    ALOGV("EVP_PKEY_from_keystore(\"%s\")", key_id);

    return EVP_PKEY_from_keystore2(key_id);
}

ssize_t wrapper_keystore2_get(const char* alias, size_t keyLength, uint8_t** value) {
    ALOGV("wrapper_keystore2_get in android_engine");
    return keystore2_get(alias, keyLength, value);
}

char** wrapper_keystore2_list(const char *prefix, size_t keyLength, int *count) {
    ALOGV("wrapper_keystore2_list in android_engine");
    return keystore2_list(prefix, keyLength, count);
}

}  // extern "C"
