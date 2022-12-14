/*
 * ext4_get_pathname.cpp --- do directry/inode -> name translation
 *
 * Copyright (C) Xiaomi Inc.
 *
 */
#define LOG_TAG "FragCompact"

#include "config.h"
#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "../../log.h"
#include "ext2fs.h"
#include "ext4_decrypt.h"

extern CompactLogger log;

struct get_pathname_struct {
    ext2_ino_t  search_ino;
    ext2_ino_t  parent;
    ext2_ino_t  dir;
    char        *name;
    errcode_t   errcode;
};

static int get_pathname_proc(struct ext2_dir_entry *dirent,
                 int	offset EXT2FS_ATTR((unused)),
                 int	blocksize EXT2FS_ATTR((unused)),
                 char	*buf EXT2FS_ATTR((unused)),
                 void	*priv_data)
{
    struct get_pathname_struct	*gp;
    errcode_t			retval;
    struct fscrypt_str iname;
    struct fscrypt_str oname;
    int ret;
    int name_len = ext2fs_dirent_name_len(dirent);

    gp = (struct get_pathname_struct *) priv_data;

    if ((name_len == 2) && !strncmp(dirent->name, "..", 2))
        gp->parent = dirent->inode;
    if (dirent->inode == gp->search_ino) {
        retval = ext2fs_get_mem(name_len + 1, &gp->name);
        if (retval) {
            LOGE("ext2fs_get_mem failed, retval is %ld", retval);
            gp->errcode = retval;
            return DIRENT_ABORT;
        }

        if (ext2fs_is_dir_encrypted(gp->dir)) {
            memcpy(iname.fname, dirent->name, name_len);
            iname.len = name_len;

            ret = ext2fs_decrypt_fname(gp->dir, &iname, &oname);
            if (ret) {
                LOGE("ext4_decrypt_fname failed, ret is %d", ret);
                return DIRENT_ABORT;
            }

            strncpy(gp->name, oname.fname, oname.len);
            gp->name[oname.len] = '\0';
        } else {
            strncpy(gp->name, dirent->name, name_len);
            gp->name[name_len] = '\0';
        }
        return DIRENT_ABORT;
    }
    return 0;
}

static errcode_t ext2fs_get_pathname_int(ext2_filsys fs, ext2_ino_t dir,
                     ext2_ino_t ino, int maxdepth,
                     char *buf, char **name)
{
    struct get_pathname_struct gp;
    char	*parent_name = 0, *ret;
    errcode_t	retval;

    if (dir == ino) {
        retval = ext2fs_get_mem(2, name);
        if (retval)
            return retval;
        strcpy(*name, (dir == EXT2_ROOT_INO) ? "/" : ".");
        return 0;
    }

    if (!dir || (maxdepth < 0)) {
        retval = ext2fs_get_mem(4, name);
        if (retval)
            return retval;
        strcpy(*name, "...");
        return 0;
    }

    gp.search_ino = ino;
    gp.parent = 0;
    gp.dir = dir;
    gp.name = 0;
    gp.errcode = 0;

    retval = ext2fs_dir_iterate(fs, dir, 0, buf, get_pathname_proc, &gp);
    if (retval == EXT2_ET_NO_DIRECTORY) {
        char tmp[32];

        if (ino)
            snprintf(tmp, sizeof(tmp), "<%u>/<%u>", dir, ino);
        else
            snprintf(tmp, sizeof(tmp), "<%u>", dir);
        retval = ext2fs_get_mem(strlen(tmp)+1, name);
        if (retval)
            goto cleanup;
        strcpy(*name, tmp);
        return 0;
    } else if (retval) {
        goto cleanup;
    }
    if (gp.errcode) {
        retval = gp.errcode;
        goto cleanup;
    }

    retval = ext2fs_get_pathname_int(fs, gp.parent, dir, maxdepth-1,
                    buf, &parent_name);
    if (retval)
        goto cleanup;
    if (!ino) {
        *name = parent_name;
        return 0;
    }

    if (gp.name)
        retval = ext2fs_get_mem(strlen(parent_name)+strlen(gp.name)+2,
                    &ret);
    else
        retval = ext2fs_get_mem(strlen(parent_name)+5, &ret);
    if (retval)
        goto cleanup;

    ret[0] = 0;
    if (parent_name[1])
        strcat(ret, parent_name);
    strcat(ret, "/");
    if (gp.name)
        strcat(ret, gp.name);
    else
        strcat(ret, "???");
    *name = ret;
    retval = 0;

cleanup:
    ext2fs_free_mem(&parent_name);
    ext2fs_free_mem(&gp.name);
    return retval;
}

errcode_t ext2fs_get_pathname_ex(ext2_filsys fs, ext2_ino_t dir, ext2_ino_t ino,
                    char **name)
{
    char	*buf;
    errcode_t	retval;

    EXT2_CHECK_MAGIC(fs, EXT2_ET_MAGIC_EXT2FS_FILSYS);

    retval = ext2fs_get_mem(fs->blocksize, &buf);
    if (retval)
        return retval;
    if (dir == ino)
        ino = 0;
    retval = ext2fs_get_pathname_int(fs, dir, ino, 32, buf, name);
    ext2fs_free_mem(&buf);
    return retval;
}
