/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _CAPTURESERVER_H_
#define _CAPTURESERVER_H_
#include "dump.h"
#include <string.h>
#include <utils/threads.h>

namespace android{
    class CaptureServerThread : public Thread {
    public:
        CaptureServerThread();
    private:
        std::string mTopDir;
        bool checkAndSaveLog(std::string& currLog);
        void runCaptureLog(const char* type);
        int charArr2Int(char b[]);
        std::string createLogPath(const char* type);
    protected:
        virtual bool threadLoop();
    };
}

#endif //_CAPTURESERVER_H_
