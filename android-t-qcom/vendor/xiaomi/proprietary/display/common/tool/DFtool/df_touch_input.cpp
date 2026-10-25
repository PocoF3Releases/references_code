#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <asm/types.h>

#define BITS_PER_LONG  (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(x)  (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define test_bit(bit, array)   ((array)[(bit)/BITS_PER_LONG] & (1L << ((bit) % BITS_PER_LONG)))
static int xres = 0;
static int yres = 0;

char const*const RESOLUTION_FACTOR_FILE
        = "/sys/class/touch/touch_dev/resolution_factor";

static int getTouchEventFd()
{
	char name[256];
	char path[256];
	struct input_absinfo abs_x;
	struct input_absinfo abs_y;
	unsigned long absBitmask[BITS_TO_LONGS(ABS_MAX)];
	int ret, fd = -1;
	unsigned int i;

	for (i = 0; i < 32; i++) {
		snprintf(path, sizeof(path), "/dev/input/event%i", i);

		fd = open(path, O_RDWR);

		if (fd < 0 && errno == ENOENT)
			continue;
		else if (fd < 0) {
			printf("failed to open %s", path);
			break;
		}

		if(ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBitmask)), absBitmask) < 0) {
			close(fd);
			break;
		} else if(test_bit(ABS_MT_POSITION_X, absBitmask)
				&& test_bit(ABS_MT_POSITION_Y, absBitmask)) {
#if 0
			ret = ioctl(fd, EVIOCGNAME(sizeof(name)), name);
			if (ret < 0) {
				printf("ioctl(%s, EVIOCGNAME, ...) failed", path);
				close(fd);
				break;
			} else
				printf("device[%d] name: %s\n", i, name);
#endif
			if(ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_x) >= 0
           			&& ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs_y) >= 0) {
				xres = abs_x.maximum;
				yres = abs_y.maximum;
				return fd;
			} else {
				close(fd);
			}
		} else
			close(fd);
	}

	return -1;
}

void touch_up(int fd)
{
	struct input_event ev;

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_ABS;
	ev.code = ABS_MT_TRACKING_ID;
	gettimeofday(&ev.time, NULL);
	ev.value = -1;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("ABS_MT_TRACKING_ID error\n");
	}

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_KEY;
	ev.code = BTN_TOUCH;
	ev.value = 0;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("BTN_TOUCH error\n");
	}

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("EV_REPORT error\n");
	}
}

void touch_down(int fd, int dx1, int dy1)
{
	struct input_event ev;

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_KEY;
	ev.code = BTN_TOUCH;
	ev.value = 1;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("BTN_TOUCH error\n");
	}

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_ABS;
	ev.code = ABS_MT_POSITION_X;
	gettimeofday(&ev.time, NULL);
	ev.value = dx1;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("ABS_MT_POSITION_X error\n");
	}

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_ABS;
	ev.code = ABS_MT_POSITION_Y;
	gettimeofday(&ev.time, NULL);
	ev.value = dy1;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("ABS_MT_POSITION_Y error\n");
	}

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;
	if (write(fd, &ev, sizeof(struct input_event)) < 0) {
		printf("EV_REPORT error\n");
	}
}

static void touch_swipe(int tp_fd, int dx1, int dy1, int dx2, int dy2, int duration)
{
	struct input_event ev;
	struct timeval tv_down;
	struct timeval tv_now;
	long long elapsedTime, endTime, nowTime, downTime;
	int dx, dy;

	if (tp_fd <= 0)
		return;

	gettimeofday(&tv_down, NULL);
	downTime = (tv_down.tv_sec * 1000 + tv_down.tv_usec/1000);

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_ABS;
	ev.code = ABS_MT_TRACKING_ID;
	gettimeofday(&ev.time, NULL);
	ev.value = 1;
	if (write(tp_fd, &ev, sizeof(struct input_event)) < 0)
		printf("write ABS_MT_TRACKING_ID error\n");

	endTime = duration + downTime;
	gettimeofday(&tv_now, NULL);
	nowTime = (tv_now.tv_sec * 1000 + tv_now.tv_usec/1000);

	/* do swipe */
	while (nowTime < endTime) {
		elapsedTime = nowTime - downTime;
		dx = dx1 + (dx2 - dx1) * elapsedTime/duration;
		dy = dy1 + (dy2 - dy1) * elapsedTime/duration;
		touch_down(tp_fd, dx, dy);
		gettimeofday(&tv_now, NULL);
		nowTime = (tv_now.tv_sec * 1000 + tv_now.tv_usec/1000);
	}

	touch_down(tp_fd, dx2, dy2);
	touch_up(tp_fd);

	return;
}

