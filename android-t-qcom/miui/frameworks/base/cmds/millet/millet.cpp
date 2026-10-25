#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <errno.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dlfcn.h>

#include "milletApi.h"
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include <android-base/logging.h>

#define LIB_FULL_NAME		"libmillet_comm.so"
#define MONITOR_PRI_ADJUST	-10
#define RETRY_COUNT		3
#define RETRY_INTERVAL		3
#define DIE_WAKE		(~0U)

typedef int (*fp_create_millet_sock)(millet_sock *sock_fd);
typedef int (*fp_destroy_millet_sock)(millet_sock *sock_fd);
typedef int (*fp_millet_recv_loop)(millet_sock *sock_fd, enum MILLET_TYPE type, millet_callback hook);

static fp_create_millet_sock create_sock;
static fp_destroy_millet_sock destroy_sock;
static fp_millet_recv_loop recv_loop;
static millet_sock recv_sock;
static millet_callback millet_recvhook[MILLET_TYPES_NUM][POLICY_TYPE_NUM];

extern void monitor_init(void);
extern void register_callback(enum MILLET_TYPE type);

int register_mod_hook(millet_callback hook, enum MILLET_TYPE type,
		POLICY_TYPE policy)
{
	if (!TYPE_VALID(type))
		return MILLET_ERR;

	if (!POLICY_VALID(policy))
		return MILLET_ERR;

	printf("MILLET_USER: mod %s register recv hook now\n",
		NAME_ARRAY[type]);
	millet_recvhook[type][policy] = hook;
	return MILLET_OK;
}

static void monitor_transport(const struct millet_data* data)
{
	int policy, type = data->owner;

	if (!data)
		return;

	for (policy = O_POLICY; policy < POLICY_TYPE_NUM; policy ++) {
		if (millet_recvhook[type][policy])
			millet_recvhook[type][policy](data);
	}
}

static enum MILLET_TYPE find_modtype_by_name(const char *str)
{
	int i;

	for (i = O_TYPE; i < MILLET_TYPES_NUM; i++)
		if (!strcmp(NAME_ARRAY[i], str))
			break;

	return (enum MILLET_TYPE)i;
}

int main(int argc, char* argv[])
{
	enum MILLET_TYPE type;
	const char* name;
	int policy, i, pri, cycle = 0;
	void *handle = NULL;

	printf("MILLET_USER: main init\n");
	char **logid = argv;
	android::base::InitLogging(logid,&android::base::KernelLogger);


	LOG(INFO) << "millet start!";

	if(argc != 2) {
		int i;
		printf("MILLET_USER: valid input arg\n");
		for (i = 0; i < argc; i++) {
			printf("%s\t", argv[i]);
		}
		printf("\n");
		return MILLET_ERR;
	}

	for (policy = O_POLICY; policy < POLICY_TYPE_NUM; policy++) {
		for (i = O_TYPE; i < MILLET_TYPES_NUM; i++)
			millet_recvhook[i][policy] = NULL;
}

	monitor_init();
	name = argv[1];
	type = find_modtype_by_name(name);
	if (!TYPE_VALID(type)) {
		printf("MILLET_USER: valid type %s\n", argv[1]);
		return MILLET_ERR;
	}

	handle = dlopen(LIB_FULL_NAME, RTLD_NOW);
	if (handle == NULL) {
		printf("Can't find %s", LIB_FULL_NAME);
		return MILLET_ERR;
	}

	create_sock  =
		(fp_create_millet_sock) dlsym(handle, "create_millet_sock");
	recv_loop    =
		(fp_millet_recv_loop) dlsym(handle, "millet_recv_loop");
	destroy_sock =
		(fp_destroy_millet_sock) dlsym(handle, "destroy_millet_sock");

	if (!create_sock || !recv_loop || !destroy_sock) {
		printf("Can't find millet comm func\n");
		return MILLET_ERR;
	}

	while (create_sock(&recv_sock) == MILLET_ERR) {
		printf("MILLET_USER: create listen sock failed %s\n",
				strerror(errno));
		if (++cycle > RETRY_COUNT)
			goto out;

		sleep(RETRY_INTERVAL);
	}
	android::ProcessState::self()->startThreadPool();

	pri = nice(MONITOR_PRI_ADJUST);
	printf("type is %d pri %d\n", type, pri);
	if ((recv_loop(&recv_sock, type, monitor_transport))== MILLET_ERR) {
                printf("MILLET:recv_loop failed\n");
		goto out;
        }
	destroy_sock(&recv_sock);
	return MILLET_OK;
out:
	while(1) {
		/*do nothing*/
		sleep(DIE_WAKE);
	}
}
