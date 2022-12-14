#ifndef ANDROID_ACHEX_H
#define ANDROID_ACHEX_H

#include <stddef.h>
#include <vector>

class ACHex {
public:
    static bool hex2bin(const char * str, std::vector<unsigned char> * pBin);
    static void bin2hex(const unsigned char * bin, size_t len, std::vector<char> * pStr);
};

#endif //ANDROID_HEX_H
