/**
 * Copyright (c) 2015 The CheckFs Open Source Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _FS_EXT4_H_
#define _FS_EXT4_H_

/* This header generated from linux kernel header(fs/ext4/ext4.h) */

#include <sys/types.h>

/*
 * Structure of the super block
 */
struct ext4_super_block {
    /*00*/__le32 s_inodes_count;        /* Inodes count */
    __le32 s_blocks_count_lo;           /* Blocks count */
    __le32 s_r_blocks_count_lo;         /* Reserved blocks count */
    __le32 s_free_blocks_count_lo;      /* Free blocks count */
    /*10*/__le32 s_free_inodes_count;   /* Free inodes count */
    __le32 s_first_data_block;          /* First Data Block */
    __le32 s_log_block_size;            /* Block size */
    __le32 s_log_cluster_size;          /* Allocation cluster size */
    /*20*/__le32 s_blocks_per_group;    /* # Blocks per group */
    __le32 s_clusters_per_group;        /* # Clusters per group */
    __le32 s_inodes_per_group;          /* # Inodes per group */
    __le32 s_mtime;                     /* Mount time */
    /*30*/__le32 s_wtime;               /* Write time */
    __le16 s_mnt_count;                 /* Mount count */
    __le16 s_max_mnt_count;             /* Maximal mount count */
    __le16 s_magic;                     /* Magic signature */
    __le16 s_state;                     /* File system state */
    __le16 s_errors;                    /* Behaviour when detecting errors */
    __le16 s_minor_rev_level;           /* minor revision level */
    /*40*/__le32 s_lastcheck;           /* time of last check */
    __le32 s_checkinterval;             /* max. time between checks */
    __le32 s_creator_os;                /* OS */
    __le32 s_rev_level;                 /* Revision level */
    /*50*/__le16 s_def_resuid;          /* Default uid for reserved blocks */
    __le16 s_def_resgid;                /* Default gid for reserved blocks */
    /*
     * These fields are for EXT4_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    __le32 s_first_ino;                 /* First non-reserved inode */
    __le16 s_inode_size;                /* size of inode structure */
    __le16 s_block_group_nr;            /* block group # of this superblock */
    __le32 s_feature_compat;            /* compatible feature set */
    /*60*/__le32 s_feature_incompat;    /* incompatible feature set */
    __le32 s_feature_ro_compat;         /* readonly-compatible feature set */
    /*68*/__u8 s_uuid[16];              /* 128-bit uuid for volume */
    /*78*/char s_volume_name[16];       /* volume name */
    /*88*/char s_last_mounted[64];      /* directory where last mounted */
    /*C8*/__le32 s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT4_FEATURE_COMPAT_DIR_PREALLOC flag is on.
     */
    __u8 s_prealloc_blocks;        /* Nr of blocks to try to preallocate*/
    __u8 s_prealloc_dir_blocks;    /* Nr to preallocate for dirs */
    __le16 s_reserved_gdt_blocks;  /* Per group desc for online growth */
    /*
     * Journaling support valid if EXT4_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    /*D0*/__u8 s_journal_uuid[16]; /* uuid of journal superblock */
    /*E0*/__le32 s_journal_inum;   /* inode number of journal file */
    __le32 s_journal_dev;          /* device number of journal file */
    __le32 s_last_orphan;          /* start of list of inodes to delete */
    __le32 s_hash_seed[4];         /* HTREE hash seed */
    __u8   s_def_hash_version;     /* Default hash version to use */
    __u8   s_jnl_backup_type;
    __le16 s_desc_size;            /* size of group descriptor */
    /*100*/__le32 s_default_mount_opts;
    __le32 s_first_meta_bg;        /* First metablock block group */
    __le32 s_mkfs_time;            /* When the filesystem was created */
    __le32 s_jnl_blocks[17];       /* Backup of the journal inode */
    /* 64bit support valid if EXT4_FEATURE_COMPAT_64BIT */
    /*150*/__le32 s_blocks_count_hi;  /* Blocks count */
    __le32 s_r_blocks_count_hi;    /* Reserved blocks count */
    __le32 s_free_blocks_count_hi; /* Free blocks count */
    __le16 s_min_extra_isize;      /* All inodes have at least # bytes */
    __le16 s_want_extra_isize;     /* New inodes should reserve # bytes */
    __le32 s_flags;                /* Miscellaneous flags */
    __le16 s_raid_stride;          /* RAID stride */
    __le16 s_mmp_update_interval;  /* # seconds to wait in MMP checking */
    __le64 s_mmp_block;            /* Block for multi-mount protection */
    __le32 s_raid_stripe_width;    /* blocks on all data disks (N*stride)*/
    __u8   s_log_groups_per_flex;  /* FLEX_BG group size */
    __u8   s_checksum_type;        /* metadata checksum algorithm used */
    __le16 s_reserved_pad;
    __le64 s_kbytes_written;       /* nr of lifetime kilobytes written */
    __le32 s_snapshot_inum;        /* Inode number of active snapshot */
    __le32 s_snapshot_id;          /* sequential ID of active snapshot */
    __le64 s_snapshot_r_blocks_count; /* reserved blocks for active
                                         snapshot's future use */
    __le32 s_snapshot_list;        /* inode number of the head of the
                                      on-disk snapshot list */
#define EXT4_S_ERR_START offsetof(struct ext4_super_block, s_error_count)
    __le32 s_error_count;          /* number of fs errors */
    __le32 s_first_error_time;     /* first time an error happened */
    __le32 s_first_error_ino;      /* inode involved in first error */
    __le64 s_first_error_block;    /* block involved of first error */
    __u8   s_first_error_func[32]; /* function where the error happened */
    __le32 s_first_error_line;     /* line number where error happened */
    __le32 s_last_error_time;      /* most recent time of an error */
    __le32 s_last_error_ino;       /* inode involved in last error */
    __le32 s_last_error_line;      /* line number where error happened */
    __le64 s_last_error_block;     /* block involved of last error */
    __u8   s_last_error_func[32];  /* function where the error happened */
#define EXT4_S_ERR_END offsetof(struct ext4_super_block, s_mount_opts)
    __u8   s_mount_opts[64];
    __le32 s_usr_quota_inum;       /* inode for tracking user quota */
    __le32 s_grp_quota_inum;       /* inode for tracking group quota */
    __le32 s_overhead_clusters;    /* overhead blocks/clusters in fs */
    __le32 s_reserved[108];        /* Padding to the end of the block */
    __le32 s_checksum;             /* crc32c(superblock) */
};

