/*
 * Copyright (C) Xiaomi Inc.
 * Description:  misight xml parser head file
 *
 */

#include "xml_parser.h"

#include <mutex>
#include <stdio.h>
#include <string.h>

#include <libxml/SAX.h>
#include <libxml/parser.h>

#include "log.h"


namespace android {
namespace MiSight {
constexpr uint32_t CHUNK_SIZE = 512;

void OnStartElementNs(
    void *ctx,
    const xmlChar *localname,
    const xmlChar *prefix,
    const xmlChar *URI,
    int nb_namespaces,
    const xmlChar **namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar **attributes)
{
    std::string local = std::string(reinterpret_cast<const char*>(localname));
    XmlParser *parser = reinterpret_cast<XmlParser *>(ctx);
    if (parser == nullptr) {
        MISIGHT_LOGD("local=%s parser is null. pre=%s,uri=%s,nb=%d,ns=%p,nb_de=%d",
            local.c_str(), prefix, URI, nb_namespaces, namespaces, nb_defaulted);
        return;
    }

    if (!parser->IsStart() && !parser->IsMatch(local)) {
        return;
    }

    std::map<std::string, std::string> attrMap;
    const int attrDefLen = 5;
    const int attrNameId = 0; // name
    const int attrValStart = 3; // value start
    const int attrValEnd = 4;   // value end
    if (nb_attributes > 0) {
        for (int i = 0; i < nb_attributes; i++) {
            int loop = i * attrDefLen;
            std::string name(reinterpret_cast<const char*>(attributes[loop + attrNameId]));
            std::string valStart(reinterpret_cast<const char*>(attributes[loop + attrValStart]));
            std::string valEnd(reinterpret_cast<const char*>(attributes[loop + attrValEnd]));
            std::string value = valStart.substr(0, valStart.size() - valEnd.size());
            attrMap[name] = value;
        }
    }
    parser->OnStartElement(local, attrMap);
}

void OnEndElementNs(
    void* ctx,
    const xmlChar* localname,
    const xmlChar* prefix,
    const xmlChar* URI)
{

    XmlParser *parser = reinterpret_cast<XmlParser *>(ctx);
    if (parser == nullptr) {
        MISIGHT_LOGI("parser is null. pre=%s,uri=%s", prefix, URI);
        return;
    }
    if (!parser->IsStart()) {
        return;
    }
    std::string local = std::string(reinterpret_cast<const char*>(localname));
    parser->OnEndElement(local);
}

#if 0
void OnCharacters(void *ctx, const xmlChar *ch, int len)
{

}
#endif
std::mutex parserMutex_;

xmlSAXHandler InitSaxHeader()
{
    xmlSAXHandler SAXHander;

    memset(&SAXHander, 0, sizeof(xmlSAXHandler));

    SAXHander.initialized = XML_SAX2_MAGIC;
    SAXHander.startElementNs = OnStartElementNs;
    SAXHander.endElementNs = OnEndElementNs;
    SAXHander.characters = nullptr; //OnCharacters;
    return SAXHander;
}

int ReadXmlFile(const std::string& filePath, sp<XmlParser> xmlParser)
{
    MISIGHT_LOGD("ReadXmlFile, %s, ", filePath.c_str());
    FILE *fp = fopen(filePath.c_str(), "r");
    if (!fp) {
        MISIGHT_LOGE("not open file %s", filePath.c_str());
        return -1;
    }

    char chars[CHUNK_SIZE];

    // read 4 bytes, to check xml file format
    const int xmlHeadLen = 4;
    int res = fread(chars, 1, xmlHeadLen, fp);
    if (res <= 0) {
        MISIGHT_LOGE("file formate err, %s:", filePath.c_str());
        fclose(fp);
        return -1;
    }

    // Init xml context
    xmlSAXHandler SAXHander = InitSaxHeader();
    xmlInitParser();


    xmlParserCtxtPtr ctxt = xmlCreatePushParserCtxt(&SAXHander, xmlParser.get(), chars, res, NULL);

    // parser xml file
    int ret = 0;
    while ((res = fread(chars, 1, sizeof(chars), fp)) > 0) {
        if (xmlParseChunk(ctxt, chars, res, 0)) {
            xmlParserError(ctxt, "xmlParseChunk");
            MISIGHT_LOGE("xml parse chunk failed: %s", chars);
            ret = -1;
            goto EXIT;
        }
        if (xmlParser->IsFinish()) {
            goto EXIT;
        }
    }

    xmlParseChunk(ctxt, chars, 0, 1);

EXIT:
    xmlFreeParserCtxt(ctxt);
    xmlCleanupParser();
    fclose(fp);
    return ret;
}

int XmlParser::ParserXmlFile(const std::string& filePath)
{
    isStart_ = false;
    isFinish_ = false;
    std::lock_guard<std::mutex> lock(parserMutex_);
    return ReadXmlFile(filePath, this);
}

bool XmlParser::IsFinish()
{
    return isFinish_;
}

void XmlParser::SetFinish()
{
    isStart_ = false;
    isFinish_ = true;
}

bool XmlParser::IsStart()
{
    return isStart_;

}

void XmlParser::SetStart()
{
    isStart_ = true;
    isFinish_ = false;
}

} // namespace MiSight
} // namespace android

