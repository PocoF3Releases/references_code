/* rsa.c
**
** Copyright 2012, The Android Open Source Project
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of Google Inc. nor the names of its contributors may
**       be used to endorse or promote products derived from this software
**       without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY Google Inc. ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
** EVENT SHALL Google Inc. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "miutil/mi_ersfrp_verification.h"
#include <stdio.h>
#include <string.h>

// a[] -= mod
static void subM(const RSAPublicKey* key,
                 uint32_t* a) {
    int64_t A = 0;
    int i;
    for (i = 0; i < key->len; ++i) {
        A += (uint64_t)a[i] - key->n[i];
        a[i] = (uint32_t)A;
        A >>= 32;
    }
}

// return a[] >= mod
static int geM(const RSAPublicKey* key,
               const uint32_t* a) {
    int i;
    for (i = key->len; i;) {
        --i;
        if (a[i] < key->n[i]) return 0;
        if (a[i] > key->n[i]) return 1;
    }
    return 1;  // equal
}

// montgomery c[] += a * b[] / R % mod
static void montMulAdd(const RSAPublicKey* key,
                       uint32_t* c,
                       const uint32_t a,
                       const uint32_t* b) {
    uint64_t A = (uint64_t)a * b[0] + c[0];
    uint32_t d0 = (uint32_t)A * key->n0inv;
    uint64_t B = (uint64_t)d0 * key->n[0] + (uint32_t)A;
    int i;

    for (i = 1; i < key->len; ++i) {
        A = (A >> 32) + (uint64_t)a * b[i] + c[i];
        B = (B >> 32) + (uint64_t)d0 * key->n[i] + (uint32_t)A;
        c[i - 1] = (uint32_t)B;
    }

    A = (A >> 32) + (B >> 32);

    c[i - 1] = (uint32_t)A;

    if (A >> 32) {
        subM(key, c);
    }
}

// montgomery c[] = a[] * b[] / R % mod
static void montMul(const RSAPublicKey* key,
                    uint32_t* c,
                    const uint32_t* a,
                    const uint32_t* b) {
    int i;
    for (i = 0; i < key->len; ++i) {
        c[i] = 0;
    }
    for (i = 0; i < key->len; ++i) {
        montMulAdd(key, c, a[i], b);
    }
}

// In-place public exponentiation.
// Input and output big-endian byte array in inout.
static void modpow(const RSAPublicKey* key,
                   uint8_t* inout) {
    uint32_t a[RSANUMWORDS];
    uint32_t aR[RSANUMWORDS];
    uint32_t aaR[RSANUMWORDS];
    uint32_t* aaa = 0;
    int i;

    // Convert from big endian byte array to little endian word array.
    for (i = 0; i < key->len; ++i) {
        uint32_t tmp =
            (inout[((key->len - 1 - i) * 4) + 0] << 24) |
            (inout[((key->len - 1 - i) * 4) + 1] << 16) |
            (inout[((key->len - 1 - i) * 4) + 2] << 8) |
            (inout[((key->len - 1 - i) * 4) + 3] << 0);
        a[i] = tmp;
    }

    if (key->exponent == 65537) {
        aaa = aaR;  // Re-use location.
        montMul(key, aR, a, key->rr);  // aR = a * RR / R mod M
        for (i = 0; i < 16; i += 2) {
            montMul(key, aaR, aR, aR);  // aaR = aR * aR / R mod M
            montMul(key, aR, aaR, aaR);  // aR = aaR * aaR / R mod M
        }
        montMul(key, aaa, aR, a);  // aaa = aR * a / R mod M
    } else if (key->exponent == 3) {
        aaa = aR;  // Re-use location.
        montMul(key, aR, a, key->rr);  /* aR = a * RR / R mod M   */
        montMul(key, aaR, aR, aR);     /* aaR = aR * aR / R mod M */
        montMul(key, aaa, aaR, a);     /* aaa = aaR * a / R mod M */
    }

    // Make sure aaa < mod; aaa is at most 1x mod too large.
    if (geM(key, aaa)) {
        subM(key, aaa);
    }

    // Convert to bigendian byte array
    for (i = key->len - 1; i >= 0; --i) {
        uint32_t tmp = aaa[i];
        *inout++ = tmp >> 24;
        *inout++ = tmp >> 16;
        *inout++ = tmp >> 8;
        *inout++ = tmp >> 0;
    }
}

