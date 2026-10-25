#include "probeUtils.h"
#include <stdlib.h>
#include <string.h>

#define SWAPENDIAN(x)\
    (x = (x >> 8 & 0xff00ff) | (x & 0xff00ff) << 8, x = x >> 16 | x << 16)


struct Crypto1State *crypto1_create(uint64_t key) {
    struct Crypto1State *s = (struct Crypto1State *) malloc(sizeof(*s));
    memset(s, 0x0, sizeof(*s));
    int i;

    for (i = 47; s && i > 0; i -= 2) {
        s->odd = s->odd << 1 | BIT(key, (i - 1) ^ 7);
        s->even = s->even << 1 | BIT(key, i ^ 7);
    }
    return s;
}

void crypto1_destroy(struct Crypto1State *state) {
    free(state);
}

uint8_t crypto1_bit(struct Crypto1State *s, uint8_t in, int is_encrypted) {
    uint32_t feedin;
    uint8_t ret = filter(s->odd);

    feedin = ret & !!is_encrypted;
    feedin ^= !!in;
    feedin ^= LF_POLY_ODD & s->odd;
    feedin ^= LF_POLY_EVEN & s->even;
    s->even = s->even << 1 | parity(feedin);

    s->odd ^= (s->odd ^= s->even, s->even ^= s->odd);

    return ret;
}

uint8_t crypto1_byte(struct Crypto1State *s, uint8_t in, int is_encrypted) {
    uint8_t i, ret = 0;

    for (i = 0; i < 8; ++i)
        ret |= crypto1_bit(s, BIT(in, i), is_encrypted) << i;

    return ret;
}

uint32_t crypto1_word(struct Crypto1State *s, uint32_t in, int is_encrypted) {
    uint32_t i, ret = 0;

    for (i = 0; i < 32; ++i)
        ret |= crypto1_bit(s, BEBIT(in, i), is_encrypted) << (i ^ 24);

    return ret;
}

/* prng_successor
 * helper used to obscure the keystream during authentication
 */
uint32_t prng_successor(uint32_t x, uint32_t n) {
    SWAPENDIAN(x);

    while (n > 0) {
        n--;
        x = x >> 1 | (x >> 16 ^ x >> 18 ^ x >> 19 ^ x >> 21) << 31;
    }

    return SWAPENDIAN(x);
}

static uint16_t *dist = 0;

int nonce_distance(uint32_t from, uint32_t to) {
    uint16_t x, i;
    if (!dist) {
        dist = (uint16_t *) malloc(2 << 16);
        if (!dist)
            return -1;
        for (x = i = 1; i; ++i) {
            dist[(x & 0xff) << 8 | x >> 8] = i;
            x = x >> 1 | (x ^ x >> 2 ^ x >> 3 ^ x >> 5) << 15;
        }
    }
    return (65535 + dist[to >> 16] - dist[from >> 16]) % 65535;
}

// crc methods here
/**
 * @brief CRC_A
 *
 */
void iso14443a_crc(uint8_t *pbtData, size_t szLen, uint8_t *pbtCrc) {
    uint32_t wCrc = 0x6363;

    do {
        uint8_t bt;
        bt = *pbtData++;
        bt = (bt ^ (uint8_t)(wCrc & 0x00FF));
        bt = (bt ^ (bt << 4));
        wCrc = (wCrc >> 8) ^ ((uint32_t) bt << 8) ^ ((uint32_t) bt << 3) ^ ((uint32_t) bt >> 4);
    } while (--szLen);

    *pbtCrc++ = (uint8_t)(wCrc & 0xFF);
    *pbtCrc = (uint8_t)((wCrc >> 8) & 0xFF);
}

/**
 * @brief Append CRC_A
 *
 */
void iso14443a_crc_append(uint8_t *pbtData, size_t szLen) {
    iso14443a_crc(pbtData, szLen, pbtData + szLen);
}

/**
 * @brief CRC_B
 *
 */
void iso14443b_crc(uint8_t *pbtData, size_t szLen, uint8_t *pbtCrc) {
    uint32_t wCrc = 0xFFFF;

    do {
        uint8_t bt;
        bt = *pbtData++;
        bt = (bt ^ (uint8_t)(wCrc & 0x00FF));
        bt = (bt ^ (bt << 4));
        wCrc = (wCrc >> 8) ^ ((uint32_t) bt << 8) ^ ((uint32_t) bt << 3) ^ ((uint32_t) bt >> 4);
    } while (--szLen);
    wCrc = ~wCrc;
    *pbtCrc++ = (uint8_t)(wCrc & 0xFF);
    *pbtCrc = (uint8_t)((wCrc >> 8) & 0xFF);
}

/**
 * @brief Append CRC_B
 *
 */
void iso14443b_crc_append(uint8_t *pbtData, size_t szLen) {
    iso14443b_crc(pbtData, szLen, pbtData + szLen);
}


long long unsigned int bytes_to_num(uint8_t *src, uint32_t len) {
    uint64_t num = 0;
    while (len > 0) {
        len--;
        num = (num << 8) | (*src);
        src++;
    }
    return num;
}

uint8_t oddparity(const uint8_t bt)
{
    // cf http://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    return (0x9669 >> ((bt ^ (bt >> 4)) & 0xF)) & 1;
}