static void touch_tap(int tp_fd, int dx1, int dy1)
{
	struct input_event ev;

	if (tp_fd <= 0)
		return;

	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_ABS;
	ev.code = ABS_MT_TRACKING_ID;
	gettimeofday(&ev.time, NULL);
	ev.value = 1;
	if (write(tp_fd, &ev, sizeof(struct input_event)) < 0)
		printf("write ABS_MT_TRACKING_ID error\n");

	touch_down(tp_fd, dx1, dy1);
	touch_up(tp_fd);

	return;
}

static void showUsage()
{
	getTouchEventFd();
	printf("DFtool <command> [<arg>...]\n\n");

	printf("The commands are:\n");
	printf("      swipe\n");
	printf("      tap\n\n");

	printf("The commands:\n");
	printf("      swipe <x1> <y1> <x2> <y2> [duration(ms)]\n");
	printf("      tap <x> <y>\n\n");
	printf("          x-res: %d \n", xres);
	printf("          y-res: %d \n\n", yres);
}

static int get_resolution_factor(char const* path, int *resolution_factor)
{
	int rc = 0;
	int fd;
	int factor = 0;
	char buffer[10];

	if (!access(path, F_OK)) {
		fd = open(path, O_RDONLY);
		if (fd >= 0) {
			printf("%s: success to open %s\n", __FUNCTION__, path);
			ssize_t count = read(fd, buffer, (size_t)(sizeof(buffer)));
			factor = atoi(buffer);
			if (count <= 0 || factor <= 0) {
				printf("%s: error! read count (%d), resulution factor(%d)\n",
					__FUNCTION__, count, factor);
				rc = -1;
				goto exit;
			}
			if (factor > 0) {
				printf("%s: resulution_factor is %d\n",__FUNCTION__, factor);
				*resolution_factor = factor;
				goto exit;
			}
		} else {
			printf("%s: fail to open %s\n", __FUNCTION__, path);
			return -1;
		}
	} else {
		return -1;
	}

exit:
	close(fd);
	return rc;
}

int touchInput(int argc, char* argv[]) {

	int tp_fd = 0;
	int dx1, dy1, dx2, dy2, duration;
	int resolution_factor;

	if (argc <= 2) {
		showUsage();
		return -1;
	}

	tp_fd = getTouchEventFd();
	if (tp_fd <= 0) {
		printf("get touch fd num failed!\n");
		return -1;
	}

	if (get_resolution_factor(RESOLUTION_FACTOR_FILE, &resolution_factor) < 0) {
		resolution_factor = 1;
	}

	if (resolution_factor < 0) {
		printf("get_resolution_factor error\n");
		return -1;
	}

	if (!strncmp(argv[1], "swipe", sizeof("swipe")) && (argc >= 6)) {
		dx1 = atoi(argv[2]) * resolution_factor;
		dy1 = atoi(argv[3]) * resolution_factor;
		dx2 = atoi(argv[4]) * resolution_factor;
		dy2 = atoi(argv[5]) * resolution_factor;
		duration = (argc == 7) ? atoi(argv[6]) : 100;

		touch_swipe(tp_fd, dx1, dy1, dx2, dy2, duration);
	} else if (!strncmp(argv[1], "tap", sizeof("tap")) && (argc == 4)) {
		dx1 = atoi(argv[2]) * resolution_factor;
		dy1 = atoi(argv[3]) * resolution_factor;
		touch_tap(tp_fd, dx1, dy1);
	} else
		showUsage();

	close(tp_fd);
	return 0;
}
