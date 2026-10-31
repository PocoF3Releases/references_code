
#ifndef _F2FS_DEPEND_H_
#define _F2FS_DEPEND_H_
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sstream>
#include <assert.h>
using namespace std;
#define EXIT_ERR_CODE		(-1)
#define ver_after(a, b) (typecheck(unsigned long long, a) &&            \
		typecheck(unsigned long long, b) &&                     \
		((long long)((a) - (b)) > 0))
#define offsetof_(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member) * __mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof_(type, member)); })
#define FAILED -1
#define SUCCESS 0
#define NO_NEED 1
#define NULL_ADDR 0x0U
#define NEW_ADDR -1U

#define UFSFBO_DEBUG 1

#if UFSFBO_DEBUG
#define FBO_NATIVE_INFO_MSG(msg, args...) do { \
        ALOGI("%s:%d: " msg, __func__, __LINE__, ##args); \
    } while(0)
#else
#define FBO_NATIVE_INFO_MSG(msg, args...) do { } while(0)
#endif

#define FBO_NATIVE_DBG_MSG(msg, args...) ALOGD("%s:%d: " msg, __func__, __LINE__, ##args)

#define FBO_NATIVE_ERR_MSG(msg, args...) ALOGE("%s:%d: " msg, __func__, __LINE__, ##args)

typedef  unsigned int u32;
typedef  unsigned long long u64;
struct list_head {
	struct list_head *next, *prev;
};
struct file_info
{
    string file_path;
    string file_inode;
    string file_size;
    file_info(string path, string inode, string size) : file_path(path), file_inode(inode), file_size(size) {}
};
struct file_zone_entry {
	struct list_head list;
	u64 start_blkaddr;
	u64 range;
};

static inline void __list_add(struct list_head *new_list,
				struct list_head *prev,
				struct list_head *next)
{
	next->prev = new_list;
	new_list->next = next;
	new_list->prev = prev;
	prev->next = new_list;
}
static inline void __list_del(struct list_head * prev, struct list_head * next)
{
	next->prev = prev;
	prev->next = next;
}
static inline void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}
static inline void list_add_tail(struct list_head *new_list, struct list_head *head)
{
	__list_add(new_list, head->prev, head);
}
#define LIST_HEAD_INIT(name) { &(name), &(name) }
#define list_entry(ptr, type, member)					\
		container_of(ptr, type, member)
#define list_first_entry(ptr, type, member)				\
		list_entry((ptr)->next, type, member)
#define list_next_entry(pos, member)					\
		list_entry((pos)->member.next, typeof(*(pos)), member)
#define list_for_each_entry(pos, head, member)				\
	for (pos = list_first_entry(head, typeof(*pos), member);	\
		&pos->member != (head);					\
		pos = list_next_entry(pos, member))
#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_first_entry(head, typeof(*pos), member),	\
		n = list_next_entry(pos, member);			\
		&pos->member != (head);					\
		pos = n, n = list_next_entry(n, member))
#endif