#define EXT4_SB(sb) (sb)

/*
 * Codes for operating systems
 */
#define EXT4_OS_LINUX    0
#define EXT4_OS_HURD     1
#define EXT4_OS_MASIX    2
#define EXT4_OS_FREEBSD  3
#define EXT4_OS_LITES    4

/*
 * Revision levels
 */
#define EXT4_GOOD_OLD_REV 0  /* The good old (original) format */
#define EXT4_DYNAMIC_REV  1  /* V2 format w/ dynamic inode sizes */

#define EXT4_CURRENT_REV  EXT4_GOOD_OLD_REV
#define EXT4_MAX_SUPP_REV EXT4_DYNAMIC_REV

#define EXT4_GOOD_OLD_INODE_SIZE 128

/*
 * Macro-instructions used to manage several block sizes
 */
#define EXT4_MIN_BLOCK_SIZE     1024
#define EXT4_MAX_BLOCK_SIZE     65536
#define EXT4_MIN_BLOCK_LOG_SIZE 10
#define EXT4_MAX_BLOCK_LOG_SIZE 16
#ifdef __KERNEL__
# define EXT4_BLOCK_SIZE(s)         ((s)->s_blocksize)
#else
# define EXT4_BLOCK_SIZE(s)         (EXT4_MIN_BLOCK_SIZE << (s)->s_log_block_size)
#endif
#define EXT4_ADDR_PER_BLOCK(s)      (EXT4_BLOCK_SIZE(s) / sizeof(__u32))
#define EXT4_CLUSTER_SIZE(s)        (EXT4_BLOCK_SIZE(s) << \
                                     EXT4_SB(s)->s_cluster_bits)
