//  Copyright (C) 2022 Xiaomi.inc
//  Author:gaolei6@xiaomi.com
//  Create:2022-10-11

#include "minikin/MLEOptimizer.h"

#include <log/log.h>
// below for utf-utils
#include <cstdlib>
#include <string>
#include <vector>

#include <unicode/utf.h>
#include <unicode/utf8.h>

#include "minikin/U16StringPiece.h"

namespace minikin {
uint16_t MLEOptimizerMatch::ZH_HANS_MIN = 19968;// U+4E00
uint16_t MLEOptimizerMatch::ZH_HANS_MAX = 40869; // U+9FA5
uint16_t MLEOptimizerMatch::EN_A = 65; // U+0041
uint16_t MLEOptimizerMatch::EN_Z = 90; // U+005A
uint16_t MLEOptimizerMatch::EN_a = 97; // U+0061
uint16_t MLEOptimizerMatch::EN_z = 122; // U+007A
uint16_t MLEOptimizerMatch::NUM_0 = 48; // U+0030
uint16_t MLEOptimizerMatch::NUM_9 = 57; // U+0039
uint16_t MLEOptimizerMatch::SPACE = 32;// U+0020

bool MLEOptimizer::isBoPoMoFoText(uint16_t codePoint) {
    if (codePoint >= mMLEOptimizerMatch.ZH_HANS_MIN && codePoint <= mMLEOptimizerMatch.ZH_HANS_MAX) {
        return true;
    }
    return false;
}
bool MLEOptimizer::isEnglish(uint16_t codePoint) {
    if ((codePoint >= mMLEOptimizerMatch.EN_A && codePoint <= mMLEOptimizerMatch.EN_Z)
        || (codePoint >= mMLEOptimizerMatch.EN_a && codePoint <= mMLEOptimizerMatch.EN_z)) {
        return true;
    }
    return false;
}
bool MLEOptimizer::isNumber(uint16_t codePoint) {
    if (codePoint >= mMLEOptimizerMatch.NUM_0 && codePoint <= mMLEOptimizerMatch.NUM_9) {
        return true;
    }
    return false;
}
std::pair<bool, bool> MLEOptimizer::isMatch(uint16_t codePointLeft /*left glyphId*/, uint16_t codePointRight /* right glypyId */) {
    bool isHansNumPattern = false;
    if (isEnglish(codePointLeft) && isBoPoMoFoText(codePointRight)) {
        return std::make_pair(true, isHansNumPattern);
    }
    if (isNumber(codePointLeft) && isBoPoMoFoText(codePointRight)) {
        isHansNumPattern = true;
        return std::make_pair(true, isHansNumPattern);
    }
    //
    if (isBoPoMoFoText(codePointLeft) && isEnglish(codePointRight)) {
        return std::make_pair(true, isHansNumPattern);
    }
    if (isBoPoMoFoText(codePointLeft) && isNumber(codePointRight)) {
        isHansNumPattern = true;
        return std::make_pair(true, isHansNumPattern);
    }
    return std::make_pair(false, isHansNumPattern);
}

//
void MLEOptimizer::resetLastPieceTail([[maybe_unused]] const U16StringPiece& text, uint16_t codePoint) {
    viewPieceEndGlyph_id = codePoint;
}
uint16_t MLEOptimizer::getLastPieceTail([[maybe_unused]] const U16StringPiece& text) {
    return viewPieceEndGlyph_id;
}

// For range measure
uint16_t MLEOptimizer::getPieceTailAt(const U16StringPiece& text, size_t pos) {
    return text.at(pos);
}

std::vector<uint16_t> utf8ToUtf16(const std::string& text);
void MLEOptimizer::initSpace(bool isRtl, const MinikinPaint& paint, StartHyphenEdit startHyphen,
                         EndHyphenEdit endHyphen) {
    mIsRtl = isRtl;
     if (paint.font.get() == mPaint.font.get()) {
         return;
     } else {
        mPaint = paint;
     }
    std::string text = std::string(" ");
    auto utf16 = minikin::utf8ToUtf16(text);
    MIUILayoutPiece mask_layout(utf16, Range(0, 1), isRtl, paint, startHyphen, endHyphen, shouldInitSpaceGap());
    if (mask_layout.glyphCount() != 1) {
        return;
    }
    if (shouldInitSpaceGap()) {
        systemSpaceGap = mask_layout.advanceAt(0);
        defaultMiuiGap = systemSpaceGap * mGapFactor;
        // normalize for best gap
        if (defaultMiuiGap > 15.0f) {
            defaultMiuiGap = 15.0f;
        }
        if (defaultMiuiGap < 0.0f) {
            defaultMiuiGap = 0.0f;
        }
        //ALOGE("MLETEST,  defaultMiuiGap %f, sysSpaceGap%f , factor %f, PaintSize%f", defaultMiuiGap, systemSpaceGap, mGapFactor, mPaint.size);
    }
}

std::vector<uint16_t> utf8ToUtf16(const std::string& text) {
    std::vector<uint16_t> result;
    int32_t i = 0;
    const int32_t textLength = static_cast<int32_t>(text.size());
    uint32_t c = 0;
    while (i < textLength) {
        U8_NEXT(text.c_str(), i, textLength, c);
        if (U16_LENGTH(c) == 1) {
            result.push_back(c);
        } else {
            result.push_back(U16_LEAD(c));
            result.push_back(U16_TRAIL(c));
        }
    }
    return result;
}

} // namespace minikin
