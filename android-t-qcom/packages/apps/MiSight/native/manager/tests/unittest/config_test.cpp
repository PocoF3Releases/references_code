/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight config parser test implement file
 *
 */

#include "config_test.h"

#include "config.h"
using namespace android::MiSight;

TEST_F(ConfigTest, TestParseNormal) {
    std::string path = "/data/test/config/plugin_config_normal";
    Config cfg(path);
    ASSERT_EQ(true, cfg.Parse());
    ASSERT_EQ(cfg.GetPluginInfoList().size(), 6);
    ASSERT_EQ(cfg.GetPipelineInfoList().size(), 3);
}

// plugin format fault
TEST_F(ConfigTest, TestParseAbnormal_LoadFault) {
    std::string path = "/data/test/config/plugin_config_abnormal1";
    Config cfg(path);
    ASSERT_EQ(false, cfg.Parse());
}

// plugin thread module set fault
TEST_F(ConfigTest, TestParseAbnormal_ThreadNoName) {
    std::string path = "/data/test/config/plugin_config_abnormal2";
    Config cfg(path);
    ASSERT_EQ(false, cfg.Parse());
}

// plugin thread module set fault
TEST_F(ConfigTest, TestParseAbnormal_ThreadNoType) {
    std::string path = "/data/test/config/plugin_config_abnormal3";
    Config cfg(path);
    ASSERT_EQ(false, cfg.Parse());
}

// plugin title fault
TEST_F(ConfigTest, TestParseAbnormal_TitleFault) {
    std::string path = "/data/test/config/plugin_config_abnormal4";
    Config cfg(path);
    ASSERT_EQ(false, cfg.Parse());
}

// plugin not define
TEST_F(ConfigTest, TestParseAbnormal_UnDefPlugin) {
    std::string path = "/data/test/config/plugin_config_abnormal5";
    Config cfg(path);
    ASSERT_EQ(false, cfg.Parse());
}

// pipeline not define
TEST_F(ConfigTest, TestParseAbnormal_UnDefPipeline) {
    std::string path = "/data/test/config/plugin_config_abnormal6";
    Config cfg(path);
    ASSERT_EQ(false, cfg.Parse());
}
// plugin redefine
TEST_F(ConfigTest, TestParseAbnormal_RedefPlugin) {
    std::string path = "/data/test/config/plugin_config_abnormal7";
    Config cfg(path);
    ASSERT_EQ(false, cfg.Parse());
}