#ifdef __KERNEL__
# define EXT4_BLOCK_SIZE_BITS(s)    ((s)->s_blocksize_bits)
# define EXT4_CLUSTER_BITS(s)       (EXT4_SB(s)->s_cluster_bits)
#else
# define EXT4_BLOCK_SIZE_BITS(s)    ((s)->s_log_block_size + 10)
#endif
#ifdef __KERNEL__
#define EXT4_ADDR_PER_BLOCK_BITS(s) (EXT4_SB(s)->s_addr_per_block_bits)
#define EXT4_INODE_SIZE(s)          (EXT4_SB(s)->s_inode_size)
#define EXT4_FIRST_INO(s)           (EXT4_SB(s)->s_first_ino)
#else
#define EXT4_INODE_SIZE(s)          (((s)->s_rev_level == EXT4_GOOD_OLD_REV) ? \
                                     EXT4_GOOD_OLD_INODE_SIZE :         \
                                     (s)->s_inode_size)
#define EXT4_FIRST_INO(s)           (((s)->s_rev_level == EXT4_GOOD_OLD_REV) ? \
                                     EXT4_GOOD_OLD_FIRST_INO :          \
                                     (s)->s_first_ino)
#endif
#define EXT4_BLOCK_ALIGN(size, blkbits) ALIGN((size), (1 << (blkbits)))

/* Translate a block number to a cluster number */
#define EXT4_B2C(sbi, blk)          ((blk) >> (sbi)->s_cluster_bits)
/* Translate a cluster number to a block number */
#define EXT4_C2B(sbi, cluster)      ((cluster) << (sbi)->s_cluster_bits)
/* Translate # of blks to # of clusters */
#define EXT4_NUM_B2C(sbi, blks)     (((blks) + (sbi)->s_cluster_ratio - 1) >> \
                                     (sbi)->s_cluster_bits)
/* Mask out the low bits to get the starting block of the cluster */
#define EXT4_PBLK_CMASK(s, pblk) ((pblk) &                              \
                                  ~((ext4_fsblk_t) (s)->s_cluster_ratio - 1))
#define EXT4_LBLK_CMASK(s, lblk) ((lblk) &                              \
                                  ~((ext4_lblk_t) (s)->s_cluster_ratio - 1))
/* Get the cluster offset */
#define EXT4_PBLK_COFF(s, pblk) ((pblk) &                               \
                                 ((ext4_fsblk_t) (s)->s_cluster_ratio - 1))
#define EXT4_LBLK_COFF(s, lblk) ((lblk) &                               \
                                 ((ext4_lblk_t) (s)->s_cluster_ratio - 1))

/*
 * Macro-instructions used to manage group descriptors
 */
#define EXT4_MIN_DESC_SIZE              32
#define EXT4_MIN_DESC_SIZE_64BIT        64
#define EXT4_MAX_DESC_SIZE              EXT4_MIN_BLOCK_SIZE
#define EXT4_DESC_SIZE(s)               (EXT4_SB(s)->s_desc_size)
#ifdef __KERNEL__
# define EXT4_BLOCKS_PER_GROUP(s)       (EXT4_SB(s)->s_blocks_per_group)
# define EXT4_CLUSTERS_PER_GROUP(s)     (EXT4_SB(s)->s_clusters_per_group)
# define EXT4_DESC_PER_BLOCK(s)         (EXT4_SB(s)->s_desc_per_block)
# define EXT4_INODES_PER_GROUP(s)       (EXT4_SB(s)->s_inodes_per_group)
# define EXT4_DESC_PER_BLOCK_BITS(s)    (EXT4_SB(s)->s_desc_per_block_bits)
#else
# define EXT4_BLOCKS_PER_GROUP(s)       ((s)->s_blocks_per_group)
# define EXT4_DESC_PER_BLOCK(s)         (EXT4_BLOCK_SIZE(s) / EXT4_DESC_SIZE(s))
# define EXT4_INODES_PER_GROUP(s)       ((s)->s_inodes_per_group)
#endif

/*
 * File system states
 */
#define EXT4_VALID_FS    0x0001  /* Unmounted cleanly */
#define EXT4_ERROR_FS    0x0002  /* Errors detected */
#define EXT4_ORPHAN_FS   0x0004  /* Orphans being recovered */

/*
 * Behaviour when detecting errors
 */
#define EXT4_ERRORS_CONTINUE 1   /* Continue execution */
#define EXT4_ERRORS_RO       2   /* Remount fs read-only */
#define EXT4_ERRORS_PANIC    3   /* Panic */
#define EXT4_ERRORS_DEFAULT  EXT4_ERRORS_CONTINUE

/*
 * Misc. filesystem flags
 */
