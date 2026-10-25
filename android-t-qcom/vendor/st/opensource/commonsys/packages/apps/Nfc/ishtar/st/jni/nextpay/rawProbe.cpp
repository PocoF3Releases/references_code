#include "rawProbe.h"

#include "json.h"
#include "probeUtils.h"
#include "nfcInitiator.h"
#include "alog.h"
#include <stdio.h>

uint32_t median(denonce *d);

int mf_enhanced_auth(int e_sector, int a_sector, mftag *tag_p, denonce *nonce_p, char mode, bool dumpKeysA, ntprobe *nt_probe_p) {
    struct Crypto1State *pcs;

    uint8_t Nr[4] = {0x00, 0x00, 0x00, 0x00}; // Reader nonce
    uint8_t Auth[4] = {0x00, tag_p->sectors[e_sector].trailer, 0x00, 0x00};
    uint8_t AuthEnc[4] = {0x00, tag_p->sectors[e_sector].trailer, 0x00, 0x00};
    uint8_t AuthEncPar[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t ArEnc[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t ArEncPar[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t Rx[MAX_FRAME_LEN]; // Tag response
    uint8_t RxPar[MAX_FRAME_LEN]; // Tag response

    uint32_t Nt, NtLast;

    int i;
    uint32_t m;
    static uint32_t skipProbeNum = 0;

    // make sure dist array is initialized
    nonce_distance(0, 0);

    mf_nfc_mode_configure(1);

    // Prepare AUTH command
    Auth[0] = (tag_p->sectors[e_sector].foundKeyA) ? MC_AUTH_A : MC_AUTH_B;
    iso14443a_crc_append(Auth, 2);
    //PRINT("Auth command:\t");
    //print_hex(Auth, 4);

    if (mf_transceive_bytes(Auth, 4, Rx) < 0) {
        PRINT("Error while requesting plain tag-nonce\n");
        return -1;
    }

    // Save the tag nonce (Nt)
    Nt = bytes_to_num(Rx, 4);

    // Init the cipher with key {0..47} bits
    if (tag_p->sectors[e_sector].foundKeyA) {
        pcs = crypto1_create(bytes_to_num(tag_p->sectors[e_sector].KeyA, 6));
    } else {
        pcs = crypto1_create(bytes_to_num(tag_p->sectors[e_sector].KeyB, 6));
    }

    // Load (plain) uid^nt into the cipher {48..79} bits
    crypto1_word(pcs, bytes_to_num(Rx, 4) ^ tag_p->authuid, 0);

    // Generate (encrypted) nr+parity by loading it into the cipher
    for (i = 0; i < 4; i++) {
        // Load in, and encrypt the reader nonce (Nr)
        ArEnc[i] = crypto1_byte(pcs, Nr[i], 0) ^ Nr[i];
        ArEncPar[i] = filter(pcs->odd) ^ oddparity(Nr[i]);
    }
    // Skip 32 bits in the pseudo random generator
    Nt = prng_successor(Nt, 32);
    // Generate reader-answer from tag-nonce
    for (i = 4; i < 8; i++) {
        // Get the next random byte
        Nt = prng_successor(Nt, 8);
        // Encrypt the reader-answer (Nt' = suc2(Nt))
        ArEnc[i] = crypto1_byte(pcs, 0x00, 0) ^ (Nt & 0xff);
        ArEncPar[i] = filter(pcs->odd) ^ oddparity(Nt);
    }

    int res;
    if (((res = mf_transceive_bits(ArEnc, 64, ArEncPar, Rx, RxPar)) < 0) || (res != 32)) {
        crypto1_destroy(pcs);
        ERR("Reader-answer transfer error, exiting..");
        return -1;
    }

    // Decrypt the tag answer and verify that suc3(Nt) is At
    Nt = prng_successor(Nt, 32);
    if (!((crypto1_word(pcs, 0x00, 0) ^ bytes_to_num(Rx, 4)) == (Nt & 0xFFFFFFFF))) {
        crypto1_destroy(pcs);
        ERR("[At] is not Suc3(Nt), something is wrong, exiting..");
        return -1;
    }

    PRINT("Auth success!\n");

    // If we are in "Get Distances" mode
    if (mode == 'd') {
        skipProbeNum = 0;
        for (m = 0; m < nonce_p->num_distances; m++) {
            // Encrypt Auth command with the current keystream
            for (i = 0; i < 4; i++) {
                AuthEnc[i] = crypto1_byte(pcs, 0x00, 0) ^ Auth[i];
                // Encrypt the parity bits with the 4 plaintext bytes
                AuthEncPar[i] = filter(pcs->odd) ^ oddparity(Auth[i]);
            }

            // Sending the encrypted Auth command
            if (mf_transceive_bits(AuthEnc, 32, AuthEncPar, Rx, RxPar) < 0) {
                PRINT("Error requesting encrypted tag-nonce\n");
                crypto1_destroy(pcs);
                //return (void*)-1;
                return -1;
            }

            // Decrypt the encrypted auth
            if (tag_p->sectors[e_sector].foundKeyA) {
                pcs = crypto1_create(bytes_to_num(tag_p->sectors[e_sector].KeyA, 6));
            } else {
                pcs = crypto1_create(bytes_to_num(tag_p->sectors[e_sector].KeyB, 6));
            }
            NtLast = bytes_to_num(Rx, 4) ^ crypto1_word(pcs, bytes_to_num(Rx, 4) ^ tag_p->authuid, 1);

            // Save the determined nonces distance
            nonce_p->distances[m] = nonce_distance(Nt, NtLast);
            //PRINT("%d,", d->distances[m]);

            // Again, prepare and send {At}
            for (i = 0; i < 4; i++) {
                ArEnc[i] = crypto1_byte(pcs, Nr[i], 0) ^ Nr[i];
                ArEncPar[i] = filter(pcs->odd) ^ oddparity(Nr[i]);
            }
            Nt = prng_successor(NtLast, 32);
            for (i = 4; i < 8; i++) {
                Nt = prng_successor(Nt, 8);
                ArEnc[i] = crypto1_byte(pcs, 0x00, 0) ^ (Nt & 0xFF);
                ArEncPar[i] = filter(pcs->odd) ^ oddparity(Nt);
            }
            if (((res = mf_transceive_bits(ArEnc, 64, ArEncPar, Rx, RxPar)) < 0) || (res != 32)) {
                ERR("Reader-answer transfer error, exiting..");
                crypto1_destroy(pcs);
                //return (void*)-1;
                return -1;
            }
            Nt = prng_successor(Nt, 32);
            if (!((crypto1_word(pcs, 0x00, 0) ^ bytes_to_num(Rx, 4)) == (Nt & 0xFFFFFFFF))) {
                ERR("[At] is not Suc3(Nt), something is wrong, exiting..");
                crypto1_destroy(pcs);
                //return (void*)-1;
                return -1;
            }
            //PRINT("AuthEnc success!\n");
        } // Next auth probe

        // Find median from all distances
        nonce_p->median = median(nonce_p);
    } // The end of Get Distances mode

    // If we are in "Get Recovery" mode
    if (mode == 'r') {
        if (nonce_p->deviation == 0)
            skipProbeNum++;
        for (m = 0; m < skipProbeNum; m++) {
            // Encrypt Auth command with the current keystream
            for (i = 0; i < 4; i++) {
                AuthEnc[i] = crypto1_byte(pcs, 0x00, 0) ^ Auth[i];
                // Encrypt the parity bits with the 4 plaintext bytes
                AuthEncPar[i] = filter(pcs->odd) ^ oddparity(Auth[i]);
            }

            // Sending the encrypted Auth command
            if (mf_transceive_bits(AuthEnc, 32, AuthEncPar, Rx, RxPar) < 0) {
                PRINT("Error requesting encrypted tag-nonce\n");
                crypto1_destroy(pcs);
                //return (void*)-1;
                return -1;
            }

            // Decrypt the encrypted auth
            if (tag_p->sectors[e_sector].foundKeyA) {
                pcs = crypto1_create(bytes_to_num(tag_p->sectors[e_sector].KeyA, 6));
            } else {
                pcs = crypto1_create(bytes_to_num(tag_p->sectors[e_sector].KeyB, 6));
            }
            NtLast = bytes_to_num(Rx, 4) ^ crypto1_word(pcs, bytes_to_num(Rx, 4) ^ tag_p->authuid, 1);

            // Again, prepare and send {At}
            for (i = 0; i < 4; i++) {
                ArEnc[i] = crypto1_byte(pcs, Nr[i], 0) ^ Nr[i];
                ArEncPar[i] = filter(pcs->odd) ^ oddparity(Nr[i]);
            }
            Nt = prng_successor(NtLast, 32);
            for (i = 4; i < 8; i++) {
                Nt = prng_successor(Nt, 8);
                ArEnc[i] = crypto1_byte(pcs, 0x00, 0) ^ (Nt & 0xFF);
                ArEncPar[i] = filter(pcs->odd) ^ oddparity(Nt);
            }
            if (((res = mf_transceive_bits(ArEnc, 64, ArEncPar, Rx, RxPar)) < 0) || (res != 32)) {
                ERR("Reader-answer transfer error, exiting..");
                crypto1_destroy(pcs);
                //return (void*)-1;
                return -1;
            }
            Nt = prng_successor(Nt, 32);
            if (!((crypto1_word(pcs, 0x00, 0) ^ bytes_to_num(Rx, 4)) == (Nt & 0xFFFFFFFF))) {
                ERR("[At] is not Suc3(Nt), something is wrong, exiting..");
                crypto1_destroy(pcs);
                //return (void*)-1;
                return -1;
            }
        } // Next auth probe skipProbeNum

        Auth[0] = dumpKeysA ? MC_AUTH_A : MC_AUTH_B;
        Auth[1] = a_sector;
        iso14443a_crc_append(Auth, 2);

        // Encryption of the Auth command, sending the Auth command
        for (i = 0; i < 4; i++) {
            AuthEnc[i] = crypto1_byte(pcs, 0x00, 0) ^ Auth[i];
            // Encrypt the parity bits with the 4 plaintext bytes
            AuthEncPar[i] = filter(pcs->odd) ^ oddparity(Auth[i]);
        }
        if (mf_transceive_bits(AuthEnc, 32, AuthEncPar, Rx, RxPar) < 0) {
            PRINT("while requesting encrypted tag-nonce");
            crypto1_destroy(pcs);
            //return (void*)-1;
            return -1;
        }

        // Save the encrypted nonce
        nt_probe_p->NtE = bytes_to_num(Rx, 4);
        // Parity validity check
        for (i = 0; i < 3; ++i) {
            nt_probe_p->parity[i] = (oddparity(Rx[i]) != RxPar[i]);
        }
        nt_probe_p->median = nonce_p->median;
        nt_probe_p->deviation = nonce_p->deviation;
        nt_probe_p->Nt = Nt;
    }

    crypto1_destroy(pcs);
    return 0;
}

int compare_int_32(const void *a, const void *b) {
    const uint32_t* x = (uint32_t*) a;
    const uint32_t* y = (uint32_t*) b;
    if (*x > *y) {
        return 1;
    } else if (*x < *y) {
        return -1;
    }
    return 0;
}

uint32_t median(denonce *d) {
    int middle = (int) (d->num_distances / 2);

    qsort(d->distances, d->num_distances, sizeof(uint32_t), compare_int_32);

    if (d->num_distances % 2 == 1) {
        // Odd number of elements
        return d->distances[middle];
    } else {
        // Even number of elements, return the smaller value
        return d->distances[middle - 1];
    }
}



