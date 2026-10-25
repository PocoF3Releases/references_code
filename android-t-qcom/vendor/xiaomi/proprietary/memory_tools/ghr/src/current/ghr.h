#ifndef _GHR_H_
#define _GHR_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<fcntl.h>
#include <string.h>

typedef  unsigned char u8;
typedef  unsigned short u16;
typedef  unsigned int u32;
typedef  unsigned long u64;

#define GSORT_PARTITION "/dev/block/by-name/gsort"
#define HRNODE_EMMC "/d/mmc0/mmc0:0001/hr"
#define HRNODE_EMMC_R "/sys/devices/virtual/mi_memory/mi_memory/mmc_info/hr"
#define HRNODE_EMMC_C3L_R "/sys/devices/virtual/mi_memory/mi_memory_device/emmc0/hr"
#define HRNODE_EMMC_C3L_Q "/sys/kernel/debug/mmc0/mmc0:0001/hr"
#define HRNODE "/sys/class/block/sda/device/hr"
#define HRNODE_R "/sys/class/mi_memory/mi_memory_device/ufshcd0/hr"
#define SERIAL_NODE "/sys/kernel/debug/1d84000.ufshc/dump_string_desc_serial"
#define SERIAL_NODE_R "/sys/class/mi_memory/mi_memory_device/ufshcd0/dump_string_desc_serial"
#define HEALTH_NODE "/sys/kernel/debug/1d84000.ufshc/dump_health_desc"
#define HEALTH_NODE_R "/sys/class/mi_memory/mi_memory_device/ufshcd0/dump_health_desc"
#define DEVICE_NODE "/sys/kernel/debug/1d84000.ufshc/dump_device_desc"
#define DEVICE_NODE_R "/sys/class/mi_memory/mi_memory_device/ufshcd0/dump_device_desc"
#define MI_IC_PATH "/data/vendor/mi_ic"

#define CID_MANFID_SANDISK    0x45
#define CID_MANFID_TOSHIBA    0x11
#define CID_MANFID_MICRON    0x13
#define CID_MANFID_SAMSUNG    0x15
#define CID_MANFID_HYNIX    0x90
#define CID_MANFID_NUMONYX_MICRON 0xfe
#define CID_MANFID_KINGSTON    0x70
#define CID_MANFID_YMTC    0x00009b
#define CID_MANFID_HOSIN    0x0000d6

#define MAX_RTBB 3

struct desc_field_offset {
	char *name;
	int offset;
	int size;
};

char output_hr[2048];

char tmp[128];

int get_hr(char *patch, char *hr, int len);

void  parse_hr(char *hr, struct desc_field_offset *vendor_desc, int item_count, int little_endian);

void get_nand_density();

void parse_createfile(char *file);

#endif /*_GHR_H_*/
