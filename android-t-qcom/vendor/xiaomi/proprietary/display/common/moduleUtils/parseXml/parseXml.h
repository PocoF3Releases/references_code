#ifndef _MI_PARSE_XML_H_
#define _MI_PARSE_XML_H_
#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <fstream>
#include <cutils/properties.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/StrongPointer.h>
#include <utils/RefBase.h>
#include "XmlParser.h"

namespace android {

#define PRIM_PANEL_INFO_PATH "/sys/class/mi_display/disp-DSI-0/panel_info"
#define SEC_PANEL_INFO_PATH "/sys/class/mi_display/disp-DSI-1/panel_info"
class MiParseXml : public RefBase
{
public:
    MiParseXml();
    virtual ~MiParseXml() {}
    int init();
    int getXmlHandle(int displayId, XmlParser* xml_handle);
    int getDefaultXmlHandle(int displayId, XmlParser * handle);
    int getPanelName(const char *file_name,char *panel_name);
    int parseXml(int displayId, const string key, char *p, char *tokens[], unsigned int *count);
    int parseXml(int displayId, const string key, char *p, unsigned int len);
    static XmlParser xml_handle[2];
    static XmlParser defaultXml[2];
private:
    static bool initialized;
};
}
#endif
