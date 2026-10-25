#ifndef _BOOST_H_
#define _BOOST_H_

#define LOG_TAG "LB"

#define MAG_NUM			'T'
#define TRIGGER_GAME_BOOST	_IO(MAG_NUM, 1)
#define TRIGGER_GAME_VSYNC      _IO(MAG_NUM, 2)
#define TRIGGER_GAME_DEQUEUE	_IO(MAG_NUM, 3)
#define TRIGGER_GAME_CEILING	_IO(MAG_NUM, 4)

#define PATH_PERF_IOCTL "/dev/migt"
#define CONFIG_PATH "/data/system/migt/migt"
#define PATH_MINOR_WINDOW "/sys/module/metis/parameters/minor_window_app"
#define SUPPORT 1
#define NOT_SUPPORT 2
#define UNKNOWN 3

#define DECRYPT_MAGIC_BASE 117

#define BUF_SIZE 256

enum param_index {
    PACKAGE_INDEX = 0,
    BOOST_INDEX,
    DUR_INDEX,
    ALL_INDEX,
};

// int xgfNotifyQueue(__u32);

#endif
