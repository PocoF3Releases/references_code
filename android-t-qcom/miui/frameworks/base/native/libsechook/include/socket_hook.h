/*
 * socket_hook.h
 *
 *  Created on: 2014-2-10
 *      Author: wangle
 */

#ifndef SOCKET_HOOK_H_
#define SOCKET_HOOK_H_

#ifdef __cplusplus
extern "C" {
#endif

void block_self_network(int);

int is_block_self_network();

void set_shandow_socket(int (*)(int, int, int));

extern int (*real_socket_ptr)(int, int, int);

int ___socket(int, int, int);

#ifdef __cplusplus
}
#endif

#endif /* SOCKET_HOOK_H_ */
