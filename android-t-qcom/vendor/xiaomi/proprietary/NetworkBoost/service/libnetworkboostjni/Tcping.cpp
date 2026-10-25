#define LOG_TAG "NetworkAccelerateSwitch-Tcping"

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <poll.h>
#include <signal.h>
#include <cutils/log.h>

#include "Tcping.h"


struct timespec evConsTime(time_t sec, long nsec) {
    struct timespec x;

    x.tv_sec = sec;
    x.tv_nsec = nsec;
    return (x);
}

struct timespec evNowTime(void) {
    struct timespec tsnow;
    clock_gettime(CLOCK_REALTIME, &tsnow);
    return tsnow;
}

struct timespec evAddTime(struct timespec addend1, struct timespec addend2) {
    struct timespec x;

    x.tv_sec = addend1.tv_sec + addend2.tv_sec;
    x.tv_nsec = addend1.tv_nsec + addend2.tv_nsec;
    if (x.tv_nsec >= BILLION) {
        x.tv_sec++;
        x.tv_nsec -= BILLION;
    }
    return (x);
}

struct timespec evSubTime(struct timespec minuend, struct timespec subtrahend) {
    struct timespec x;

    x.tv_sec = minuend.tv_sec - subtrahend.tv_sec;
    if (minuend.tv_nsec >= subtrahend.tv_nsec)
        x.tv_nsec = minuend.tv_nsec - subtrahend.tv_nsec;
    else {
        x.tv_nsec = BILLION - subtrahend.tv_nsec + minuend.tv_nsec;
        x.tv_sec--;
    }
    return (x);
}

int evCmpTime(struct timespec a, struct timespec b) {
#define SGN(x) ((x) < 0 ? (-1) : (x) > 0 ? (1) : (0));
    time_t s = a.tv_sec - b.tv_sec;
    long n;

    if (s != 0) return SGN(s);

    n = a.tv_nsec - b.tv_nsec;
    return SGN(n);
}

int retrying_poll(const int sock, const short events, const struct timespec* finish) {
    struct timespec now, timeout;

retry:
    ALOGI(" %s : %d retrying_poll", __func__, sock);

    now = evNowTime();
    if (evCmpTime(*finish, now) > 0)
        timeout = evSubTime(*finish, now);
    else
        timeout = evConsTime(0L, 0L);
    struct pollfd fds = {.fd = sock, .events = events};
    int n = ppoll(&fds, 1, &timeout, /*__mask=*/NULL);
    if (n == 0) {
        ALOGE(" %s : %d retrying_poll timeout", __func__, sock);
        errno = ETIMEDOUT;
        return 0;
    }
    if (n < 0) {
        if (errno == EINTR) goto retry;
        ALOGE(" %s : %d retrying_poll failed", __func__, sock);
        return n;
    }
    if (fds.revents & (POLLIN | POLLOUT | POLLERR)) {
        int error;
        socklen_t len = sizeof(error);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error) {
            errno = error;
            ALOGE(" %s : %d retrying_poll getsockopt failed", __func__, sock);
            return -1;
        }
    }
    ALOGI(" %s : %d retrying_poll returning %d", __func__, sock, n);
    return n;
}

int connect_with_timeout(int sock, const sockaddr* nsap, socklen_t salen,
                                const timespec timeout) {
    int res, origflags;

    origflags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, origflags | O_NONBLOCK);

    res = connect(sock, nsap, salen);
    if (res < 0 && errno != EINPROGRESS) {
        res = -1;
        goto done;
    }
    if (res != 0) {
        timespec now = evNowTime();
        timespec finish = evAddTime(now, timeout);
        ALOGI(" %s : %d ", __func__, sock);
        res = retrying_poll(sock, POLLIN | POLLOUT, &finish);
        if (res <= 0) {
            res = -1;
        }
    }
done:
    fcntl(sock, F_SETFL, origflags);
    ALOGD(" %s : %d connect_with_const timeout returning %d", __func__, sock, res);
    return res;
}

bool setInterface(int sock, const char *device) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);
    ioctl(sock,SIOCGIFINDEX, &ifr);
    return setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,
        (void *)&ifr, sizeof(ifr)) >= 0;
}