#define EXT2_FLAGS_SIGNED_HASH   0x0001  /* Signed dirhash in use */
#define EXT2_FLAGS_UNSIGNED_HASH 0x0002  /* Unsigned dirhash in use */
#define EXT2_FLAGS_TEST_FILESYS  0x0004  /* to test development code */

/*
 * Feature set definitions
 */
#define EXT4_HAS_COMPAT_FEATURE(sb,mask)                                \
    ((EXT4_SB(sb)->s_es->s_feature_compat & cpu_to_le32(mask)) != 0)
#define EXT4_HAS_RO_COMPAT_FEATURE(sb,mask)                             \
    ((EXT4_SB(sb)->s_es->s_feature_ro_compat & cpu_to_le32(mask)) != 0)
#define EXT4_HAS_INCOMPAT_FEATURE(sb,mask)                              \
    ((EXT4_SB(sb)->s_es->s_feature_incompat & cpu_to_le32(mask)) != 0)
#define EXT4_SET_COMPAT_FEATURE(sb,mask)                        \
    EXT4_SB(sb)->s_es->s_feature_compat |= cpu_to_le32(mask)
#define EXT4_SET_RO_COMPAT_FEATURE(sb,mask)                     \
    EXT4_SB(sb)->s_es->s_feature_ro_compat |= cpu_to_le32(mask)
#define EXT4_SET_INCOMPAT_FEATURE(sb,mask)                      \
    EXT4_SB(sb)->s_es->s_feature_incompat |= cpu_to_le32(mask)
#define EXT4_CLEAR_COMPAT_FEATURE(sb,mask)                      \
    EXT4_SB(sb)->s_es->s_feature_compat &= ~cpu_to_le32(mask)
#define EXT4_CLEAR_RO_COMPAT_FEATURE(sb,mask)                   \
    EXT4_SB(sb)->s_es->s_feature_ro_compat &= ~cpu_to_le32(mask)
#define EXT4_CLEAR_INCOMPAT_FEATURE(sb,mask)                    \
    EXT4_SB(sb)->s_es->s_feature_incompat &= ~cpu_to_le32(mask)

#define EXT4_FEATURE_COMPAT_DIR_PREALLOC       0x0001
#define EXT4_FEATURE_COMPAT_IMAGIC_INODES      0x0002
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL        0x0004
#define EXT4_FEATURE_COMPAT_EXT_ATTR           0x0008
#define EXT4_FEATURE_COMPAT_RESIZE_INODE       0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX          0x0020

#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER    0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE      0x0002
#define EXT4_FEATURE_RO_COMPAT_BTREE_DIR       0x0004
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE       0x0008
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM        0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK       0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE     0x0040
#define EXT4_FEATURE_RO_COMPAT_QUOTA           0x0100
#define EXT4_FEATURE_RO_COMPAT_BIGALLOC        0x0200
/*
 * METADATA_CSUM also enables group descriptor checksums (GDT_CSUM).  When
 * METADATA_CSUM is set, group descriptor checksums use the same algorithm as
 * all other data structures' checksums.  However, the METADATA_CSUM and
 * GDT_CSUM bits are mutually exclusive.
 */
#define EXT4_FEATURE_RO_COMPAT_METADATA_CSUM   0x0400

#define EXT4_FEATURE_INCOMPAT_COMPRESSION      0x0001
#define EXT4_FEATURE_INCOMPAT_FILETYPE         0x0002
#define EXT4_FEATURE_INCOMPAT_RECOVER          0x0004 /* Needs recovery */
#define EXT4_FEATURE_INCOMPAT_JOURNAL_DEV      0x0008 /* Journal device */
#define EXT4_FEATURE_INCOMPAT_META_BG          0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS          0x0040 /* extents support */
#define EXT4_FEATURE_INCOMPAT_64BIT            0x0080
#define EXT4_FEATURE_INCOMPAT_MMP              0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG          0x0200
#define EXT4_FEATURE_INCOMPAT_EA_INODE         0x0400 /* EA in inode */
#define EXT4_FEATURE_INCOMPAT_DIRDATA          0x1000 /* data in dirent */
#define EXT4_FEATURE_INCOMPAT_BG_USE_META_CSUM 0x2000 /* use crc32c for bg */
#define EXT4_FEATURE_INCOMPAT_LARGEDIR         0x4000 /* >2GB or 3-lvl htree */
#define EXT4_FEATURE_INCOMPAT_INLINE_DATA      0x8000 /* data in inode */

