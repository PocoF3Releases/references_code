/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight plugin testability event store manager head file
 *
 */

#ifndef MISIGHT_PLUGINS_TESTABILITY_EVENT_STORE_MANAGER_TEST
#define MISIGHT_PLUGINS_TESTABILITY_EVENT_STORE_MANAGER_TEST

#include <gtest/gtest.h>

#include <utils/StrongPointer.h>

#include "event_storage_manager.h"


class EventStoreManagerTest : public testing::Test {
public:
    static void SetUpTestCase();

    static void TearDownTestCase()
    {
    }

protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
    static bool InitEventStorePlugin();

    static android::sp<android::MiSight::EventStorageManager> eventStore_;

};


class EventDatabaseHelperTest : public testing::Test {
public:
    static void SetUpTestCase() {}

    static void TearDownTestCase()
    {
    }

protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }

};



#endif

