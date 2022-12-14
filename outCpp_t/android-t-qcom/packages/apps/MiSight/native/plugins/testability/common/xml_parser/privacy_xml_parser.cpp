/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event privacy xml config file parser implement file
 *
 */

#include "privacy_xml_parser.h"


namespace android {
namespace MiSight {

const std::string PRIVACY_XML = "privacy.xml";
// privacy xml
const std::string PRIVACY_REGION_LOCAL_NAME = "Region";
const std::string PRIVACY_REGION_ATTR_NAME = "name";
const std::string PRIVACY_REGION_ATTR_LOCALE = "locale";
const std::string PRIVACY_REGION_ATTR_CODE = "code";
const std::string PRIVACY_REGION_ATTR_ISOLANG = "isoLang";
const std::string PRIVACY_REGION_ATTR_ISOCOUNTRY = "isoCountry";
const std::string PRIVACY_REGION_ATTR_MCC = "mcc";
const std::string PRIVACY_REGION_ATTR_ALLOW_PRV = "allowPrv";
const std::string PRIVACY_REGION_ATTR_ALLOW_DEVPRV = "allowDevPrv";
const std::string PRIVACY_REGION_ATTR_ALLOW_EVENT = "allowEvent";
const std::string PRIVACY_REGION_ATTR_ALLOW_DEVEVENT = "allowDevEvent";

PrivacyXmlParser::PrivacyXmlParser()
     :XmlParser(PRIVACY_XML), findRegion_(false), version_(""), expName_(""), expValue_(""),
      workDir_(""), faultLevel_(EventUtil::FaultLevel::GENERAL), privacyLevel_(EventUtil::PrivacyLevel::L3)
{}

PrivacyXmlParser::~PrivacyXmlParser()
{
}

sp<PrivacyXmlParser> PrivacyXmlParser::getInstance()
{
    static sp<PrivacyXmlParser> intance_ = new PrivacyXmlParser;
    return intance_;
}

bool PrivacyXmlParser::IsMatch(const std::string& localName)
{
    if (localName == PRIVACY_REGION_LOCAL_NAME) {
        return true;
    }
    return false;

}

void PrivacyXmlParser::OnStartElement(const std::string& localName, std::map<std::string, std::string> attributes)
{
     if (localName != PRIVACY_REGION_LOCAL_NAME) {
         return;
     }
     if (expName_ == PRIVACY_REGION_ATTR_LOCALE && expValue_ == attributes[PRIVACY_REGION_ATTR_LOCALE]) {
        SetStart();
        findRegion_ = true;
        if (DevInfo::IsInnerVersion(version_)) {
            faultLevel_ = EventUtil::GetFaultLevel(attributes[PRIVACY_REGION_ATTR_ALLOW_DEVEVENT]);
            privacyLevel_ = EventUtil::GetPrivacyLevel(attributes[PRIVACY_REGION_ATTR_ALLOW_DEVPRV]);
            MISIGHT_LOGD("fault=%s,%d dev privacy=%s %d",attributes[PRIVACY_REGION_ATTR_ALLOW_DEVEVENT].c_str(),
                faultLevel_, attributes[PRIVACY_REGION_ATTR_ALLOW_DEVPRV].c_str(),privacyLevel_);
        } else {
            MISIGHT_LOGD("fault=%s,privacy=%s",attributes[PRIVACY_REGION_ATTR_ALLOW_EVENT].c_str(),
                attributes[PRIVACY_REGION_ATTR_ALLOW_PRV].c_str());
            faultLevel_ = EventUtil::GetFaultLevel(attributes[PRIVACY_REGION_ATTR_ALLOW_EVENT]);
            privacyLevel_ = EventUtil::GetPrivacyLevel(attributes[PRIVACY_REGION_ATTR_ALLOW_PRV]);
        }
     }
}

void PrivacyXmlParser::OnEndElement(const std::string& localName)
{
    SetFinish();
}

bool PrivacyXmlParser::GetPolicyByLocale(const std::string& workDir, const std::string& version,
    const std::string& locale, EventUtil::FaultLevel &faultLevel, EventUtil::PrivacyLevel &privacyLevel)
{
    if (locale == "") {
        MISIGHT_LOGI("locale is null");
        return false;
    }
    std::lock_guard<std::mutex> lock(parserMutex_);
    if (workDir_ == "" || workDir_ != workDir) {
        findRegion_ = false;
        workDir_ = workDir;
    }
    if (findRegion_) {
        if (locale != expValue_ || version_ != version) {
            findRegion_ = false;
        }
    }
    if (!findRegion_) {
        expName_ = PRIVACY_REGION_ATTR_LOCALE;
        expValue_ = locale;
        version_ = version;

        if (ParserXmlFile(workDir + PRIVACY_XML) != 0) {
            MISIGHT_LOGI("parse xml file failed");
            return false;
        }
    }
    if (!findRegion_) {
        MISIGHT_LOGI("can't find %s privacy policy", locale.c_str());
        return false;
    }
    faultLevel = faultLevel_;
    privacyLevel = privacyLevel_;
    return true;
}
}
}


