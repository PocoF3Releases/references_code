#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../common/log.h"

#include "FSUtil.h"

#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>

#include "PriviledgedProcService.h"

int startAsDaemon() {
    KLOGI("startAsDaemon: called. ");
    PriviledgedProcService::publishAndJoinThreadPool();
    return 0;
}


int onPostFS() {
    KLOGI("onPostFS: called. ");
    return FSUtil::restoreFSOwnerAndContext();
}

int onPostFSData() {
    KLOGI("onPostFSData: called. ");
    return FSUtil::restoreFSOwnerAndContext();
}

int main(int argc, char ** argv) {

    // Logd may not be ready.
    // Use kernel log instead in this file.
#ifdef NO_KLOG_INIT
#else
    klog_init();
#endif
    klog_set_level(KLOG_DEBUG_LEVEL);

    if (argc != 2) {
        KLOGE("main: wrong arguement count. ");
        return -1;
    }

    const char * proc = argv[1];

    if (!strcmp("daemon", proc)) {
        KLOGI("main: start daemon. ");
        return startAsDaemon();
    } else if (!strcmp("post-fs", proc)) {
        KLOGI("main: handle post-fs. ");
        return onPostFS();
    } else if (!strcmp("post-fs-data", proc)) {
        KLOGI("main: handle post-fs-data");
        return onPostFSData();
    }

    KLOGE("Unrecognized proc: %s. ", proc);
}
