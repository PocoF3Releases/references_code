#include <ACHex.h>

#include <string.h>

#include "../common/log.h"

#undef MY_LOG_TAG
#define MY_LOG_TAG "ACHex"

bool ACHex::hex2bin(const char * str, std::vector<unsigned char> * pBin) {

    pBin->clear();

    size_t len = strlen(str);

    if (len & 1) {
        LOGE("ACHEX::hex2bin, the length of str is odd. ");
        return false;
    }

    pBin->resize(len >> 1, 0);

    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 'A';
        }

        if (i & 1) {
            pBin->at(i >> 1) <<= 4;
        }

        if (c >= 'A' && c <= 'F') {
            pBin->at(i >> 1) |= c - 'A' + 10;
        } else if (c >= '0' && c <= '9') {
            pBin->at(i >> 1) |= c - '0';
        } else {
            LOGE("ACHEX::hex2bin, bad character in str. ");
            return false;
        }
    }

    return true;
}

static char HEX(unsigned char x) {
    x = x & 0x0F;
    if (x < 10) return x + '0';
    return x - 10 + 'A';
}

void ACHex::bin2hex(const unsigned char * bin, size_t len, std::vector<char> * pStr) {

    pStr->clear();
    pStr->resize(len * 2 + 1, 0);

    for (size_t i = 0; i < len; i++) {
        pStr->at(i * 2) = HEX(bin[i] >> 4);
        pStr->at(i * 2 + 1) = HEX(bin[i]);
    }

    pStr->at(len * 2) = 0;
}
