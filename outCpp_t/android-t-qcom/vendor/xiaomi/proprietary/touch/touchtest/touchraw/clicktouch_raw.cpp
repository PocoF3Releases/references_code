#define LOG_TAG "TouchRaw"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
//#include <utils/Log.h>
//#include <utils/String8.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <time.h>
#include <queue>
#include <vector>
#include <inttypes.h>
#include <math.h>
#include <linux/input.h>
#include <sys/resource.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/inotify.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <sys/time.h>

double tick(void)
{
	struct timeval t;
	gettimeofday(&t, 0);
	return t.tv_sec + 1E-6 * t.tv_usec;
}

void show_raw(int *data, int rx, int tx, bool show_label)
{
	int i, j;

	if (show_label) {
		printf("    ");
		for (j = 0; j < tx; j++)
				printf("TX%-3d",  j);
		printf("\n");
	}

	for (i = 0; i < rx; i++) {
		if (show_label) {
			printf("RX%-3d",  i);
		}
		for (j = 0; j < tx; j++) {
				printf("%-5d", data[i * tx + j]);
		}
		printf("\n");
	}
	printf("\n");
}

int main(int argc, char* argv[])
{
	struct pollfd fds[2];
	int fd_poll;
	int fd_map;

	int fd_rx_num;
	int fd_tx_num;
	char temp_buf[10];
	short *buf;
	void *mem_base;
	int TX = 0;
	int RX = 0;

	int i, j, m;
	struct  timeval start ;
	int cnt = 0;
	int *delta = NULL;

	fd_poll = open("/sys/class/touch/touch_dev/clicktouch_raw",   O_RDWR);
	if (fd_poll < 0) {
		printf("Can't open update_rawdata:%d\n", fd_poll);
		return 0;
	}


	write(fd_poll, "10", 2);
	pread(fd_poll, temp_buf, 2, 0);

	fd_rx_num = open("/sys/class/touch/touch_dev/touch_thp_rx_num", O_RDONLY);
	if (fd_rx_num < 0) {
		printf("can't get rx num fd\n");
		return 0;
	}
	memset(temp_buf, 0x00, sizeof(temp_buf));
	pread(fd_rx_num, temp_buf, 2, 0);
	RX = atoi(temp_buf);

	fd_tx_num = open("/sys/class/touch/touch_dev/touch_thp_tx_num", O_RDONLY);
	if (fd_tx_num < 0) {
		printf("can't get tx num fd\n");
		return 0;
	}

	memset(temp_buf, 0x00, sizeof(temp_buf));
	pread(fd_tx_num, temp_buf, 2, 0);
	TX = atoi(temp_buf);

	//printf("tx:%d, rx:%d\n", TX, RX);
	delta = (int *)malloc(TX * RX * sizeof(int));

	fd_map = open("/dev/xiaomi-touch", O_RDWR);
	if (fd_map < 0) {
		printf("can't open xiaomi-touchs\n");
		return 0;
	}

	mem_base = mmap(0, TX * RX,  PROT_READ | PROT_WRITE, MAP_SHARED, fd_map, 0);
	if (mem_base == MAP_FAILED) {
		printf("MAP failed\n");
		return 0;
	}

	memset(fds, 0x00, sizeof(fds));
	fds[0].fd = fd_poll;
    fds[0].events = POLLPRI | POLLERR;
	double t;
	while(1) {
		poll(fds, 1 , -1);
		memset(temp_buf, 0x00, sizeof(temp_buf));
        if(fds[0].revents & (POLLPRI | POLLIN)) {
			pread(fd_poll, temp_buf, 2, 0);
			buf = (short *)mem_base;
            for (i = 0; i < TX * RX; i++) {
                delta[i] = (int)buf[i];
            }
            printf("start\n");
			show_raw(delta, RX, TX, false);
        }
    }
}
