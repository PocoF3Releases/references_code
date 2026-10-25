#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <string.h>

#include "res_hook.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "res_hook"

#define MIUI_MCC_MNC_START    9000
#define MIUI_MCC_MNC_END      10000
#define MIUI_DEFAULT_MCC      9999
#define MIUI_COMMON_MCC_I18N  9998
#define MIUI_COMMON_MCC_CN    9997
#define MIUI_COMMON_MNC       9999

static bool isInternationalBuild() {
    static bool checked = false;
    static bool internationalBuild = false;
    if (!checked) {
        checked = true;
        char value[PROPERTY_VALUE_MAX] = {'\0'};
        property_get("ro.product.mod_device", value, "");
        const char *suffix = "_global";
        const char *pos = strstr(value, suffix);
        internationalBuild = (value + strlen(value)) == (pos + strlen(suffix));
    }
    return internationalBuild;
}

static uint16_t getMiuiMcc(){
    char value[PROPERTY_VALUE_MAX] = {'\0'};
    property_get("ro.miui.mcc", value, "0");
    const uint16_t mmcc = atoi(value);
    return mmcc >= MIUI_MCC_MNC_START ? mmcc : MIUI_DEFAULT_MCC;
}

static uint16_t getMiuiMnc() {
    char value[PROPERTY_VALUE_MAX] = {'\0'};
    property_get("ro.miui.mnc", value, "0");
    const uint16_t mmnc = atoi(value);
    return mmnc >= MIUI_MCC_MNC_START ? mmnc : MIUI_COMMON_MNC;
}

static uint16_t getMiuiCommonMcc() {
    return isInternationalBuild() ? MIUI_COMMON_MCC_I18N : MIUI_COMMON_MCC_CN;
}

static uint16_t getMiuiCommonMnc() {
    return MIUI_COMMON_MNC;
}

int matchMiuiMccMnc(uint16_t mcc, uint16_t mnc) {
    if (mcc >= MIUI_MCC_MNC_END || mnc >= MIUI_MCC_MNC_END) {
        return MATCH_CONTINUE;
    }
    if (mcc >= MIUI_MCC_MNC_START && mnc >= MIUI_MCC_MNC_START) {
        const uint16_t miuiMcc = getMiuiMcc();
        const uint16_t miuiCommonMcc = getMiuiCommonMcc();
        if (mcc != miuiMcc && mcc != miuiCommonMcc)  return MATCH_UNMATCH;
        if (mcc == miuiMcc && mnc != getMiuiMnc())  return MATCH_UNMATCH;
        if (mcc == miuiCommonMcc && mnc != getMiuiCommonMnc())  return MATCH_UNMATCH;
        return MATCH_MATCH;
    } else if (mcc >= MIUI_MCC_MNC_START || mnc >= MIUI_MCC_MNC_START) {
        return MATCH_UNMATCH;
    } else {
        return MATCH_CONTINUE;
    }
}

int isMiuiMccMncBetterThan(uint16_t mcc, uint16_t mnc, uint16_t omcc, uint16_t omnc) {
    if (mcc >= MIUI_MCC_MNC_START || mnc >= MIUI_MCC_MNC_START
        || omcc >= MIUI_MCC_MNC_START || omnc >= MIUI_MCC_MNC_START) {

#ifdef DEBUG_LOG
        ALOGI("mcc:%d mnc:%d omcc:%d omnc:%d mmcc:%d mmnc:%d mcmcc:%d mcmnc:%d",
            mcc, mnc, omcc, omnc, getMiuiMcc(), getMiuiMnc(),
            getMiuiCommonMcc(), getMiuiCommonMnc());
#endif

        if (mcc != omcc) {
            const uint16_t miuiMcc = getMiuiMcc();
            if (mcc == miuiMcc)  return COMPARE_BETTER;
            if (omcc == miuiMcc)  return COMPARE_WORSE;

            const uint16_t miuiCommonMcc = getMiuiCommonMcc();
            if (mcc == miuiCommonMcc)  return COMPARE_BETTER;
            if (omcc == miuiCommonMcc)  return COMPARE_WORSE;
        }

        if (mnc != omnc) {
            const uint16_t miuiMnc = getMiuiMnc();
            if (mnc == miuiMnc)  return COMPARE_BETTER;
            if (omnc == miuiMnc)  return COMPARE_WORSE;

            const uint16_t miuiCommonMnc = getMiuiCommonMnc();
            if (mnc == miuiCommonMnc)  return COMPARE_BETTER;
            if (omnc == miuiCommonMnc)  return COMPARE_WORSE;
        }
    }
    return COMPARE_CONTINUE;
}

