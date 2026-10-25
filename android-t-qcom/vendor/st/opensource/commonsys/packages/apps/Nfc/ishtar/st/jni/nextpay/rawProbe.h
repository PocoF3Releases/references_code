#ifndef MI_RAW_PROBE_RAW_PROBE_H
#define MI_RAW_PROBE_RAW_PROBE_H

#include <stdint.h>

#define NR_TRAILERS_1k  (16)
#define MAX_FRAME_LEN 264

// Default number of distance probes
#define DEFAULT_DIST_NR       7
// Default number of probes for a key recovery for one sector
#define DEFAULT_PROBES_NR      3
// Number of sets with 32b keys
#define DEFAULT_SETS_NR        4

#define DEVIATION_LIMIT        220

#define TRY_KEYS               50

//DEFAULT_PROBES_NR*TRY_KEYS
#define SECTOR_TRY_KEYS        150

#define odd_parity(i) (( (i) ^ (i)>>1 ^ (i)>>2 ^ (i)>>3 ^ (i)>>4 ^ (i)>>5 ^ (i)>>6 ^ (i)>>7 ^ 1) & 0x01)

typedef enum {
    MC_AUTH_A = 0x60,
    MC_AUTH_B = 0x61,
    MC_READ = 0x30,
    MC_WRITE = 0xA0,
    MC_TRANSFER = 0xB0,
    MC_DECREMENT = 0xC0,
    MC_INCREMENT = 0xC1,
    MC_STORE = 0xC2
} mifare_cmd;

typedef struct {
    uint32_t median;
    uint32_t deviation;
    uint32_t Nt;
    uint32_t NtE;
    uint8_t parity[3];              // used for 3 bits of parity information
} ntprobe;                                // Revealed information about nonce

typedef struct {
    uint8_t KeyA[6];
    uint8_t KeyB[6];
    bool foundKeyA;
    bool foundKeyB;
    uint8_t trailer;                         // Value of a trailer block, index of last block of sector
} sector;

typedef struct {
    uint32_t *distances;
    uint32_t median;
    uint32_t num_distances;
    uint32_t deviation;                   // deviation
} denonce;                                      // Revealed information about nonce

typedef struct {
    sector sectors[NR_TRAILERS_1k];      //
    uint8_t num_sectors;
    uint32_t e_sector_index;                     // Exploit sector
    uint32_t authuid;
} mftag;

int mf_enhanced_auth(int e_sector, int a_sector, mftag *tag_p, denonce *nonce_p, char mode, bool dumpKeysA, ntprobe *nt_probe_p);

#endif //MI_RAW_PROBE_RAW_PROBE_H