// Verify a 2048-bit RSA PKCS1.5 signature against an expected msg.
// It's a special rsa_verify function for edl authentication.
// This function is PKCS1.5 padding and no hash.
//
// Returns CRYPT_OK(0) on successful verification, else on failure.
static int rsa_decrypt(const unsigned char *sig,   unsigned long siglen,
                        unsigned char *msg,  unsigned long msglen, RSAPublicKey *key){
    uint8_t buf[RSANUMBYTES];
    unsigned long i;

    // Wrong input
    if (sig == NULL || siglen != RSANUMBYTES) {
        return CRYPT_WRONG_INPUT;
    }
    // Wrong msg
    if (msg == NULL || msglen < MIN_MSGBYTES || msglen > siglen) {
        return CRYPT_WRONG_MSG;
    }
    // Copy input to local workspace.
    for (i = 0; i < siglen; ++i) {
        buf[i] = sig[i];
    }

    modpow(key, buf);  // In-place exponentiation.

    // Compare against expected signature format/or called padding
    if(buf[0] != 0x0){
        return CRYPT_WRONG_FORMAT;
    }
    if(buf[1] != 0x1){
        return CRYPT_WRONG_FORMAT;
    }
    for(i = 2; i < siglen-msglen-1; i++){
        if(buf[i] != 0xFF){
            return CRYPT_WRONG_FORMAT;
        }
    }
    if(siglen != msglen) {
        if(buf[siglen-msglen-1] != 0x0){
            return CRYPT_WRONG_FORMAT;
        }
    }

    // Compare against expected message value.
    for (i = siglen - msglen; i < siglen; i++) {
        if (buf[i] != msg[i - siglen + msglen]) {
            return CRYPT_VERIFY_ERROR;
        }
    }

    return CRYPT_OK;  // All checked out OK.
}

int rsa_decrypt(const unsigned char *sig,   unsigned long siglen,
                        unsigned char *msg,  unsigned long msglen){
    RSAPublicKey miPubkey = {
        .len = 64,
        .n0inv = 0x5d57403f,
        .n = {
            905662529u,957766561u,1528918386u,552169290u,
            100462982u,1440851386u,1374592318u,3763452574u,
            3340662753u,1978012860u,3548105177u,1902456168u,
            2676688098u,960423795u,1619973606u,2237410801u,
            2166034525u,3262235114u,403171324u,1013581853u,
            2429683077u,3228768161u,3554426153u,2826353473u,
            3686682203u,3052806549u,1020804279u,1175168975u,
            1369655585u,2102906991u,4197652445u,2959513877u,
            1968603856u,43661939u,2068023621u,354145005u,
            3989085151u,4068827928u,2255347563u,3703370911u,
            404593182u,2576167477u,1353367515u,1540741758u,
            1148633957u,3432509229u,1978942348u,1996055717u,
            3274016966u,596479647u,3487565409u,1392933311u,
            2022650037u,141089510u,2475418106u,2282028531u,
            3569538937u,1931400165u,2375711766u,3155423272u,
            247842987u,1326257092u,548939747u,3086732771u
        },
        .rr = {
            2744695467u,3502757586u,3404644260u,2597020995u,
            166193575u,4118278629u,1093792612u,2032260629u,
            2635535028u,3198061762u,764200268u,3701389351u,
            3691904126u,54283528u,3178120642u,4226019992u,
            3765175440u,3699267488u,138990107u,3134833244u,
            227810550u,82856746u,2155632811u,2236533545u,
            3608474298u,3616644239u,110460542u,1437263958u,
            4076689338u,2772116163u,1502962902u,1013625016u,
            1969422059u,1227352161u,3015287263u,1149510725u,
            3131076324u,1494726663u,1673233511u,401359119u,
            1867221334u,1082523808u,4247177022u,976950260u,
            3704856245u,1587395035u,2931056186u,860682223u,
            1217604751u,3772677212u,1018434169u,254048131u,
            510325049u,1014892321u,685336956u,1400428320u,
            1460491755u,4156694112u,3575175713u,1640723620u,
            589236533u,2993162234u,1106359266u,1235684862u
        },
        .exponent = 65537,
    };
    return rsa_decrypt(sig, siglen, msg, msglen, &miPubkey);
}
