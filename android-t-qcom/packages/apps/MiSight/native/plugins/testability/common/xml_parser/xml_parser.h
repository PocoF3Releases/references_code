/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight xml parser head file
 *
 */
#ifndef MISIGHT_PLUGINS_TESTABIILITY_XML_PARSER_UTIL_H
#define MISIGHT_PLUGINS_TESTABIILITY_XML_PARSER_UTIL_H

#include <map>
#include <string>

#include <utils/RefBase.h>
#include <utils/StrongPointer.h>


namespace android {
namespace MiSight {
class XmlParser : public RefBase {
public:
    explicit XmlParser(const std::string& fileName): isFinish_(false), isStart_(false), fileName_(fileName) {}
    virtual ~XmlParser() {}
    int ParserXmlFile(const std::string& filePath);
    virtual bool IsMatch(const std::string& localName) = 0;
    virtual void OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes) = 0;
    virtual void OnEndElement(const std::string& localName) = 0;
    void SetFinish();
    bool IsFinish();

    bool IsStart();
    void SetStart();
    const std::string GetXmlFileName()
    {
        return fileName_;
    }

private:
    bool isFinish_;
    bool isStart_;
    std::string fileName_;
};
} // namespace MiSight
} // namespace android
#endif