#define EXT2_FEATURE_COMPAT_SUPP        EXT4_FEATURE_COMPAT_EXT_ATTR
#define EXT2_FEATURE_INCOMPAT_SUPP     (EXT4_FEATURE_INCOMPAT_FILETYPE| \
                                        EXT4_FEATURE_INCOMPAT_META_BG)
#define EXT2_FEATURE_RO_COMPAT_SUPP    (EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER| \
                                        EXT4_FEATURE_RO_COMPAT_LARGE_FILE|   \
                                        EXT4_FEATURE_RO_COMPAT_BTREE_DIR)

#define EXT3_FEATURE_COMPAT_SUPP        EXT4_FEATURE_COMPAT_EXT_ATTR
#define EXT3_FEATURE_INCOMPAT_SUPP     (EXT4_FEATURE_INCOMPAT_FILETYPE| \
                                        EXT4_FEATURE_INCOMPAT_RECOVER|  \
                                        EXT4_FEATURE_INCOMPAT_META_BG)
#define EXT3_FEATURE_RO_COMPAT_SUPP    (EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER| \
                                        EXT4_FEATURE_RO_COMPAT_LARGE_FILE|   \
                                        EXT4_FEATURE_RO_COMPAT_BTREE_DIR)

#define EXT4_FEATURE_COMPAT_SUPP        EXT2_FEATURE_COMPAT_EXT_ATTR
#define EXT4_FEATURE_INCOMPAT_SUPP     (EXT4_FEATURE_INCOMPAT_FILETYPE| \
                                        EXT4_FEATURE_INCOMPAT_RECOVER|  \
                                        EXT4_FEATURE_INCOMPAT_META_BG|  \
                                        EXT4_FEATURE_INCOMPAT_EXTENTS|  \
                                        EXT4_FEATURE_INCOMPAT_64BIT|    \
                                        EXT4_FEATURE_INCOMPAT_FLEX_BG|  \
                                        EXT4_FEATURE_INCOMPAT_MMP |     \
                                        EXT4_FEATURE_INCOMPAT_INLINE_DATA)
#define EXT4_FEATURE_RO_COMPAT_SUPP    (EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER | \
                                        EXT4_FEATURE_RO_COMPAT_LARGE_FILE |   \
                                        EXT4_FEATURE_RO_COMPAT_GDT_CSUM |     \
                                        EXT4_FEATURE_RO_COMPAT_DIR_NLINK |    \
                                        EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE |  \
                                        EXT4_FEATURE_RO_COMPAT_BTREE_DIR |    \
                                        EXT4_FEATURE_RO_COMPAT_HUGE_FILE |    \
                                        EXT4_FEATURE_RO_COMPAT_BIGALLOC |     \
                                        EXT4_FEATURE_RO_COMPAT_METADATA_CSUM| \
                                        EXT4_FEATURE_RO_COMPAT_QUOTA)

/*
 * Default mount options
 */
#define EXT4_DEFM_DEBUG          0x0001
#define EXT4_DEFM_BSDGROUPS      0x0002
#define EXT4_DEFM_XATTR_USER     0x0004
#define EXT4_DEFM_ACL            0x0008
#define EXT4_DEFM_UID16          0x0010
#define EXT4_DEFM_JMODE          0x0060
#define EXT4_DEFM_JMODE_DATA     0x0020
#define EXT4_DEFM_JMODE_ORDERED  0x0040
#define EXT4_DEFM_JMODE_WBACK    0x0060
#define EXT4_DEFM_NOBARRIER      0x0100
#define EXT4_DEFM_BLOCK_VALIDITY 0x0200
#define EXT4_DEFM_DISCARD        0x0400
#define EXT4_DEFM_NODELALLOC     0x0800

/* Legal values for the dx_root hash_version field: */
#define DX_HASH_LEGACY            0
#define DX_HASH_HALF_MD4          1
#define DX_HASH_TEA               2
#define DX_HASH_LEGACY_UNSIGNED   3
#define DX_HASH_HALF_MD4_UNSIGNED 4
#define DX_HASH_TEA_UNSIGNED      5

#endif /* _FS_EXT4_H_ */
