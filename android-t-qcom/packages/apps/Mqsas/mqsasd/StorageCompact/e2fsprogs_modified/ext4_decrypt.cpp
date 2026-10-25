/*
 * Copyright (C) Xiaomi Inc.
 *
 */
#define LOG_TAG "FragCompact"

#include "ext4_decrypt.h"
#include "ext2fs.h"
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "../../log.h"
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>

extern ext2_filsys current_fs;
extern CompactLogger log;
extern int donor_dir_fd;

/*
 * ext4_is_dir_encrypted() - return whether dir is encrypted or not
 *
 * Return: 1 if dir is encrypted, 0 otherwise
 */
int ext2fs_is_dir_encrypted(ext2_ino_t dir)
{
    struct ext2_inode inode;
    errcode_t ret;

    ret = ext2fs_read_inode (current_fs, dir, &inode);
    if (ret != 0) {
        LOGE("ext2fs_read_inode error, ret is %ld, dir is %u", ret, dir);
        return 0;
    }

    if (!LINUX_S_ISDIR(inode.i_mode)) {
        LOGE("input dir %u is not a directory", dir);
        return 0;
    }

    if (inode.i_flags & EXT4_ENCRYPT_FL) {
        LOGV("dir %u is encrypted", dir);
        return 1;
    }

    return 0;
}


/*
 * ext4_get_fscrypt_context() - get fscrypt context from inode's xattr
 *
 * Return: 0 on success, -1 on failure
 */
static int ext2fs_get_fscrypt_context(ext2_ino_t ino, union fscrypt_context **ctx)
{
    struct ext2_xattr_handle *handle;
    errcode_t retval;
    size_t value_len;
    int ret = -1;

    if (ctx == NULL) {
        LOGE("invalid parameter");
        return ret;
    }

    retval = ext2fs_xattrs_open(current_fs, ino, &handle);
    if (retval) {
        LOGE("ext2fs_xattrs_open error, ret is %ld", retval);
        return ret;
    }

    retval = ext2fs_xattrs_read(handle);
    if (retval) {
        LOGE("ext2fs_xattrs_read error, ret is %ld", retval);\
        goto err;
    }

    retval = ext2fs_xattr_get(handle, EXT4_XATTR_NAME_ENCRYPTION_CONTEXT,
                (void **)ctx, &value_len);
    if (retval == EXT2_ET_EA_KEY_NOT_FOUND) {
        LOGE("fscrypt context is not found, ino is %u", ino);
        goto err;
    } else if (retval) {
        LOGE("ext2fs_xattr_get error, ret is %ld", retval);
        goto err;
    }

    if (value_len != ext2fs_get_fscrypt_context_size(*ctx)) {
        LOGE("invalid fscrypt context");
        goto err;
    }

    if (!ext2fs_fscrypt_version_is_valid(*ctx)) {
        LOGE("invalid encryption context version");
        goto err;
    }

    ret = 0;
err:
    (void) ext2fs_xattrs_close(&handle);
    return ret;
}

/*
 *ext4_decrypt_fname_v1() - decrypt an encrypted filename for v1 encrypt type
 *
 */
int ext4_decrypt_fname_v1(union fscrypt_context *ctx, fscrypt_str *iname, fscrypt_str *oname)
{
    struct encrypt_fname_v1 *enc_fname_v1;
    int ret;
    errcode_t   retval;
    unsigned int ctx_size;

    retval = ext2fs_get_memzero(sizeof(struct encrypt_fname_v1), &enc_fname_v1);
    if (retval) {
        LOGE("malloc enc_fname_v1 failed");
        return -1;
    }

    ctx_size = ext2fs_get_fscrypt_context_size(ctx);

    memcpy(&enc_fname_v1->ctx_v1, ctx, ctx_size);
    memcpy(&enc_fname_v1->iname, iname, sizeof(fscrypt_str));

    ret = ioctl(donor_dir_fd, EXT4_IOC_DECRYPT_FNAME_V1, enc_fname_v1);
    if (ret) {
        LOGE("failed to decrypt filename, errno is %d", errno);
        ext2fs_free_mem(&enc_fname_v1);
        return -1;
    }

    memcpy(oname, &enc_fname_v1->oname, sizeof(fscrypt_str));
    ext2fs_free_mem(&enc_fname_v1);
    return 0;
}

/*
 *ext4_decrypt_fname_v2() - decrypt an encrypted filename for v2 encrypt type
 *
 */
int ext4_decrypt_fname_v2(union fscrypt_context *ctx, fscrypt_str *iname, fscrypt_str *oname)
{
    struct encrypt_fname_v2 *enc_fname_v2;
    int ret;
    errcode_t   retval;
    unsigned int ctx_size;

    retval = ext2fs_get_memzero(sizeof(struct encrypt_fname_v2), &enc_fname_v2);
    if (retval) {
        LOGE("malloc enc_fname_v2 failed");
        return -1;
    }

    ctx_size = ext2fs_get_fscrypt_context_size(ctx);

    memcpy(&enc_fname_v2->ctx_v2, ctx, ctx_size);
    memcpy(&enc_fname_v2->iname, iname, sizeof(fscrypt_str));

    ret = ioctl(donor_dir_fd, EXT4_IOC_DECRYPT_FNAME_V2, enc_fname_v2);
    if (ret) {
        LOGE("failed to decrypt filename, errno is %d", errno);
        ext2fs_free_mem(&enc_fname_v2);
        return -1;
    }

    memcpy(oname, &enc_fname_v2->oname, sizeof(fscrypt_str));
    ext2fs_free_mem(&enc_fname_v2);
    return 0;
}

/*
 * ext4_decrypt_fname() - decrypt an encrypted filename
 *
 * @dir:    directory inode
 * @iname:  input encrypted file name
 * @oname:  output decrypted file name on success
 * Return: 0 on success, -1 on failure
 */
int ext2fs_decrypt_fname(ext2_ino_t dir, fscrypt_str *iname, fscrypt_str *oname)
{
    union fscrypt_context *ctx = NULL;
    int ret;

    if (iname == NULL || oname == NULL) {
        LOGE("invalid parameter");
        return -1;
    }

    ret = ext2fs_get_fscrypt_context(dir, &ctx);
    if (ret < 0) {
        LOGE("ext4_get_fscrypt_context error, ret is %d", ret);
        return -1;
    }

    if (ctx->version == FS_ENCRYPTION_CONTEXT_V1)
        ret = ext4_decrypt_fname_v1(ctx,iname,oname);
    else
        ret = ext4_decrypt_fname_v2(ctx,iname,oname);

    if (ret < 0) {
        LOGE("ext4_decrypt_fname_v1 or ext4_decrypt_fname_v2 failed, ret is %d", ret);
        ext2fs_free_mem(&ctx);
        return -1;
    }

    ext2fs_free_mem(&ctx);
    return 0;
}
