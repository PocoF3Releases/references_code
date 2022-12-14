#ifndef _F2FS_RANGE_H_
#define _F2FS_RANGE_H_
#include "fsck.h"

#define FAILED -1
#define SUCCESS 0
#define NO_NEED 1

#define DEBUG 0

struct file_zone_entry {
	struct list_head list;
	u64 start_blkaddr;
	u64 range;
};
int dump_file_range(u32 nid, struct list_head *head, int force);
int f2fs_init_get_file_zones(char *dev_path);
void destory_file_zones(struct list_head *head);
void f2fs_exit_get_file_zones();
extern int defrag_file(u64 start, u64 len, char *file_path);
#if DEBUG
void print_file_zones(struct list_head *head);
#endif
#endif
