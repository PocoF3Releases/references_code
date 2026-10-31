/*
 * Copyright (C) Xiaomi Inc.
 * Description: misight event looper test head file
 *
 */

#ifndef MISIGHT_EVENTLOOP_UNIT_TEST
#define MISIGHT_EVENTLOOP_UNIT_TEST

#include <gtest/gtest.h>

class EventLoopTest : public testing::Test {
public:
    static void SetUpTestCase()
    {

    }
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


class PluginTest : public testing::Test {
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
class CommonTest : public testing::Test {
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

