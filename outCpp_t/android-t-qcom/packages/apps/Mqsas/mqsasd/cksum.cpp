#include "cksum.h"

unsigned int crc_table[256];

// Create a 256 entry CRC32 lookup table.
void crc_init(unsigned int *crc_table)
{
    unsigned int i;

    // Init the CRC32 table (big endian)
    for (i = 0; i < 256; i++) {
        unsigned int j, c = i<<24;

        for (j = 8; j; j--)
            c = c&0x80000000 ? (c<<1)^0x04c11db7 : (c<<1);

        crc_table[i] = c;
    }
}

unsigned int cksum(unsigned int crc, unsigned char c)
{
    return (crc << 8) ^ crc_table[((crc >> 24) ^ c) & 255];
}
