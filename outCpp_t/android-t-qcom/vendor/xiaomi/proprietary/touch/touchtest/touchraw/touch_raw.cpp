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
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <ctype.h>
#include <getopt.h>
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


typedef unsigned int   uint32_t;
static const int pos_TX[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const int pos_RX[8] = {0, -1, -1, -1, 0, 1, 1, 1};


//using namespace android;
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
        printf("      ");
        for (j = 0; j < tx; j++)
                printf("TX%-4d",  j);
        printf("\n");
    }
    for (i = 0; i < rx; i++) {
        if (show_label) {
            printf("RX%-2d",  i);
        }
        for (j = 0; j < tx; j++) {
            if (j < tx - 1)
                printf("%5d,", data[i * tx + j]);
            else
                printf("%5d", data[i * tx + j]);
        }
        printf("\n");
    }
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
    void *mem_base;
    int TX = 0;
    int RX = 0;
    int skip_cnt = 10;
    int i, j, m;
    int map_size;


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

    fd_poll = open("/sys/class/touch/touch_dev/touch_thp_dump",   O_RDWR);
    if (fd_poll < 0) {
        //printf("can't open update_rawdata\n");
        printf("Can't open update_rawdata:%d\n", fd_poll);
        return 0;
    }
    write(fd_poll, "2", 2);

    fd_map = open("/dev/xiaomi-touch", O_RDWR);
    if (fd_map < 0) {
        //printf("can't open xiaomi-touch\n");
        printf("can't open xiaomi-touchs\n");
        return 0;
    }

    map_size = 11 * 4096;

    mem_base = mmap(0, map_size,  PROT_READ | PROT_WRITE, MAP_SHARED, fd_map, 8192);
    if (mem_base == MAP_FAILED) {
        //printf("map failed\n");
        printf("MAP failed\n");
        return 0;
    }
    int raw_buf[TX * RX * sizeof(int) * 4];
    memset(fds, 0x00, sizeof(fds));
    fds[0].fd = fd_poll;
    fds[0].events = POLLPRI | POLLERR;
    int delta[TX * RX];
    int delta_st[TX * RX];
    int base[3][TX * RX];
    int raw[TX * RX];
    while(1) {
        poll(fds, 1 , -1);
        //memset(buf,  0,  8192 * sizeof(char));
        memset(temp_buf, 0x00, sizeof(temp_buf));
        if(fds[0].revents & (POLLPRI | POLLIN)) {
            pread(fd_poll, temp_buf, 2, 0);
            if (temp_buf[0] == '5') {
                memcpy((char *)raw_buf, mem_base, TX * RX * 4 * sizeof(int));
                printf("start\n");
                show_raw(raw_buf + TX * RX * 3,  RX, TX, false);
            }
        }
    }
}