bool setSocketTimeout(int sock) {
    struct timeval timeout;
    int ret = 0;

    // ALOGE("setSocketTimeout TIME_OUT is 1s!\n");
    timeout.tv_sec = TIME_OUT_MS / 1000;
    timeout.tv_usec = (TIME_OUT_MS % 1000) * 1000;

    socklen_t len = sizeof(timeout);
    ret = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
        &timeout, len);
    ret = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
        &timeout, len);
    return (ret >= 0);
}

/* Generate random content */
int genRandomString(int length,char* ouput)
{
    int flag, i;
    struct timeval tpstart;
    gettimeofday(&tpstart,NULL);
    srand(tpstart.tv_usec);
    for (i = 0; i < length - 2; i++)
    {
        flag = rand() % 3;
        switch (flag)
        {
        case 0:
            ouput[i] = 'A' + rand() % 26;
            break;
        case 1:
            ouput[i] = 'a' + rand() % 26;
            break;
        case 2:
            ouput[i] = '0' + rand() % 10;
            break;
        default:
            ouput[i] = 'x';
            break;
        }
    }
    ouput[length-1] = '\0';
    return 0;
}

int getInterfaceTcpingRtt(Ping_info *ping_info) {
    int result = -1;
    int tcping_rtt = 0;
    char tcping_buff[BUFSIZE];
    int sock_fd = 0;
    struct sockaddr_in dest_addr;
    tcp_info tcpinfo;
    socklen_t tcp_info_length = sizeof tcpinfo;

    sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

    if (sock_fd < 0) {
        ALOGE(" create socket failed of client!\n");
        goto err;
    }

    bzero(&dest_addr,sizeof(struct sockaddr_in));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(ping_info->server);
    dest_addr.sin_port = htons(ping_info->port);

    if (!setSocketTimeout(sock_fd)) {
        ALOGE(" Set SocketTimeout failed.");
        goto socket_err;
    }

    if (ping_info->device != NULL) {
        if (!setInterface(sock_fd, ping_info->device)) {
            ALOGE("Set interface %s failed.", ping_info->device);
            goto socket_err;
        }
    }

    struct timespec timeout;
    timeout.tv_sec = TIME_OUT_MS / 1000;
    timeout.tv_nsec = (TIME_OUT_MS % 1000) * 1000000;
    result = connect_with_timeout(sock_fd, (struct sockaddr*)&dest_addr,
                (socklen_t)sizeof(struct sockaddr_in), timeout);

    if (result < 0) {
        ALOGE(" connect failed of client,device is %s!\n", ping_info->device);
        goto socket_err;
    }

    for (int i = 0; i < 5; i++) {
        memset(tcping_buff, 0, BUFSIZE);
        genRandomString(BUFSIZE, tcping_buff);
        result = send(sock_fd, tcping_buff, strlen(tcping_buff), MSG_NOSIGNAL);
        if (result < 0) {
            ALOGE(" send data to server request failed %d,device is %s(errno %s,"
                        " errno count %d)\n", result, ping_info->device, strerror(errno), i);
            if (i == 0) {
                goto socket_err;
            }
            break;
        }

        memset(tcping_buff, 0, BUFSIZE);
        result = recv(sock_fd, tcping_buff, BUFSIZE, MSG_WAITALL | MSG_NOSIGNAL);
        if (result < 0) {
            ALOGE("recieve failed %d,device is %s(errno %s,"
                        " errno count %d)", result, ping_info->device,  strerror(errno), i);
            if (i == 0) {
                goto socket_err;;
            }
            break;
        }

    }

    ALOGD(" receive succeccd is result:%d,device is %s", result, ping_info->device);

    if (-1 == getsockopt(sock_fd, SOL_TCP, TCP_INFO, &tcpinfo, &tcp_info_length) ) {
        goto socket_err;
    }

    tcping_rtt = (tcpinfo.tcpi_rtt)/1000;
    close(sock_fd);
    ALOGI(" TCPing cacla rtt is %d,device is %s", tcping_rtt, ping_info->device);
    return tcping_rtt;

socket_err:
    close(sock_fd);
err:
    /* connnect or send timeout */
    return 500;
}