/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability plugin event info pack test head file
 *
 */

#include "core_common_test.h"

#include <list>
#include <string>
#include <time.h>
#include <vector>

#include "dev_info.h"
#include "file_util.h"
#include "string_util.h"
#include "time_util.h"

using namespace android;
using namespace android::MiSight;

TEST_F(CoreCommonTest, TestTimeUtil) {
    // 1656985326 2022-07-05 09:42:06
    std::string timeStr = TimeUtil::GetTimeDateStr(1656985326, "%Y%m%d%H%M%S");
    ASSERT_EQ(timeStr, "20220705094206");

    timeStr = TimeUtil::GetTimeDateStr(1656985326, "%Y-%m-%d %H:%M:%S");
    ASSERT_EQ(timeStr, "2022-07-05 09:42:06");
    ASSERT_EQ(TimeUtil::DateStrToTime(timeStr, "%d-%d-%d %d:%d:%d"), 1656985326);
    ASSERT_EQ(TimeUtil::GetCurrentTime(), TimeUtil::GetTimestampSecond());
}


TEST_F(CoreCommonTest, TestStringUtil) {
    std::string trimStr = StringUtil::TrimStr(" xx xx ");
    ASSERT_EQ("xx xx", trimStr);
    trimStr = StringUtil::TrimStr("a xx xx a", 'a');
    ASSERT_EQ(" xx xx ", trimStr);

    std::list<std::string> strList = StringUtil::SplitStr("a b c");
    ASSERT_EQ(strList.size(), 3);
    strList = StringUtil::SplitStr("a,b,c", ',');
    ASSERT_EQ(strList.size(), 3);

    int32_t intTest = StringUtil::ConvertInt("3");
    ASSERT_EQ(3, intTest);
    intTest = StringUtil::ConvertInt("a");
    ASSERT_EQ(0, intTest);

    bool find = StringUtil::StartsWith("123abc", "123");
    ASSERT_EQ(true, find);
    find = StringUtil::StartsWith("123abc", "abc");
    ASSERT_EQ(false, find);
    find = StringUtil::EndsWith("123abc", "abc");
    ASSERT_EQ(true, find);
    find = StringUtil::EndsWith("123abc", "123");
    ASSERT_EQ(false, find);

    std::string replace = StringUtil::ReplaceStr("123abc123", "123", "abc");
    ASSERT_EQ("abcabcabc", replace);
    replace = StringUtil::ReplaceStr("123abc123", "456", "abc");
    ASSERT_EQ("123abc123", replace);

    replace = StringUtil::EscapeSqliteChar("'/%&_");
    ASSERT_EQ("''///%/&/_", replace);
}


TEST_F(CoreCommonTest, TestFileUtil) {
    std::string path = "/data/test/create";
    FileUtil::DeleteFile(path);
    bool ret = FileUtil::CreateDirectory(path);
    ASSERT_EQ(ret, true);
    ret = FileUtil::FileExists(path);
    ASSERT_EQ(ret, true);
    ret = FileUtil::FileExists(path+"d");
    ASSERT_EQ(ret, false);
    ret = FileUtil::IsDirectory(path);
    ASSERT_EQ(ret, true);

    std::string filePath = path + "/test";
    ret = FileUtil::SaveStringToFile(filePath, filePath);
    ASSERT_EQ(ret, true);
    ret = FileUtil::IsDirectory(filePath);
    ASSERT_EQ(ret, false);
    int32_t fileSize = FileUtil::GetFileSize(filePath);
    ASSERT_EQ(fileSize, filePath.size());

    std::string content;
    ret = FileUtil::LoadStringFromFile(filePath, content);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(content, filePath);
    ret = FileUtil::MoveFile(filePath, path + "/dst_test");
    printf("ret=%d\n", ret);
    FileUtil::DeleteFile(path + "/dst_test");
    ret = FileUtil::FileExists(path + "/dst_test");
    ASSERT_EQ(ret, false);
    std::string notExist = path + "/a";
    ASSERT_EQ(-1, FileUtil::GetDirectorySize(notExist));
    ASSERT_LT(0, FileUtil::GetDirectorySize(path));
    size_t size = 0;
    ASSERT_GE(0, FileUtil::DeleteDirectoryOrFile(path.c_str(), &size));
    ASSERT_LE(0, size);
}

class PluginContextDemo : public PluginContext {
public:
    PluginContextDemo() {
        mapProp_.clear();
    }
    ~PluginContextDemo() {
        mapProp_.clear();
    }
    std::string GetSysProperity(const std::string& key, const std::string& defVal) override
    {
        auto propPair = mapProp_.find(key);
        if (propPair != mapProp_.end()) {
            printf("GetSysProperity=%s,%s\n",key.c_str(),propPair->second.c_str());
            return propPair->second;
        }
        printf("GetSysProperity=%s,def=%s\n",key.c_str(),defVal.c_str());
        return defVal;
    }

    bool SetSysProperity(const std::string& key, const std::string& val, bool update) override
    {
        mapProp_[key] = val;
        printf("SetSysProperity=%s,def=%s\n",key.c_str(),mapProp_[key].c_str());
        return update;
    }
    std::map<std::string, std::string> mapProp_;
};


TEST_F(CoreCommonTest, TestDevInfo) {
    ASSERT_EQ(DevInfo::MiuiVersionType::PRE, DevInfo::GetMiuiVersionType(nullptr));
    ASSERT_EQ(true, DevInfo::IsInnerVersion(""));
    ASSERT_EQ(DevInfo::GetUploadSwitch(nullptr), "off");

    std::string runMode = "";
    std::string filePath = "";
    DevInfo::GetRunMode(nullptr, runMode, filePath);
    ASSERT_EQ(runMode, "InnerDev");
    ASSERT_EQ(filePath, "");
    ASSERT_EQ(0, DevInfo::GetActivateTime());
    DevInfo::SetUploadSwitch(nullptr, DevInfo::UE_SWITCH_OFF);
    DevInfo::SetRunMode(nullptr, "", "");

    PluginContextDemo* context = new PluginContextDemo();
    std::string ver = "V14.0.0.0.XXXXXX";
    context->SetSysProperity(DevInfo::DEV_VERSION_PROP, ver, false);
    ASSERT_EQ(ver, DevInfo::GetMiuiVersion(context));
    ASSERT_EQ(true, DevInfo::IsMiuiStableVersion(ver));
    ASSERT_EQ(false, DevInfo::IsMiuiDevVersion(ver));
    ver = "V14.0.0.DEV";
    ASSERT_EQ(false, DevInfo::IsMiuiStableVersion(ver));
    ASSERT_EQ(true, DevInfo::IsMiuiDevVersion(ver));

    DevInfo::SetRunMode(context, "test", "/data/a.log");
    ASSERT_EQ(true, DevInfo::GetRunMode(context, runMode, filePath));
    ASSERT_EQ(runMode, "test");
    ASSERT_EQ(filePath, "/data/a.log");

    printf("ue:%s\n", DevInfo::GetUploadSwitch(context).c_str());
    DevInfo::SetUploadSwitch(context, DevInfo::UE_SWITCH_OFF);
    ASSERT_EQ("off", DevInfo::GetUploadSwitch(context));
    delete context;
}
