#ifndef MILENAGE_H
#define MILENAGE_H

#include <jni.h>
#include <string.h>
#include "common.h"

void initR15();

void milenage_f1( const uint8 k[16], const uint8 rand[16], uint8 sqn[6], uint8 amf[2],
        uint8 mac_a[8] );

void milenage_f2345 ( uint8 k[16], uint8 rand[16],
        uint8 res[8], uint8 ck[16], uint8 ik[16], uint8 ak[6] );

#endif
