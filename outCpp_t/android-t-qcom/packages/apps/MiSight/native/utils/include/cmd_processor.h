/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight cmd executed and praser head file
 *
 */

#ifndef MISIGHT_UTILS_CMD_PROCESSOR_H
#define MISIGHT_UTILS_CMD_PROCESSOR_H

#include <string>

#include <sys/stat.h>

namespace android {
namespace MiSight {

static constexpr uint32_t DEFAULT_DURATION_SECONDS = 60;
static constexpr uint32_t BUF_SIZE_64 = 64;
static constexpr uint32_t BUF_SIZE_128 = 128;
static constexpr uint32_t BUF_SIZE_256 = 256;
static constexpr uint32_t BUF_SIZE_512 = 512;
static constexpr uint32_t BUF_SIZE_4096 = 4096;
static constexpr uint64_t NS_PER_SECOND = 1000000000;
static constexpr uint32_t MAX_LINE_LEN = 1024;

namespace CmdProcessor {

std::string GetProcNameByPid(int32_t pid);
pid_t GetPidByName(const std::string& processName);
int32_t ExecCommand(const std::string &cmd, const std::vector<std::string> &args);
bool IsSpecificCmdExist(const std::string& fullPath);
bool WriteCommandResultToFile(int fd, const std::string& cmd);
bool GetCommandResult(const std::string& cmd, std::string& result);


};
} // namespace MiSight
} // namespace android
#endif
