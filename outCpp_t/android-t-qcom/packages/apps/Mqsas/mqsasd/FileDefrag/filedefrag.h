/*
 * Copyright (C) Xiaomi Inc.
 *
 */

#ifndef _FILE_DEFRAG_H_
#define _FILE_DEFRAG_H_

enum donor_call_defrag_estat {
    DFILE_PAGE_IN_CORE,
    DFILE_FREE_PAGES,
    DFILE_IOC_MOVE_EXT,
    DONOR_CALL_DEFRAG_ESTAT_MAX,
};

enum defrag_stat {
    FILE_REG,
    FILE_SIZE_ZERO,
    FILE_BLOCKS_ZERO,
    FILE_OPEN_FAIL,
    FILE_GET_EXT,
    FILE_CHG_PHY_TO_LOG,
    FILE_CHECK,
    FILE_SYNC,
    DONOR_FILE_JOIN_EXT,
    DONOR_FILE_OPEN,
    DONOR_FILE_UNLINK,
    DONOR_FILE_FALLOCATE,
    DONOR_FILE_GET_EXT,
    DONOR_FILE_CHG_PHY_TO_LOG,
    DONOR_DEFRAG_OK,
    DONOR_NO_DEFRAG_OK,
    DONOR_DEFRAG_FAIL,
    DEFRAG_STAT_MAX,
};

extern long g_defrag_stat[DEFRAG_STAT_MAX];
extern long g_donor_call_defrag_estat[DONOR_CALL_DEFRAG_ESTAT_MAX];
extern int extents_before_defrag;
extern int extents_after_defrag;
extern unsigned int defraged_file_count;
extern __u8 log_groups_per_flex;
extern __u32 blocks_per_group;
extern __u32 feature_incompat;

extern void defrag_dir(char *dir);
extern void defrag_file(char *file);
extern void defragdata_output();

#endif /* _FILE_DEFRAG_H_ */
