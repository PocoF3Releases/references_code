/**
 * main.cpp
 * 主程序
 */

#define LOG_TAG "syspressure.test"
//#define LOG_NDEBUG 0
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <signal.h>
#include "SysPressureCtrlManager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "resmonitor.main"

int main(int argc, char** argv) {
    android::syspressure::SysPressureCtrlManager::getInstance()->init();
    android::IPCThreadState::self()->joinThreadPool();
    return 0;
}