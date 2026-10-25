#ifndef _REG_INTF_H_
#define _REG_INTF_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define REG_INTF_MAGIC 'M'
#define REG_INTF_PRIVATE 168

struct reg_intf_access_rawmem {
	unsigned int p;
	int width;
	unsigned int val;
};

#define REG_INTF_WRITE_MEM \
	_IOW(REG_INTF_MAGIC, REG_INTF_PRIVATE + 1, struct reg_intf_access_rawmem)
#define REG_INTF_READ_MEM \
	_IOWR(REG_INTF_MAGIC, REG_INTF_PRIVATE + 2, struct reg_intf_access_rawmem)

#endif /* _REG_INTF_H_ */
