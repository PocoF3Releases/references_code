#include "parseXml.h"
#define LOG_TAG "MiXmlParse"
using std::fstream;
namespace android {

enum DISPLAY_LIST {
    DISPLAY_PRIMARY,
    DISPLAY_SECONDARY,
    DISPLAY_MAX,
};

XmlParser MiParseXml::xml_handle[2];
XmlParser MiParseXml::defaultXml[2];
bool MiParseXml::initialized = false;

MiParseXml::MiParseXml()
{
    int ret = 0;
    if (!initialized) {
        ret = init();
        if (!ret)
            initialized = true;
    }
}

int MiParseXml::init()
{
    int ret = 0;
    for (int i = 0; i < DISPLAY_MAX; i++) {
        if (i == DISPLAY_SECONDARY && access(SEC_PANEL_INFO_PATH, F_OK))
            break;
        ret = getXmlHandle(i, &xml_handle[i]);
        if (ret) {
            ALOGE("Failed to parse df xml");
        }
        ret = getDefaultXmlHandle(i, &defaultXml[i]);
        if (ret) {
            ALOGE("Failed to parse default df xml");
        }
    }
    return ret;
}

int MiParseXml::getPanelName(const char *file_name,char *panel_name)
{
    char _panel_name[256] = {0};// Panel name
    XmlParser xmlHandle;
    fstream fs;
    fs.open(file_name,fstream::in);
    if (!fs.is_open()){
        ALOGE("Failed to open msm_fb_panel_info node device node\n");
        return -1;
    }
    string line;
    while (std::getline(fs, line)) {
        unsigned int token_count = 0;
        const unsigned int max_count = 10;
        char *tokens[max_count] = {NULL};
        if (!xmlHandle.ParseLine(line.c_str(), "=\n", tokens, max_count, &token_count)) {
            ALOGV("%s\t%s\t%d\n",tokens[0],tokens[1],token_count);
            if(tokens[0] != NULL)
            if (!strncmp(tokens[0], "panel_name", strlen("panel_name"))) {

                snprintf(_panel_name, sizeof(_panel_name), "%s", tokens[1]);
                break;
            }
        }
    }

    if (strlen(_panel_name)) {
        snprintf(&panel_name[0], sizeof(_panel_name), "%s", &_panel_name[0]);
        char *tmp = panel_name;
        while ((tmp = strstr(tmp, " ")) != NULL)
            *tmp = '_';
        if ((tmp = strstr(_panel_name, "\n")) != NULL)
            *tmp = '\0';
    }

    return 0;
}

int MiParseXml::getDefaultXmlHandle(int displayId, XmlParser * handle)
{
    int err = 0;
    string fName = "odm/etc/df_default.xml";
    err = handle->parseXml(fName);
    if (err < 0)
        ALOGE("Failed to parse %s", fName.c_str());
    return err;
}

int MiParseXml::getXmlHandle(int displayId, XmlParser* xml_handle)
{
    int err = 0;
    string file_name = displayId == DISPLAY_PRIMARY ? PRIM_PANEL_INFO_PATH : SEC_PANEL_INFO_PATH;
    char panel_name[255];
    getPanelName(file_name.c_str(),panel_name);
    strcat(panel_name,"_mi.xml");
    string fName;
    fName.append("odm/etc/");
    fName.append(panel_name);

    err = xml_handle->parseXml(fName);
    if (err < 0)
        ALOGE("Failed to parse %s", fName.c_str());
    return err;
}

int MiParseXml::parseXml(int displayId, const string key, char *p, char *tokens[], unsigned int *count)
{
    int result = 0;
    int err = 0;
    const unsigned int max_count = 1024;

    if (!initialized) {
        err = init();
        if (!err)
            initialized = true;
    }
    if (initialized) {
        // get key value from device config.
        result = xml_handle[displayId].getXmlValue(key,p,4096);
        if(result >= 0) {
            if (!xml_handle[displayId].ParseLine(p, " \n", tokens, max_count, count)) {
                return 0;
            } else {
                ALOGV("Failed to parseLine");
            }
        } else {
            ALOGV("Failed to getXml value %s", key.c_str());
        }
        // If no key value in device config, use default config.
        result = defaultXml[displayId].getXmlValue(key,p,4096);
        if(result >= 0) {
            if (!defaultXml[displayId].ParseLine(p, " \n", tokens, max_count, count)) {
                return 0;
            } else {
                ALOGE("Failed to parseLine");
                err -1;
            }
        } else {
            ALOGE("Failed to getXml value %s", key.c_str());
            err -1;
        }
    }
    return err;
}

int MiParseXml::parseXml(int displayId, const string key, char *p, unsigned int len)
{
    int result = 0;
    int err = 0;

    if (!initialized) {
        err = init();
        if (!err)
            initialized = true;
    }
    if (initialized) {
        result = xml_handle[displayId].getXmlValue(key,p,len);
        if (result >= 0) {
            return 0;
        } else {
            ALOGV("Failed to get Xml value %s", key.c_str());
        }
        // If no key value in device config, use default config.
        result = defaultXml[displayId].getXmlValue(key,p,len);
        if (result >= 0) {
            return 0;
        } else {
            ALOGE("Failed to get Xml value %s", key.c_str());
            err = -1;
        }
    }
    return err;
}

}
