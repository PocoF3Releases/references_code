/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight testability plugin event log processor test head file
 *
 */
#ifndef MISIGHT_PLUGINS_TESTABILITY_EVENT_LOG_PROCESSOR_TEST
#define MISIGHT_PLUGINS_TESTABILITY_EVENT_LOG_PROCESSOR_TEST

#include <gtest/gtest.h>
#include "event_log_processor.h"

class EventLogProcessorTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();


protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
    static ::android::sp<::android::MiSight::EventLogProcessor> eventLog_;

};

#endif

