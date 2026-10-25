#ifndef __MODEM_EVENT_HANDLER
#define __MODEM_EVENT_HANDLER

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sysutils/NetlinkEvent.h>
#include <sysutils/NetlinkListener.h>
#include <log/log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "ModemEventHandler"

class ModemEventHandler:public NetlinkListener {
public:
    ModemEventHandler(int sock);
    virtual ~ModemEventHandler();

protected:
    virtual void onEvent(NetlinkEvent *evt);
};

#endif  //__MODEM_EVENT_HANDLER
