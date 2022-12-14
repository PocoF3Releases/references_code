#ifndef _FWCHECK_H_
#define _FWCHECK_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

typedef  unsigned char uint8;
typedef  unsigned short uint16;
typedef  unsigned int uint32;
typedef  unsigned long uint64;

#define UFS_VENDOR_PATH	"/sys/class/block/sda/device/vendor"
#define UFS_PN_PATH	"/sys/class/block/sda/device/model"
#define UFS_FW_PATH	"/sys/class/block/sda/device/rev"

#define eMMC_VENDOR_PATH	"/sys/class/block/mmcblk0/device/manfid"
#define eMMC_PN_PATH	"/sys/class/block/mmcblk0/device/name"
#define eMMC_FW_PATH	"/sys/class/block/mmcblk0/device/fwrev"

#define eMMC_HR_PATH	"/d/mmc0/mmc0:0001/hr"
#define UFS_HR_PATH	"/sys/class/block/sda/device/hr"
#define UFS_HR_R_PATH	"/sys/class/mi_memory/mi_memory_device/ufshcd0/hr"

#define CHECKLIST_PATH_OLD	"/dev/block/bootdevice/by-name/bk04"
#define CHECKLIST_PATH_NEW	"/dev/block/bootdevice/by-name/mem"

#define	MAX_LINE	128
#define	VENDOR_SIZE	20
#define	PN_SIZE	30
#define	FW_SIZE	30

#define END_OF_TXT "#END#"

#pragma pack(1)
typedef struct fw_struct {
	char vendor[VENDOR_SIZE];
	char pn[PN_SIZE];
	char fw[FW_SIZE];
} fw_t;
#pragma pack()
int fw_match(fw_t *checklist, fw_t *cur_fw, int item_num);
int get_dev_fw(fw_t *cur_fw);
void rm_space(char *str, int len);
int get_items_num ();
int spilt_checklist_info(fw_t *fw_checklist, char *buf);
int get_checklist_info(fw_t *fw_checklist);
#define LOG_TO_FILE(filename)				\
{								\
	FILE *fn = NULL;					\
	fn = freopen(filename, "w+", stdout);	\
	if (!fn) {						\
		printf("freopen fail\n");		\
	}								\
}
#define LOG_TO_STDOUT()				\
{								\
	FILE *fn = NULL;					\
	fn = freopen("/dev/tty", "w", stdout);	\
	if (!fn) {						\
		printf("freopen fail\n");		\
	}								\
}
struct product_use_old_path {
	char *product_name;
	uint8 len;
};
#endif /*_FWCHECK_H_*/

