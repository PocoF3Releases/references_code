/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability: event feature log collector plugin implements file
 *
 */

#include "event_log_processor.h"

#include <regex>
#include <string>

#include <fcntl.h>
//#include <openssl/base64.h>
//#include <openssl/md5.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "cmd_processor.h"
#include "dev_info.h"
#include "event_util.h"
#include "file_util.h"
#include "plugin_factory.h"
#include "privacy_xml_parser.h"
#include "public_util.h"
#include "event_quota_xml_parser.h"
#include "string_util.h"
#include "zip_pack.h"

namespace android {
namespace MiSight {
REGISTER_PLUGIN(EventLogProcessor);

const std::string UPLOADING_NAME = "uploading";


std::string scanDirPath_ = "";
int SortFileByMkTime(const struct dirent **srcFile, const struct dirent **dstFile)
{
    struct stat bufSrc;
    struct stat bufDst;
    int ret = 0;

    std::string fullPath = scanDirPath_ + "/" + std::string((*srcFile)->d_name);
    ret = stat(fullPath.c_str(), &bufSrc);

    if (ret < 0) {
        MISIGHT_LOGW("file stat1 %s failed", fullPath.c_str());
        return ret;
    }

    fullPath = scanDirPath_ + "/" + std::string((*dstFile)->d_name);
    ret = stat(fullPath.c_str(), &bufDst);
    if (ret < 0) {
        MISIGHT_LOGW("file stat2 %s failed", fullPath.c_str());
        return ret;
    }

    return bufSrc.st_mtime - bufDst.st_mtime;
 }

std::string scanDirPattern_ = "";
int scanDirMaxCnt_ = -1;
int FilterFileByPattern(const struct dirent *ent)
{
    if (ent->d_name[0] == '.') {
        return 0;
    }

    std::smatch result;
    std::regex pattern(scanDirPattern_);
    if (std::regex_match(std::string(ent->d_name), pattern)) {
        if (scanDirMaxCnt_ == -1 || scanDirMaxCnt_ > 0) {
            if (scanDirMaxCnt_ > 0) {
                scanDirMaxCnt_ --;
            }
            return 1;
        }
    }
    return 0;
}

std::vector<std::string> EventLogProcessor::GetFileInDirByPattern(const std::string& dirPath,
    const std::string& pattern, bool timeSort, bool addDir, int maxCnt)
{
    std::vector<std::string> files;
    files.clear();
    scanDirPath_ = dirPath;
    scanDirMaxCnt_ = maxCnt;

    struct dirent **namelist;
    int num;
    if (pattern != "") {
        scanDirPattern_ = pattern;
        if (timeSort) {
            num = scandir(dirPath.c_str(), &namelist, FilterFileByPattern, SortFileByMkTime);
        } else {
            num = scandir(dirPath.c_str(), &namelist, FilterFileByPattern, nullptr);
        }
    } else {
        if (timeSort) {
            num = scandir(dirPath.c_str(), &namelist, nullptr, SortFileByMkTime);
        } else {
            num = scandir(dirPath.c_str(), &namelist, nullptr, nullptr);
        }
    }
    if (num <= 0) {
        MISIGHT_LOGD("scan dir %s %d", dirPath.c_str(), num);
        return files;
    }
    while (num--) {
        std::string fileName = std::string(namelist[num]->d_name);
        if (addDir) {
            fileName = dirPath + "/" + fileName;
        }
        files.push_back(fileName);
        free(namelist[num]);
   }
   free(namelist);
   return files;
}

EventLogProcessor::EventLogProcessor()
    : miuiVersion_("")
{}

void EventLogProcessor::OnLoad()
{
    // set plugin info
    SetVersion("EventLogProcessor-1.0");
    SetName("EventLogProcessor");
    auto context = GetPluginContext();
    miuiVersion_ = DevInfo::GetMiuiVersion(context);

    EventRange range(EVENT_SERVICE_CONFIG_UPDATE_ID);
    AddListenerInfo(Event::MessageType::PLUGIN_MAINTENANCE, range);
    context->RegisterUnorderedEventListener(this);

    InitRecordLogCnt();
}

void EventLogProcessor::GetLogFiles(const std::string& eventDir, const std::string& dirName,
    std::vector<std::string>& zipFiles)
{
    std::string zipPattern = "EventId_\\S+_\\d{14}_" + dirName + "\\d{6}.zip";
    zipFiles = GetFileInDirByPattern(eventDir, zipPattern, true, true);
    std::vector<std::string> zipUeFiles = GetFileInDirByPattern(eventDir + "/ue_off/", zipPattern, true, true);
    if (zipFiles.size() == 0 && zipUeFiles.size() == 0) {
        return;
    }

    int ueId = zipUeFiles.size() -1;
    int id  = zipFiles.size() -1;

    while(ueId >= 0 && id >= 0) {
        if (FileUtil::IsFileLatest(zipUeFiles[ueId], zipFiles[id])) {
             id--;
        } else {
            zipFiles.insert(zipFiles.begin() + id + 1, zipUeFiles[ueId]);
            ueId--;
        }
    }
    if (ueId >= 0) {
        zipFiles.insert(zipFiles.begin(), zipUeFiles.begin(), zipUeFiles.begin() + ueId + 1);
    }
    return;
}

void EventLogProcessor::InitRecordLogCnt()
{
    auto context = GetPluginContext();
    std::string workDir = context->GetWorkDir();

    std::vector<std::string> eventDirs = GetFileInDirByPattern(workDir, "9\\d{2}", false, false);
    if (eventDirs.size() == 0) {
        MISIGHT_LOGD("find 9xx folder dir=%s size is 0", workDir.c_str());
        return;
    }

    sp<EventQuotaXmlParser> xmlParser = EventQuotaXmlParser::getInstance();
    std::string cfgDir = context->GetConfigDir(xmlParser->GetXmlFileName());

    // init event log cnt in each folder
    for (auto& dirName : eventDirs) {
        std::string eventDir = workDir + dirName;

        std::vector<std::string> zipFiles;
        GetLogFiles(eventDir, dirName, zipFiles);

        for (auto& oneFile : zipFiles) {
            int eventId = 0;
            int ret = sscanf(oneFile.substr(oneFile.size() - 13).c_str(), "%9d.zip", &eventId);// 13:901001001.zip
            if (ret != 1 || eventId == 0) {
                continue;
            }

            bool quota = xmlParser->CheckLogCntLimitAfterIncr(cfgDir, miuiVersion_, eventId, 1);
            if (quota) {
                FileUtil::DeleteFile(oneFile);
                xmlParser->CheckLogCntLimitAfterIncr(cfgDir, miuiVersion_, eventId, -1);
            }
        }
        zipFiles.clear();
    }
}

void EventLogProcessor::OnUnload()
{

}

EVENT_RET EventLogProcessor::OnEvent(sp<Event> &event)
{
    MISIGHT_LOGD("EventLogProcessor event=%u,%d", event->eventId_, event->messageType_);
    if (event->messageType_ == Event::MessageType::PLUGIN_MAINTENANCE) {
        if (event->eventId_ == EVENT_SERVICE_UPLOAD_ID) {
            int ret = -1;
            if (CanUpload()) {
                MISIGHT_LOGD("start log pack");
                ProcessUploadingLog(event);
                ProcessUploadingEvent(event);
                ret = 0;
            }
            MISIGHT_LOGD("log pack %d", ret);
            event->SetValue(EVENT_SERVICE_UPLOAD_RET, ret);
        } else if (event->eventId_ == EVENT_SERVICE_UE_UPDATE_ID || event->eventId_ == EVENT_SERVICE_CONFIG_UPDATE_ID) {
            auto context = GetPluginContext();
            int onSwitch = event->GetIntValue(EVENT_SERVICE_UE_UPDATE);
            if (event->eventId_ == EVENT_SERVICE_CONFIG_UPDATE_ID) {
                onSwitch = DevInfo::UE_SWITCH_ON;
                if (DevInfo::GetUploadSwitch(context) == "off") {
                    onSwitch = DevInfo::UE_SWITCH_OFF;
                }
            }
            if (onSwitch != DevInfo::UE_SWITCH_OFF) {
                if (IsNotExpired()) {
                    DevInfo::SetUploadSwitch(context, DevInfo::UE_SWITCH_ON);
                } else {
                    DevInfo::SetUploadSwitch(context, DevInfo::UE_SWITCH_ONCTRL);
                }
            } else {
                DevInfo::SetUploadSwitch(context, DevInfo::UE_SWITCH_OFF);
            }
        }
    }  else if (event->messageType_ == Event::MessageType::RAW_EVENT) {
        // raw event, insert event
        if (EventUtil::RAW_EVENT_MIN <= event->eventId_ && event->eventId_ <= EventUtil::RAW_EVENT_MAX) {
            // todo:
        }
    } else if (EventUtil::FAULT_EVENT_MIN <= event->eventId_ && event->eventId_ <= EventUtil::FAULT_EVENT_MAX) {
        if (event->messageType_ == Event::MessageType::FAULT_EVENT) {
            ProcessEventLog(event);
        }
    } else {
        return ON_FAILURE;
    }
    return ON_SUCCESS;
}


void EventLogProcessor::ProcessUploadingLog(sp<Event> &event)
{
    // foreach workdir, find 9XX folder
    auto context = GetPluginContext();
    std::string workDir = context->GetWorkDir();

    std::vector<std::string> eventDirs = GetFileInDirByPattern(workDir, "9\\d{2}", false, false);
    if (eventDirs.size() == 0) {
        MISIGHT_LOGD("find 9xx folder dir=%s size is 0", workDir.c_str());
        return;
    }

    sp<EventQuotaXmlParser> xmlParser = EventQuotaXmlParser::getInstance();
    std::string cfgDir = context->GetConfigDir(xmlParser->GetXmlFileName());

    // copy file to uploading
    for (auto& dirName : eventDirs) {
        std::string eventDir = workDir + dirName + "/";
        std::string uploading = eventDir + UPLOADING_NAME + "/";

        std::string zipPattern = "EventId_\\S+_\\d{14}_" + dirName + "\\d{6}.zip";
        std::vector<std::string> zipFiles = GetFileInDirByPattern(eventDir, zipPattern, false, false);
        if (zipFiles.size() == 0) {
            MISIGHT_LOGD("event %s no zip", dirName.c_str());
            continue;
        }

        if (!FileUtil::CreateDirectory(uploading, FileUtil::FILE_EXEC_MODE)) {
            continue;
        }
        for (auto& zip : zipFiles) {
            int eventId = 0;
            int ret = sscanf(zip.substr(zip.size() - 13).c_str(), "%9d.zip", &eventId);// 13:901001001.zip
            if (ret != 1 || eventId == 0) {
                continue;
            }

            xmlParser->CheckLogCntLimitAfterIncr(cfgDir, miuiVersion_, eventId, -1);
            FileUtil::MoveFile(eventDir + zip, uploading + zip);
            FileUtil::ChangeMode(uploading + zip, FileUtil::FILE_EXEC_MODE);
        }
    }
}

void EventLogProcessor::ProcessUploadingEvent(sp<Event> &event)
{
    // foreach workdir, find 0 folder
    auto context = GetPluginContext();
    std::string workDir = context->GetWorkDir() + EventUtil::FAULT_EVENT_DEFAULT_DIR + "/";

    std::vector<std::string> eventDirs = GetFileInDirByPattern(workDir, "eventinfo\\.9\\d{2}(\\.0)*", false, false);
    if (eventDirs.size() == 0) {
        MISIGHT_LOGD("find eventinfo.9xx in dir=%s size is 0, type=%d", workDir.c_str(), event->messageType_);
        return;
    }

    std::string uploading = workDir + UPLOADING_NAME + "/";
    if (!FileUtil::CreateDirectory(uploading, FileUtil::FILE_EXEC_MODE)) {
        MISIGHT_LOGI("failed create uploading folder");
        return;
    }
    // copy file to uploading
    for (auto& eventInfoName : eventDirs) {
        std::string eventPath = workDir + eventInfoName;
        FileUtil::MoveFile(eventPath, uploading + eventInfoName);
        FileUtil::ChangeMode(uploading + eventInfoName, FileUtil::FILE_EXEC_MODE);
    }
}


bool EventLogProcessor::IsPrivacycompliance(sp<LogCollectInfo> logCollectInfo)
{
    std::string faultStr = logCollectInfo->GetValue(EventUploadXML::FAULT_LEVEL_ATTR_NAME);
    std::string privacyStr = logCollectInfo->GetValue(EventUploadXML::PRI_LEVEL_ATTR_NAME);
    EventUtil::FaultLevel faultLevel = EventUtil::GetFaultLevel(faultStr);
    EventUtil::PrivacyLevel privacyLevel = EventUtil::GetPrivacyLevel(privacyStr);

    EventUtil::FaultLevel faultLvlGbl = EventUtil::FaultLevel::GENERAL;
    EventUtil::PrivacyLevel privacyvlGbl = EventUtil::PrivacyLevel::L4;
    auto platform = GetPluginContext();
    std::string cfgDir = platform->GetConfigDir(PrivacyXmlParser::getInstance()->GetXmlFileName());
    std::string locale = platform->GetSysProperity(DevInfo::DEV_PRODUCT_LOCALE, "");
    PrivacyXmlParser::getInstance()->GetPolicyByLocale(cfgDir, miuiVersion_, locale, faultLvlGbl, privacyvlGbl);

    MISIGHT_LOGD("fault=%d<%d,privacy=%d>%d",  faultLevel, faultLvlGbl, privacyLevel, privacyvlGbl);
    if (faultLevel < faultLvlGbl) {
        return false;
    }
    if (privacyLevel > privacyvlGbl) {
        return false;
    }
    return true;
}

void EventLogProcessor::AddStaticLog(sp<LogCollectInfo>& logCollectInfo, std::vector<std::string>& logPaths)
{
    std::string path = logCollectInfo->GetValue(EventUploadXML::PATH_ATTR_NAME);
    if (path == "") {
        return;
    }

    logPaths.push_back(path);
    logCollectInfo->SetLogFile({path});
}

void EventLogProcessor::AddDynamicLog(sp<LogCollectInfo>& logCollectInfo, std::vector<std::string>& logPaths)
{

    std::string dir = logCollectInfo->GetValue(EventUploadXML::DIR_LEVEL_ATTR_NAME);
    std::string pattern = logCollectInfo->GetValue(EventUploadXML::PATTERN_ATTR_NAME);

    if (dir == "" || pattern == "") {
        MISIGHT_LOGD("upload dynamic dir=%s, pattern=%s invalid", dir.c_str(), pattern.c_str());
        return;
    }

    dir = "/" + StringUtil::TrimStr(dir, '/');
    std::vector<std::string> files = GetFileInDirByPattern(dir, pattern, true, true);
    if (files.size() == 0) {
        MISIGHT_LOGD("upload dynamic dir=%s,pattern=%s size is 0", dir.c_str(), pattern.c_str());
        return;
    }

    std::string option = logCollectInfo->GetValue(EventUploadXML::OPTION_ATTR_NAME);
    int count = logCollectInfo->GetOptionLatest();
    std::vector<std::string> delFiles;

    if (count) {
        logPaths.insert(logPaths.begin(), files.begin(), files.begin() + count);
        delFiles.insert(delFiles.begin(), files.begin(), files.begin() + count);

    } else {
        logPaths.insert(logPaths.begin(), files.begin(), files.end());
        delFiles = files;
    }
    logCollectInfo->SetLogFile(delFiles);

}

void EventLogProcessor::CheckAndClearLogFolder(sp<Event> event)
{
    auto context = GetPluginContext();
    std::string workDir = context->GetWorkDir();
    std::string dirName = std::to_string(event->eventId_ / EventUtil::FAULT_EVENT_SIX_RANGE);

    sp<EventQuotaXmlParser> xmlParser = EventQuotaXmlParser::getInstance();
    std::string cfgDir = context->GetConfigDir(xmlParser->GetXmlFileName());

    bool isLimit = xmlParser->CheckLogCntLimitAfterIncr(cfgDir, miuiVersion_, event->eventId_, 1);
    //quota is limit, no need clear
    if (!isLimit) {
        return;
    }

    EventQuotaXmlParser::DomainQuota quota = xmlParser->GetEventQuota(cfgDir, event->eventId_, miuiVersion_);
    std::vector<std::string> zipFiles;
    GetLogFiles(workDir + dirName, dirName, zipFiles);

    int index = 0;
    int delCnt = 0;
    for (auto& oneFile : zipFiles) {
        if (quota.eventMin != -1) {
            int eventId = 0;
            int ret = sscanf(oneFile.substr(oneFile.size() - 13).c_str(), "%9d.zip", &eventId);// 13:901001001.zip
            if (ret != 1 || eventId == 0) {
                continue;
            }
            if (eventId < quota.eventMin && eventId > quota.eventMax) {
                continue;
            }
        }
        if (index <= quota.maxLogCnt/2 - 1) {
            index++;
            continue;
        }
        FileUtil::DeleteFile(oneFile);
        delCnt--;
    }
    if (delCnt < 0) {
        xmlParser->CheckLogCntLimitAfterIncr(cfgDir, miuiVersion_, event->eventId_, delCnt);
    }

    zipFiles.clear();
}

std::string EventLogProcessor::GetEventDirPath(sp<Event> event)
{
    auto context = GetPluginContext();
    std::string workDir = context->GetWorkDir();
    std::string folder = std::to_string(event->eventId_ / EventUtil::FAULT_EVENT_SIX_RANGE);
    std::string eventDir = workDir + folder;

    if (DevInfo::GetUploadSwitch(context) != "on") {
        eventDir += "/ue_off";
    }

    if (!FileUtil::FileExists(eventDir)) {
        MISIGHT_LOGD("upload GetEventDirPath create dir=%s ", eventDir.c_str());
        FileUtil::CreateDirectory(eventDir, FileUtil::FILE_EXEC_MODE);
    }

    return eventDir;
}
#if 0
std::string EventLogProcessor::GetSnMd5Base64()
{
    std::string defaultSn = "unknown";
    auto context = GetPluginContext();
    std::string sn = context->GetSysProperity(DevInfo::DEV_SN_PROP, defaultSn);
    if (sn == defaultSn) {
        return sn;
    }

    // md5
    std::vector<unsigned char> outCmd(MD5_DIGEST_LENGTH);
    std::vector<unsigned char> inStr;
    inStr.assign(sn.begin(), sn.end());
    uint8_t *md5Sn = MD5(reinterpret_cast<uint8_t *>(inStr.data()), inStr.size(), outCmd.data());

    // base64
    size_t outLen;
    EVP_EncodedLength(&outLen, MD5_DIGEST_LENGTH);
    std::vector<char> base64(outLen);
    size_t rc = EVP_EncodeBlock(reinterpret_cast<uint8_t*>(base64.data()), md5Sn, MD5_DIGEST_LENGTH);
    if ((rc + 1) != outLen) { // 1: str tail symbol
        MISIGHT_LOGI("base64 failed, rc=%d outLen=%d src=%d",(int)rc, (int)outLen, MD5_DIGEST_LENGTH);
        return defaultSn;
    }
    sn.clear();
    sn.assign(base64.begin(), base64.begin() + rc);
    sn = StringUtil::ReplaceStr(sn, "/", "===");
    sn = StringUtil::ReplaceStr(sn, "\r", "===");
    sn = StringUtil::ReplaceStr(sn, "\n", "===");
    sn = StringUtil::ReplaceStr(sn, "\\", "===");
    return sn;
}
#endif
std::string EventLogProcessor::GetLogPackName(sp<Event> event)
{
    // log file name rule
    // Eventid_dev_devVersion_sn_timestamp_eventid_type
    auto context = GetPluginContext();
    std::string verDesc = context->GetSysProperity(DevInfo::DEV_BUILD_FINGERPRINT, "unknown");
    verDesc = StringUtil::ReplaceStr(verDesc, "/", "_");
    verDesc = StringUtil::ReplaceStr(verDesc, ":", "_");
    std::string fileName = "EventId_" + verDesc /*+ "_" + GetSnMd5Base64()*/ + "_"
        + TimeUtil::GetTimeDateStr(std::time(nullptr), "%Y%m%d%H%M%S")
        + "_" + std::to_string(event->eventId_) + ".zip";
    MISIGHT_LOGD("GetLogPackPath fileName=%s", fileName.c_str());
    return fileName;
}

void EventLogProcessor::AddCmdLog(sp<Event> event, sp<LogCollectInfo>& logCollectInfo, std::vector<std::string>& logPaths)
{
    std::string runCmd = logCollectInfo->GetValue(EventUploadXML::RUN_ATTR_NAME);
    std::string output = logCollectInfo->GetValue(EventUploadXML::OUT_ATTR_NAME);
    if (runCmd == "") {
        return;
    }

    // cmd output file path
    std::string cmdFile = GetEventDirPath(event) + "/" + TimeUtil::GetTimeDateStr(std::time(nullptr), "%Y%m%d%H%M%S") + "_";

    if (output != "") {
        cmdFile += output;
    } else {
        // if output is null, output is cmd
        output = runCmd;

        std::regex rex(" |\\n|/");
        std::string outRep = std::regex_replace(output, rex, "_");
        const int maxLen = 10;
        cmdFile += outRep.substr(0, maxLen);
        if (outRep.size() > maxLen) {
            cmdFile += outRep.substr(outRep.size() - maxLen, maxLen);
        }
    }

    int fd = open(cmdFile.c_str(), O_CREAT | O_WRONLY, FileUtil::FILE_DEFAULT_MODE);
    if (fd < 0) {
        MISIGHT_LOGD("Fail to create %s.", cmdFile.c_str());
        return;
    }
    CmdProcessor::WriteCommandResultToFile(fd, runCmd);
    close(fd);
    logPaths.push_back(cmdFile);

    MISIGHT_LOGD("upload AddCmdLog cmdFile=%s ", cmdFile.c_str());
    logCollectInfo->SetDelete(true);

    logCollectInfo->SetLogFile({cmdFile});
    return;
}

std::vector<std::string> EventLogProcessor::GetSrcLogFileList(sp<Event> event,
    std::list<sp<LogCollectInfo>>& logInfoList)
{
    std::vector<std::string> logPaths;

    for (auto& logCollectInfo : logInfoList) {
        if (!IsPrivacycompliance(logCollectInfo)) {
            continue;
        }
        // static path, add into logPaths
        if (logCollectInfo->GetLogCollectType() == LOG_COLLECT_STATIC) {
            MISIGHT_LOGD("log type =LOG_COLLECT_STATIC");
            AddStaticLog(logCollectInfo, logPaths);

        }
        if (logCollectInfo->GetLogCollectType() == LOG_COLLECT_DYNAMIC) {
            MISIGHT_LOGD("log type =LOG_COLLECT_DYNAMIC");
            AddDynamicLog(logCollectInfo, logPaths);
        }
        if (logCollectInfo->GetLogCollectType() == LOG_COLLECT_CMD) {
            MISIGHT_LOGD("log type =LOG_COLLECT_CMD");
            AddCmdLog(event, logCollectInfo, logPaths);
        }
    }
    return logPaths;
}

bool EventLogProcessor::PackLogToZip(sp<Event> event, std::vector<std::string> logPaths)
{
    std::string eventDir = GetEventDirPath(event);
    std::string zipName = GetLogPackName(event);

    sp<ZipPack> zipPack = new ZipPack();
    if (zipPack == nullptr || !zipPack->CreateZip(eventDir + "/" + zipName)) {
        MISIGHT_LOGD("create zip failed %u", event->eventId_);
        return false;
    }

    zipPack->AddFiles(logPaths);
    zipPack->Close();

    if (eventDir.find("ue_off") == eventDir.npos) {
        // finally, log zip file will be moved to 9xx/uploading folder
        event->SetValue(EventUtil::EVENT_LOG_ZIP_NAME, eventDir + "/" + UPLOADING_NAME + "/" + zipName);
    }
    FileUtil::ChangeMode(eventDir + "/" + zipName, FileUtil::FILE_EXEC_MODE);
    return true;
}

void EventLogProcessor::DeleteLogFile(std::list<sp<LogCollectInfo>>& logInfoList)
{
    for (auto& logCollectInfo : logInfoList) {
        if (!logCollectInfo->IsDeleteLog()) {
            continue;
        }
        logCollectInfo->DeleteLogFile();
    }
}

void EventLogProcessor::ProcessEventLog(sp<Event> event)
{
    if (event->GetValue(EventUtil::EVENT_NOCATCH_LOG) != "") {
        // fault event ocurred in one minute, no need log collect
        event->SetValue(EventUtil::EVENT_LOG_ZIP_NAME, event->GetValue(EventUtil::EVENT_NOCATCH_LOG));
        return;
    }
    if (event->GetValue(EventUtil::EVENT_DROP_REASON) != "") {
        // fault event drop reason is not null, no need log collect
        return;
    }

    MISIGHT_LOGD("ProcessEventLog ProcessEventLog evetind %d", event->eventId_);
    // read upload xml, get event upload info
    std::string cfgDir = GetPluginContext()->GetConfigDir(EventUploadXmlParser::getInstance()->GetXmlFileName());
    auto logCollectInfoList = EventUploadXmlParser::getInstance()->GetEventLogCollectInfo(cfgDir, event->eventId_);
    if (logCollectInfoList.size() == 0) {
        //MISIGHT_LOGI("find xml log is null");
        return;
    }

    // capture log
    auto logPaths = GetSrcLogFileList(event, logCollectInfoList);
    if (logPaths.size() == 0) {
        return;
    }

    // zip log
    bool ret = PackLogToZip(event, logPaths);

    // delete capture log file
    DeleteLogFile(logCollectInfoList);
    if (ret) {
        CheckAndClearLogFolder(event);
    }
}

bool EventLogProcessor::CanUpload()
{
    auto context = GetPluginContext();
    if (DevInfo::GetUploadSwitch(context) != "on") {
        MISIGHT_LOGI("dev swtich %s", DevInfo::GetUploadSwitch(context).c_str());
        return false;
    }
    // factory mode, no upload
    std::string factory = context->GetSysProperity(DevInfo::DEV_FACTORY_MODE, "");
    if (factory == "1") {
        MISIGHT_LOGI("factory mode, no upload");
        return false;
    }

    // only cn region can upload
    std::string region = context->GetSysProperity(DevInfo::DEV_MISIGHT_REGINE, "");
    if (region != "CN") {
        MISIGHT_LOGI("region not support upload");
        return false;
    }
    return IsNotExpired();
}

bool EventLogProcessor::IsNotExpired()
{
    auto context = GetPluginContext();
    // get summary quota form quota xml
    std::string cfgDir = context->GetConfigDir(EventQuotaXmlParser::getInstance()->GetXmlFileName());
    EventQuotaXmlParser::SummaryQuota totalQuota =
        EventQuotaXmlParser::getInstance()->GetSummaryQuota(cfgDir, miuiVersion_);
    // three months

    //
    // check currenttime over config
    uint64_t activeTime = DevInfo::GetActivateTime();
    uint64_t currentTime = TimeUtil::GetTimestampSecond();

    if (currentTime <= (activeTime + totalQuota.level1Month * DAY_SECONDS * MONTH_DAY)) {
        return true;
    }
    bool ret = false;
    if (currentTime < (activeTime + totalQuota.level2Month * DAY_SECONDS * MONTH_DAY)) {
        ret = CanUploadRatio(totalQuota.level1Ratio);
    } else if (currentTime < (activeTime + totalQuota.level3Month * DAY_SECONDS * MONTH_DAY)) {
        ret = CanUploadRatio(totalQuota.level2Ratio);
    } else if (currentTime < (activeTime + totalQuota.uploadExpireYear * DAY_SECONDS * YEAR_DAYS)) {
        ret = CanUploadRatio(totalQuota.level3Ratio);
    } else {
        MISIGHT_LOGW("upload expired, close upload active=%" PRId64, activeTime);
    }

    if (!ret) {
        DevInfo::SetUploadSwitch(context, DevInfo::UE_SWITCH_ONCTRL);
    }
    return ret;
}

bool EventLogProcessor::CanUploadRatio(int32_t level)
{
    std::string defaultSn = "unknown";
    auto context = GetPluginContext();
    std::string sn = context->GetSysProperity(DevInfo::DEV_SN_PROP, defaultSn);
    if (sn == defaultSn) {
        sn = context->GetSysProperity(DevInfo::DEV_PSN_PROP, defaultSn);
        if (sn == defaultSn) {
            MISIGHT_LOGW("unknown curent state, close upload");
            return false;
        }
    }
    uint32_t lastNum = (sn.at(sn.length()-1) - 48) % 10;// 48 ASCII table, sn range 0-9;
    if (level > lastNum) {
        return true;
    }
    MISIGHT_LOGW("last number larger raito %u, level=%u, close upload",  lastNum, level);
    return false;
}

void EventLogProcessor::OnUnorderedEvent(sp<Event> event)
{
    if (event == nullptr) {
        return;
    }
    if (event->messageType_ != Event::MessageType::PLUGIN_MAINTENANCE || event->eventId_ != EVENT_SERVICE_CONFIG_UPDATE_ID) {
        return;
    }
    GetLooper()->AddEvent(this, event);
}
}
}
