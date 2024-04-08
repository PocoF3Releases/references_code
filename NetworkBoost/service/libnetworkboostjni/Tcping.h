#ifndef _TCPING_H_
#define _TCPING_H_

#include <netinet/ip.h>
#include <netinet/in.h>

#define BUFSIZE 100
#define TIME_OUT_MS 500

#define DEFAULT_RTT_SERVER "223.5.5.5"
#define DEFAULT_RTT_PORT 443

#define BILLION 1000000000

typedef struct {
    const char *device;
    const char *server;
    uint32_t port;
}Ping_info;


int getInterfaceTcpingRtt(Ping_info *ping_info);


#endif /* !_SLA_RAW_PING_H */