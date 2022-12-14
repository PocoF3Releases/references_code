//  Copyright (C) 2022 Xiaomi.inc
//  Author:gaolei6@xiaomi.com
//  Create:2022-10-11

#ifndef MINIKIN_MLE_OPTIMIZER_H
#define MINIKIN_MLE_OPTIMIZER_H

#include <vector>
#include <mutex>
//below for PieceEndCahce
#include <utils/LruCache.h>
#include "minikin/Hasher.h"
#include "minikin/U16StringPiece.h"
#include <log/log.h>
#include <minikin/LayoutCore.h>
#include <cutils/properties.h>
#include <minikin/MinikinPaint.h>
// PerfTest
#include<ctime>
#include <chrono>
using namespace std::chrono;

#ifdef _WIN32
#include <io.h>
#endif

 namespace minikin {

const uint16_t  DEFAULT_PIECE_END_GLYPH_ID = 0; // Keep out of our match range

class MLEOptimizerMatch{
    public:
    explicit MLEOptimizerMatch() = default;
    ~MLEOptimizerMatch() = default;
    static uint16_t ZH_HANS_MIN;
    static uint16_t ZH_HANS_MAX;

    static uint16_t EN_A;
    static uint16_t EN_Z;
    static uint16_t EN_a;
    static uint16_t EN_z;

    static uint16_t NUM_0;
    static uint16_t NUM_9;

    static uint16_t SPACE;
};

class MLEOptimizer {
public:
    ~MLEOptimizer(){}
    bool isBoPoMoFoText(uint16_t codePoint);

    bool isEnglish(uint16_t codePoint);

    bool isNumber(uint16_t codePoint);

    std::pair<bool, bool> isMatch(uint16_t codePointLeft /*left glyphId*/, uint16_t codePointRight /* right glypyId */);

    static MLEOptimizer& getInstance() {
        static MLEOptimizer singleton;
        return singleton;
    }
    //
    void resetLastPieceTail(const U16StringPiece& text, uint16_t codePoint);
    uint16_t getLastPieceTail(const U16StringPiece& text);
    uint16_t getPieceTailAt(const U16StringPiece& text, size_t pos); // For Range  measure

    //
    void notifyMeasureLineBegin(const U16StringPiece& text, size_t rangeStart /*default is 0*/) {
        if (rangeStart == 0) { // full measure
            //ALOGE("MLETEST, reset pieceEnd to 0, text len:%u", text.size());
            resetLastPieceTail(text, DEFAULT_PIECE_END_GLYPH_ID); // avoid line head  to opt
        } else {
            resetLastPieceTail(text, getPieceTailAt(text, rangeStart -1));
        }

    }

    void initSpace(bool isRtl, const MinikinPaint& paint, StartHyphenEdit startHyphen, EndHyphenEdit endHyphen);
    //
    float getMiuiGap(bool isHansNum) {
        //ALOGE("MLETEST,  defaultMiuiGap : %f,  systemSpaceGap:%f", defaultMiuiGap, systemSpaceGap);
        return isHansNum ? defaultMiuiGap * mHansNumFactor : defaultMiuiGap;
    }
    uint32_t getSpaceCode() {
        return mMLEOptimizerMatch.SPACE;
    }
    bool shouldInitSpaceGap() {
        return systemSpaceGap == 0.0f;
    }

private:
    MLEOptimizer(){
        int64_t scaler = property_get_int64("persist.sys.mle_miui_gap", 62);
        if (scaler > 200 || scaler < 0) {
             scaler = 100; // reset to default value
        }
        mGapFactor = (float)(scaler / 100.0f);
        //
         scaler = property_get_int64("persist.sys.mle_miui_gap_extra", 80);
         if (scaler >100 || scaler < 0) {
             scaler = 80;
         }
         mHansNumFactor = (float)(scaler / 100.0f);
    }
    float systemSpaceGap = 0.0f;
    float mGapFactor = 1.0f; // 0~2
    float mHansNumFactor = 0.8f; //
    float defaultMiuiGap = 10.0f; // 0~50
    MLEOptimizerMatch mMLEOptimizerMatch;
    MinikinPaint mPaint = MinikinPaint(std::shared_ptr<FontCollection>(nullptr));
    bool mIsRtl = true;
    uint16_t  viewPieceEndGlyph_id =   DEFAULT_PIECE_END_GLYPH_ID;
};
 }  // namespace minikin
#endif  // MINIKIN_MLE_OPTIMIZER_H