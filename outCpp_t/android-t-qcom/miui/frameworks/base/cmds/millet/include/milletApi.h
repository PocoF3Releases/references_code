#ifndef MILLET_API
#define MILLET_API

#include "millet.h"

#include <stdio.h>

#define MILLET_DEBUG             1
#define MILLET_ERR		-1
#define MILLET_OK		 0

typedef struct millet_sock
{
	int sock;
	int in_use;
} millet_sock;

typedef enum {
	O_POLICY,
	PERF_POLICY,
	POWER_POLICY,
	GAME_POLICY,
	POLICY_TYPE_NUM,
} POLICY_TYPE;
#define POLICY_VALID(x) ((x < POLICY_TYPE_NUM) && (x > O_POLICY))

static inline void dump_msg(const struct millet_data* msg)
{
	if (!msg)
		return;
	printf("msg: %d\n", msg->msg_type);
	printf("type: %d\n", msg->owner);
	printf("src_port: 0x%lx\n", msg->src_port);
	printf("dest_port: 0x%lx\n", msg->dst_port);
	printf("uid: %d\n", msg->uid);
}

typedef void (*millet_callback)(const struct millet_data *data);
extern int register_mod_hook(millet_callback hook,  enum MILLET_TYPE, POLICY_TYPE policy);

extern "C" {
	int create_millet_sock(millet_sock *sock_fd);
	int destroy_millet_sock(millet_sock *sock_fd);
	int millet_sendmsg(millet_sock *sock_fd, enum MILLET_TYPE type, struct millet_userconf *msg);
	int millet_recv_loop(millet_sock *sock_fd, enum MILLET_TYPE type, millet_callback hook);
}
#endif
