/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability plugin event info pack test head file
 *
 */

#include "utils_test.h"

#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>


#include "cmd_processor.h"
#include "file_util.h"
#include "zip_pack.h"

using namespace android;
using namespace android::MiSight;

TEST_F(UtilsTest, TestZip) {
    std::string zipFile = "/data/test/ziptest.zip";
    if (FileUtil::FileExists(zipFile)) {
        FileUtil::DeleteFile(zipFile);
    }
    auto zipPack = new ZipPack();
    bool ret = zipPack->CreateZip(zipFile);
    ASSERT_EQ(ret, true);

    ret = zipPack->CreateZip(zipFile);
    ASSERT_EQ(ret, true);

    ret = zipPack->AddFiles({});
    ASSERT_EQ(ret, true);
    zipPack->Close();

    ret = FileUtil::FileExists(zipFile);
    ASSERT_EQ(ret, true);

    FileUtil::DeleteFile(zipFile);
    ret = zipPack->CreateZip(zipFile);
    ASSERT_EQ(ret, true);

    ret = zipPack->AddFiles({"/system/etc/misight/event.xml",
        "/system/etc/misight/event_quota.xml",
        "/system/etc/misight/event_upload.xml",
        "/system/etc/misight/plugin_config",
        "/system/etc/misight/privacy.xml",
        "/data/test",
        "/system/etc/misight/privacy.xml-no",
        });
    ASSERT_EQ(ret, true);
    zipPack->Close();
    delete zipPack;
}

TEST_F(UtilsTest, TestCmd) {
    std::string ueStr = "";
    ASSERT_TRUE(CmdProcessor::GetCommandResult("settings get secure upload_log_pref", ueStr));
    ASSERT_NE(ueStr, "");

    int fd = open("/data/test/cmd_test.file", O_CREAT | O_WRONLY, FileUtil::FILE_DEFAULT_MODE);
    ASSERT_GT(fd, 0);
    ASSERT_TRUE(CmdProcessor::WriteCommandResultToFile(fd, "ls -al /data/test"));
    close(fd);

    ASSERT_TRUE(CmdProcessor::IsSpecificCmdExist("/system/bin/misight"));

    pid_t pid = CmdProcessor::GetPidByName("system_server");
    ASSERT_GT(pid, 0);
    std::string name = CmdProcessor::GetProcNameByPid(pid);
    printf("name=====%s>>\n", name.c_str());
    ASSERT_TRUE(name == "system_server");

    ASSERT_EQ(CmdProcessor::ExecCommand("ls", {"ls", "-al", "/data/test"}), 0);
}

