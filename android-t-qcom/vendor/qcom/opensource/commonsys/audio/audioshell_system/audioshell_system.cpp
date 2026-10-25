#define LOG_TAG "audioshell_system"
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <media/AudioSystem.h>
#include <cutils/properties.h>

#include <log/log.h>


using namespace android;

static const char *ce_regions[] = {
    "BH", /*Bahrain*/
    /*Macau NOT YET*/
    "HK", /*Hong Kong*/
    "IL", /*Israel*/
    "JO", /*Jordan*/
    "KW", /*Kuwait*/
    "LB", /*Lebanon*/
    "MY", /*Malaysia*/
    "OM", /*Oman*/
    "PK", /*Pakistan*/
    "PH", /*Philippine*/
    "QA", /*Qatar*/
    "YE", /*Yemen*/
    "NP", /*Nepal*/
    "KR", /*South Korea*/
    "BD", /*Bangladesh*/
    "IQ", /*Iraq*/
    "LK", /*Sri Lanka*/
    /*Cambodia NOT YET*/
    "KZ", /*Kazakhstan*/
    /*Mongolia NOT YET*/
    /*Tajikistan NOT YET*/
    "TM", /*Turkmenistan*/
    "UZ", /*Uzbekistan*/
    /*Laos NOT YET*/

    "GH", /*Ghana*/
    "MU", /*Mauritius*/
    "MA", /*Morocco*/
    "MZ", /*Mozambique*/
    "ZA", /*South Africa*/
    "TZ", /*Tanzania*/
    "UG", /*Uganda*/
    "TN", /*Tunisia*/
    "ET", /*Ethiopia*/
    "ZM", /*Zambia*/
    "LY", /*Libya*/
    "DZ", /*Algeria*/
    "AO", /*Angola */
    "BW", /*Botswana*/
    "ZW", /*Zimbabwe*/
    "NA", /*Namibia*/
    "KE", /*Kenya*/

    "GB", /*United Kingdom*/
    "NO", /*Norway*/
    "RO", /*Romania*/
    /*Ireland NOT YET*/
    "HU", /*Hungary*/
    "FI", /*Finland*/
    "GR", /*Greece*/
    "CZ", /*Czech Republic*/
    "SK", /*Slovakia*/
    "NL", /*Netherlands*/
    "IT", /*Italy*/
    "FR", /*France*/
    "ES", /*Spain*/
    "RU", /*Russia*/
    "SE", /*Sweden*/
    "CH", /*Switzerland*/
    /*Austria NOT YET*/
    "BE", /*Belgium*/
    "PT", /*Portugal*/
    "PL", /*Poland*/
    "DK", /*Denmark*/
    "UA", /*Ukraine*/
    "DE" /*Germany*/
};

static bool is_ce_region(const char *name)
{
    unsigned long i;
    unsigned long count;
    bool ret = false;

    count = (unsigned long)(sizeof(ce_regions)/sizeof(ce_regions[0]));
    for (i = 0; i < count; i++) {
        if (strcmp(ce_regions[i], name) == 0) {
            ret = true;
            break;
        }
    }
    return ret;
}

int main(int argc, char *argv[])
{
    char value[PROPERTY_VALUE_MAX];
    bool is_ce = false;

    ALOGI("%s: start", __func__);
    property_get("ro.vendor.miui.region", value, "");
    if (strcmp(value, "") == 0) {
        ALOGD("prop not ready!!!");
    } else {
        if (is_ce_region(value)) {
            AudioSystem::setIsCERegion(true);
            ALOGD("switch to ce region");
        } else {
            AudioSystem::setIsCERegion(false);
            ALOGD("switch to non ce region");
        }
    }

    ALOGI("%s: exit", __func__);
    return 0;
}

