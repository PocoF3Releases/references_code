#pragma once

#include <unordered_map>
#include "common/Common.h"
#include "process/Process.h"

namespace android {

namespace syspressure {

class CpuLimit {
public:
    CpuLimit();
    ~CpuLimit();
    int addProcessLimit(int32_t pid, std::string& processName, int32_t cpuUsage);
    int removeProcessLimit(int32_t pid);

    void onCpuPressure(int level);

    // history
    std::shared_ptr<VectorCpuLimitInfo> getHistoryInfos(uint64_t sinceUptime);
    // current
    std::shared_ptr<VectorCpuLimitInfo> getCpuLimitInfos();

private:
    std::shared_ptr<ProcessPools> mpProcessPools;
    uint64_t mPreTrackTime;
};

} // namespace syspressure

} // namespace android