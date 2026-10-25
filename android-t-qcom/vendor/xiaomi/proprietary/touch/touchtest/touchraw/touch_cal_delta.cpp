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
//#include <utils/Errors.h>
//#include <android-base/stringprintf.h>
//#include <android-base/strings.h>
#include <sys/time.h>
static const int pos_TX[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const int pos_RX[8] = {0, -1, -1, -1, 0, 1, 1, 1};

//using namespace android;
double tick(void)
{
	struct timeval t;
	gettimeofday(&t, 0);
	return t.tv_sec + 1E-6 * t.tv_usec;
}

void show_raw(int *data, int RX, int TX, bool show_label)
{
	int i, j;

	if (show_label) {
		printf("    ");
		for (j = 0; j < TX; j++)
				printf("TX%-3d",  j);
		printf("\n");
	}
	for (i = 0; i < RX; i++) {
		if (show_label) {
			printf("RX%-3d",  i);
		}
		for (j = 0; j < TX; j++) {
				printf("%-5d", data[i * TX + j]);
		}
		printf("\n");
	}
	printf("\n");
}

static int filter_noise(int *data, int noise)
{
	int i, j, m;
	int size_cnt = 0;
	int size = 0;

	for (i = 0; i < RX; i++) {
		for (j = 0; j < TX; j++) {
			if (data[i * TX + j] < noise) {
				bool set_zero = true;
				for (m = 0; m < (int)(sizeof(pos_TX)/sizeof(int)); m++) {
					int new_TX = j + pos_TX[m];
					int new_RX = i + pos_RX[m];
					if ((new_TX >= 0 && new_TX < TX) && (new_RX >= 0 && new_RX < RX) && data[new_RX * TX + new_TX] >= noise) {
						set_zero = false;
						break;
					}

				}
				if (set_zero || data[i * TX + j] < 0)
					data[i * TX + j] = 0;
			}
			if (data[i * TX + j] == 0)
				continue;
			size++;
		}
	}
/*
	for (i = 0; i < RX; i++) {
		for (j = 0; j < TX; j++) {
			if (data[i * TX + j] == 0)
				continue;
			size_cnt = 0;
			for (m = 0; m < (int)(sizeof(pos_TX)/sizeof(int)); m++) {
				int new_TX = j + pos_TX[m];
				int new_RX = i + pos_RX[m];
				if ((new_TX >= 0 && new_TX < TX) && (new_RX >= 0 && new_RX < RX) && data[new_RX * TX + new_TX] > 0) {
					size_cnt++;
				}
			}
			if (size_cnt < SINGLE_POINT) {
				data[i * TX + j] = 0;
				size--;
			}
		}
	}*/
    return size;
}

int main(int argc, char* argv[])
{
	struct pollfd fds[2];
	int fd_poll;
	int fd_suspend;
	int fd_map;
	int fd_delta;
	int fd_rx_num;
	int fd_tx_num;
	char temp_buf[10];
	short *buf;
	void *mem_base;
	static bool is_suspend = false;
	static bool pre_suspend = true;
	char ready_baseline = false;
	char ready_delta = false;
	int delta_threshold  = 200;
	int delta_up_threshold = 60;
	int slow_cal_frame = 120;
	int min_delta = 0;
	int min_base = 0;
	int max_base = 0;
	int negtive_delta = -300;
	int max_delta = 0;
	int baseline_frame_cnt = 0;
	int TX = 0;
	int RX = 0;
	int skip_cnt = 10;
	int i, j, m;

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

	fd_poll = open("/sys/class/touch/touch_dev/update_rawdata",   O_RDWR);
	if (fd_poll < 0) {
		//printf("can't open update_rawdata\n");
		printf("Can't open update_rawdata:%d\n", fd_poll);
		return 0;
	}

	fd_delta = open("/sys/class/touch/touch_dev/enable_touch_delta",   O_WRONLY);
	if (fd_delta < 0) {
		//printf("can't open update_rawdata\n");
		printf("Can't open fd_delta:%d\n", fd_delta);
		return -1;
	}
	write(fd_delta, "0", 2);
	close(fd_delta);


	fd_map = open("/dev/xiaomi-touch", O_RDWR);
	if (fd_map < 0) {
		//printf("can't open xiaomi-touch\n");
		printf("can't open xiaomi-touchs\n");
		return 0;
	}


	fd_suspend = open("/sys/class/touch/touch_dev/suspend_state", O_RDWR);
	if (fd_suspend < 0) {
		//printf("can't open xiaomi-touch\n");
		printf("can't open suspend_state\n");
		return 0;
	}

	mem_base = mmap(0, TX * RX,  PROT_READ | PROT_WRITE, MAP_SHARED, fd_map, 0);
	if (mem_base == MAP_FAILED) {
		//printf("map failed\n");
		printf("MAP failed\n");
		return 0;
	}

	memset(fds, 0x00, sizeof(fds));
	fds[0].fd = fd_poll;
    fds[0].events = POLLPRI | POLLERR;
	fds[1].fd = fd_suspend;
	fds[1].events = POLLPRI | POLLERR;
	int delta[TX * RX];
	int base[TX * RX];
	while(1) {
		poll(fds, 2 , -1);
		//memset(buf,  0,  8192 * sizeof(char));
		memset(temp_buf, 0x00, sizeof(temp_buf));
		if(fds[1].revents & (POLLPRI | POLLIN)) {
			pread(fd_suspend, temp_buf, 2, 0);
			if (temp_buf[0] == '1') {
				is_suspend = true;
				pre_suspend = false;
			} else {
				is_suspend = false;
				pre_suspend = true;
				ready_baseline = false;
				skip_cnt = 20;
			}
		}
        if(fds[0].revents & (POLLPRI | POLLIN)) {
			pread(fd_poll, temp_buf, 2, 0);
			if (skip_cnt) {
				skip_cnt--;
				continue;
			}
			buf = (short *)mem_base;
			ready_delta = false;
            if (!ready_baseline) {
	            min_base = 1000;
            	max_base = -300;
                for (i = 0; i < TX * RX; i++) {
                    base[i] = (int)buf[i];
                    min_base = min_base > buf[i] ? buf[i] : min_base;
                    max_base = max_base > buf[i] ? max_base : buf[i];
                }
                ready_baseline = true;
                continue;
            }
            if (ready_baseline) {
                min_delta = 1000;
                max_delta = -300;
				for (i = 0; i < RX * TX; i++) {
                    delta[i] =  float(base[i] - buf[i]);
                    if (min_delta > delta[i])
                        min_delta = delta[i];
                    if (max_delta < delta[i])
                        max_delta = delta[i];
				}
				ready_delta = true;
            }
			if ((min_delta < negtive_delta && max_delta < delta_threshold) ||
				max_delta < delta_threshold) {
				ready_delta = false;
                for (i = 0; i < TX * RX; i++) {
                    base[i] = buf[i];
                }
                ready_delta = true;
			}
			filter_noise(delta, 200);

			printf("min_base:%d, max_base:%d, min_delta:%d, max_delta:%d\n", min_base, max_base, min_delta, max_delta);
			printf("start\n");
			show_raw(delta, RX, TX, false);
        }
    }
}
