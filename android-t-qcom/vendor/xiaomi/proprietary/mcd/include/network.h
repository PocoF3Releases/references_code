#ifndef OCTVM_NETWORK_H
#define OCTVM_NETWORK_H

#ifdef __cplusplus
extern "C" {
#endif

void net_init_dns(const char* conf);

int32_t net_dns_start_server(const char* conf);
void net_dns_stop_server();
int net_dns_reload();


#ifdef __cplusplus
}
#endif

#endif
