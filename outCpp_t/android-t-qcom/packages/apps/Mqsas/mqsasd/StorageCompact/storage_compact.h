/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _STORAGE_COMPACT_H_
#define _STORAGE_COMPACT_H_

/*
 * In the current implementation, there are two policies used to find group to defrag.
 * GREEDY is based on greedy algorithm and will choose block group which has more free
 * blocks and also more fragments to defrag.
 * LINEAR will search block group linearly from start to end.
 */
enum {
    GREEDY = 0,
    LINEAR,
    MAX_POLICY,
};

extern "C" int storage_compact(ext2_filsys current_fs, int fd, unsigned int first_group_no,
                                unsigned int end_group_no, unsigned int max_compact_groups, int policy);

#endif /* _STORAGE_COMPACT_H_ */
