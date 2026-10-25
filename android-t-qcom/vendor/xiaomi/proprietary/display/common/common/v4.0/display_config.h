/*
 * Copyright (c) 2014 Qualcomm Technologies, Inc.  All Rights Reserved.
 * Qualcomm Technologies Proprietary and Confidential.
 */

#ifndef _DISPLAY_CONFIG_H_
#define _DISPLAY_CONFIG_H_

// STATIC LIST OF MODES
enum DISPLAY_PP_MODE {
    DISPLAY_PP_MODE_mode0 = 0,          // RenderIntent 0, Google standard mode
    DISPLAY_PP_MODE_mode1,              // RenderIntent 1, Google enhanced mode
    DISPLAY_PP_MODE_mode2,              // RenderIntent 2, Google vivid mode
    DISPLAY_PP_MODE_mode3,              // RenderIntent 3, Google warm mode
    DISPLAY_PP_MODE_mode256 = 256,      // RenderIntent 256, mi native mode
    DISPLAY_PP_MODE_mode257,            // RenderIntent 257, mi wcg mode
    DISPLAY_PP_MODE_mode258,            // RenderIntent 258, mi auto(P3) mode
    DISPLAY_PP_MODE_mode259,            // RenderIntent 259, mi video1 mode
    DISPLAY_PP_MODE_mode260,            // RenderIntent 260, mi video2 mode
    DISPLAY_PP_MODE_mode261,            // RenderIntent 261, mi video3 mode
    DISPLAY_PP_MODE_mode262,            // RenderIntent 262, mi video4 mode
    DISPLAY_PP_MODE_mode263,            // RenderIntent 263, mi game1 mode
    DISPLAY_PP_MODE_mode264,            // RenderIntent 264, mi game2 mode
    DISPLAY_PP_MODE_mode265,            // RenderIntent 265, mi game3 mode
    DISPLAY_PP_MODE_mode266,            // RenderIntent 266, mi expert native mode
    DISPLAY_PP_MODE_mode267,            // RenderIntent 267, mi expert srgb mode
    DISPLAY_PP_MODE_mode268,            // RenderIntent 268, mi expert p3 mode
    DISPLAY_PP_MODE_mode269,            // RenderIntent 269, mi expert wcg mode
    DISPLAY_PP_MODE_mode270,            // RenderIntent 270, mi transition mode from expert modes to other modes.
    DISPLAY_PP_MODE_mode271,            // RenderIntent 271, mi auto(P3) mode for FOD
    DISPLAY_PP_MODE_mode272,            // RenderIntent 272, mi expert p3 mode for FOD
    DISPLAY_PP_MODE_mode273,            // RenderIntent 273, mi Dolby vision mode with flat mode off
    DISPLAY_PP_MODE_mode274,            // RenderIntent 274, mi Dolby vision mode with flat mode on
    DISPLAY_PP_MODE_mode281 = 281,
    DISPLAY_PP_MODE_mode282 = 282,
    DISPLAY_PP_MODE_mode283 = 283,
    DISPLAY_PP_MODE_mode284 = 284,
    DISPLAY_PP_MODE_mode285 = 285,
    DISPLAY_PP_MODE_mode286 = 286,
    DISPLAY_PP_MODE_mode287 = 287,
    DISPLAY_PP_MODE_mode288 = 288,
    DISPLAY_PP_MODE_MAX_SETTINGS
};
#ifndef MTK_PLATFORM
namespace qdisplaymode {

enum DISPLAY_PP_MODE_OLD {
    DISPLAY_PP_MODE_BYPASS = 0,
    DISPLAY_PP_MODE_OLD_mode0,//warm
    DISPLAY_PP_MODE_OLD_mode1,//cold
    DISPLAY_PP_MODE_OLD_mode2,//sRGB
    DISPLAY_PP_MODE_OLD_mode3,// eye-protection mode 0
    DISPLAY_PP_MODE_mode4,// eye-protection mode 1
    DISPLAY_PP_MODE_mode5,// eye-protection mode 2
    DISPLAY_PP_MODE_mode6,// eye-protection mode 3
    DISPLAY_PP_MODE_mode7,// eye-protection mode 4
    DISPLAY_PP_MODE_mode8,// eye-protection mode 5
    DISPLAY_PP_MODE_mode9,// eye-protection mode 6
    DISPLAY_PP_MODE_mode10,// eye-protection mode 7
    DISPLAY_PP_MODE_mode11,// eye-protection mode 8
    DISPLAY_PP_MODE_mode12,// eye-protection mode 9
    DISPLAY_PP_MODE_mode13,// eye-protection mode 10
    DISPLAY_PP_MODE_mode14,// eye-protection mode 11
    DISPLAY_PP_MODE_mode15,// eye-protection mode 12
    DISPLAY_PP_MODE_mode16,// eye-protection mode 13
    DISPLAY_PP_MODE_mode17,// eye-protection mode 14
    DISPLAY_PP_MODE_mode18,// eye-protection mode 15
    DISPLAY_PP_MODE_mode19,// eye-protection mode 16
    DISPLAY_PP_MODE_mode20,// eye-protection mode 17
    DISPLAY_PP_MODE_mode21,// eye-protection mode 18
    DISPLAY_PP_MODE_mode22,// eye-protection mode 19
    DISPLAY_PP_MODE_mode23,// eye-protection mode 20
    DISPLAY_PP_MODE_mode24,// eye-protection mode 21
    DISPLAY_PP_MODE_mode25,// eye-protection mode 22
    DISPLAY_PP_MODE_mode26,// eye-protection mode 23
    DISPLAY_PP_MODE_mode27,// eye-protection mode 24
    DISPLAY_PP_MODE_mode28,// eye-protection mode 25
    DISPLAY_PP_MODE_mode29,// eye-protection mode 26
    DISPLAY_PP_MODE_mode30,// eye-protection mode 27
    DISPLAY_PP_MODE_mode31,// eye-protection mode 28
    DISPLAY_PP_MODE_mode32,// eye-protection mode 29
    DISPLAY_PP_MODE_mode33,// eye-protection mode 30
    DISPLAY_PP_MODE_mode34,// eye-protection mode 31
    DISPLAY_PP_MODE_mode35 = 36,// DCI-P3
    DISPLAY_PP_MODE_mode36,// keep whitepoint srgb
    DISPLAY_PP_MODE_mode37,// color banlance adjust
    DISPLAY_PP_MODE_mode38,// hdr
    DISPLAY_PP_MODE_mode39,// for oled automatic contrast with sRGB tag
    DISPLAY_PP_MODE_mode40,// game mode 1 with sRGB tag
    DISPLAY_PP_MODE_mode41,// game mode 2 with sRGB tag
    DISPLAY_PP_MODE_mode42,// game mode 3 with sRGB tag
    DISPLAY_PP_MODE_mode50 = 50,// for oled enhance contrast with P3 tag
    DISPLAY_PP_MODE_mode51,// for oled automatic contrast with P3 tag
    DISPLAY_PP_MODE_mode52,// game mode 1 with P3 tag
    DISPLAY_PP_MODE_mode53,// game mode 2 with P3 tag
    DISPLAY_PP_MODE_mode54,// game mode 3 with P3 tag
    DISPLAY_PP_MODE_mode55,// video mode 1 with sRGB tag
    DISPLAY_PP_MODE_mode56,// video mode 2 with sRGB tag
    DISPLAY_PP_MODE_mode57,// video mode 3 with sRGB tag
    DISPLAY_PP_MODE_mode58,// video mode 1 with P3 tag
    DISPLAY_PP_MODE_mode59,// video mode 2 with P3 tag
    DISPLAY_PP_MODE_mode60,// video mode 3 with P3 tag
    DISPLAY_PP_MODE_END_SETTINGS
};

bool DisplayConfigPPMode(DISPLAY_PP_MODE mode);

}
#endif
#endif