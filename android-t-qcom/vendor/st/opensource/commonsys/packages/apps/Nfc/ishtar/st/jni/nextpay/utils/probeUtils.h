#ifndef MI_RAW_PROBE_PROBE_UTILS_H
#define MI_RAW_PROBE_PROBE_UTILS_H

#include <stdint.h>
#include <stdbool.h>

struct Crypto1State {uint32_t odd, even;};
struct Crypto1State *crypto1_create(uint64_t);
void crypto1_destroy(struct Crypto1State *);
  
uint8_t crypto1_byte(struct Crypto1State *, uint8_t, int);
uint32_t crypto1_word(struct Crypto1State *, uint32_t, int);
uint32_t prng_successor(uint32_t x, uint32_t n);
int nonce_distance(uint32_t from, uint32_t to);

#define LF_POLY_ODD (0x29CE5C)
#define LF_POLY_EVEN (0x870804)
#define BIT(x, n) ((x) >> (n) & 1)
#define BEBIT(x, n) BIT(x, (n) ^ 24)

static inline int parity(uint32_t x)
{
   x ^= x >> 16;
   x ^= x >> 8;
   x ^= x >> 4;
   return BIT(0x6996, x & 0xf);
}
  
static inline int filter(uint32_t const x)
{
    uint32_t f;

    f  = 0xf22c0 >> (x       & 0xf) & 16;
    f |= 0x6c9c0 >> (x >>  4 & 0xf) &  8;
    f |= 0x3c8b0 >> (x >>  8 & 0xf) &  4;
    f |= 0x1e458 >> (x >> 12 & 0xf) &  2;
    f |= 0x0d938 >> (x >> 16 & 0xf) &  1;
    return BIT(0xEC57E80A, f);
}

void iso14443a_crc_append(uint8_t *pbtData, size_t szLen);

uint8_t  oddparity(const uint8_t bt);
void  sprint_hex(char *buf, const uint8_t *pbtData, const size_t szBytes);
void  sprint_hex_par(char * buf, const uint8_t *pbtData, const size_t szBits, const uint8_t *pbtDataPar);

typedef struct card_ats {
    uint8_t sak;
    uint8_t uid[4];
} card_ats_t;

long long unsigned int bytes_to_num(uint8_t *src, uint32_t len);

uint8_t  oddparity(const uint8_t bt);

#endif
