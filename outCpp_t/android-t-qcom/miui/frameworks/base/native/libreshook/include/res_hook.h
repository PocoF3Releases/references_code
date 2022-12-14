#ifndef _RES_HOOK_H
#define _RES_HOOK_H

#include "aapt_hook.h"

extern "C" {

const int COMPARE_BETTER = 1;
const int COMPARE_WORSE = 2;
const int COMPARE_CONTINUE = 3;

const int MATCH_MATCH = 1;
const int MATCH_UNMATCH = 2;
const int MATCH_CONTINUE = 3;

int matchMiuiMccMnc(uint16_t mcc, uint16_t mnc);
int isMiuiMccMncBetterThan(uint16_t mcc, uint16_t mnc, uint16_t omcc, uint16_t omnc);

}

#endif
