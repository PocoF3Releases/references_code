/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _CKSUM_H_
#define _CKSUM_H_

extern unsigned int crc_table[256];
extern void crc_init(unsigned int *crc_table);
extern unsigned int cksum(unsigned int crc, unsigned char c);

#endif /* _CKSUM_H_ */
