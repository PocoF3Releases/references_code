#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <cutils/properties.h>

#include "millet.h"
#include "milletApi.h"

#define MILLET_DEBUG             1
#define MILLET_ERR		-1
#define MILLET_OK		 0

/* call in handle_msg*/
static uint32_t getProp(const char *str) {
    char buf[PROP_VALUE_MAX];
    property_get(str, buf, "30");
    return atoi(buf);
}
static int NETLINK_MILLET = getProp("ro.millet.netlink");
static void millet_transmit(struct millet_data* msg, millet_callback hook)
{
	if (MILLET_DEBUG)
		dump_msg(msg);

	if (hook)
		hook(msg);
	else
		dump_msg(msg);
}

static void handle_msg(struct millet_data* msg, millet_callback hook)
{
	static enum MILLET_TYPE active_mod = O_TYPE;

	if (!msg) {
		printf("MILLET_USER: MSG NULL \n!");
		return;
	}

	if (!MSG_VALID(msg->msg_type)) {
		printf("MILLET_USER: msg invalid\n!");
		return;
	}

	if ((msg->src_port !=  MILLET_KERNEL_ID) ||
		(msg->dst_port !=  MILLET_USER_ID)) {
		printf("MILLET_USER: msg port is wrong\n!");
		return;
	}

	switch (msg->msg_type) {
	case LOOPBACK_MSG:
 		printf("MILLET_USER: shake hands success\n");
		if (TYPE_VALID(msg->monitor))
			active_mod = msg->monitor;
	break;

	case MSG_TO_USER:
		if (TYPE_VALID(msg->monitor)
		    && active_mod == msg->monitor)
			millet_transmit(msg, hook);
		else
			printf("MILLET_USER: mod tye error %d\n",
			    msg->monitor);
	break;

	case MSG_TO_KERN:
	default:
		printf("MILLET_USER: msg tye error %d\n",
		    msg->msg_type);
	break;
	}
}

int create_millet_sock(millet_sock *sock_fd)
{
	int ret = MILLET_OK;
	struct sockaddr_nl src_addr;

	if (!sock_fd || sock_fd->in_use) {
		printf("MILLET_USER: NULL or in use: \n");
		return MILLET_ERR;
	}

	sock_fd->sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_MILLET);
	if (sock_fd->sock == -1) {
		printf("MILLET_USER: create socket error: %s",
			strerror(errno));
		return MILLET_ERR;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	src_addr.nl_groups = 0;

	ret = bind(sock_fd->sock, (struct sockaddr*)&src_addr,
			sizeof(src_addr));
	if (ret < 0) {
		printf("MILLET_USER: bind failed: %s", strerror(errno));
		close(sock_fd->sock);
		return MILLET_ERR;
	}

	sock_fd->in_use = 1;
	return MILLET_OK;
}

int destroy_millet_sock(millet_sock *sock_fd)
{
	if (sock_fd && sock_fd->sock) {
		sock_fd->in_use = 0;
		close(sock_fd->sock);
	}

	return MILLET_OK;
}

int millet_sendmsg(millet_sock *sock_fd, enum MILLET_TYPE type,
		struct millet_userconf *data)
{
	int ret = MILLET_OK;
	struct sockaddr_nl dst_addr;
	struct nlmsghdr *nlh = NULL;
	struct iovec iov;
	struct msghdr msg;
	struct millet_userconf* millet_msg;
	int msglen = sizeof(struct millet_userconf);

	if (!data) {
		printf("MILLET_USER: send NULL msg\n");
		return MILLET_ERR;
	}

	if (!TYPE_VALID(type)) {
		printf("MILLET_USER: sendmsg valid type %d\n", type);
		return MILLET_ERR;
	}

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msglen));
	if (!nlh) {
		printf("MILLET_USER: malloc millet msg failed!\n");
		return MILLET_ERR;
	}

	nlh->nlmsg_len = NLMSG_SPACE(msglen);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;
	millet_msg = (struct millet_userconf*)NLMSG_DATA(nlh);
	memcpy(millet_msg, data, sizeof(struct millet_userconf));

	millet_msg->msg_type  = data->msg_type;
	millet_msg->owner     = type;
	millet_msg->src_port  = MILLET_USER_ID;
	millet_msg->dst_port  = MILLET_KERNEL_ID;

	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(msglen);

	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.nl_family = AF_NETLINK;
	dst_addr.nl_pid = 0;
	dst_addr.nl_groups = 0;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&dst_addr;
	msg.msg_namelen = sizeof(dst_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (sock_fd->in_use) {
		ret = sendmsg(sock_fd->sock, &msg, 0);
		if (ret == MILLET_ERR) {
			printf("MILLET_USER: sendmsg failed %s\n",
				strerror(errno));
			destroy_millet_sock(sock_fd);
			free(nlh);
			return errno;
		}
	}

	free(nlh);
	return MILLET_OK;
}

int millet_recv_loop(millet_sock *sock_fd, enum MILLET_TYPE type,
		millet_callback recv_hook)
{
	struct millet_userconf data;
        struct sockaddr_nl dst_addr;
        struct nlmsghdr *nlh = NULL;
        struct iovec iov;
        struct msghdr msg;
        int ret = MILLET_OK;
	int msglen = sizeof(struct millet_data);

	if (!TYPE_VALID(type)) {
		printf("MILLET_USER: create recv loop type invalid %d\n",
			type);
		return MILLET_ERR;
	}

	data.msg_type  = LOOPBACK_MSG;
	data.owner     = type;

	printf("MILLET_USER: send loop back msg now\n");
	if (millet_sendmsg(sock_fd, type, &data) != MILLET_OK) {
		printf("MILLET_USER: send loop_back msg failed\n");
		return MILLET_ERR;
	}

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(msglen));
	if (!nlh) {
		printf("MILLET_USER: millet_recv_loop malloc failed!\n");
		goto out_loop;
	}

	memset(nlh, 0, NLMSG_SPACE(msglen));
	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(msglen);
	memset(&dst_addr, 0, sizeof(dst_addr));
	dst_addr.nl_family = AF_NETLINK;
	dst_addr.nl_pid = 0;
	dst_addr.nl_groups = 0;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&dst_addr;
	msg.msg_namelen = sizeof(dst_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	printf("NLMSG_PAYLOAD(nlh, 0) %d\n", NLMSG_PAYLOAD(nlh, 0));

	while (1) {
		ret = recvmsg(sock_fd->sock, &msg, 0);
		printf("recv msg from kernel\n");
		if (ret < 0) {
			printf("MILLET_USER: socket err\n");
			goto out_loop;
		}

		handle_msg((struct millet_data *) NLMSG_DATA(nlh), recv_hook);
	}

out_loop:
	free(nlh);
	return MILLET_ERR;
}
