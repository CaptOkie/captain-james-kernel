/***********************************************/
/* THIS THE setxattr STUFF RIGHT HERE */
/***********************************************/

static long setxattr(struct dentry *d, const char *name, void *value, size_t size, int flags)
{
    long error;
    void *kvalue = NULL;
    void *vvalue = NULL;    /* If non-NULL, we used vmalloc() */
    char kname[XATTR_NAME_MAX + 1];

    if (flags & ~(XATTR_CREATE|XATTR_REPLACE))
        return -EINVAL;

    error = (long)strncpy(kname, name, sizeof(kname));
    if (error == 0) {
        return error;
    }

    if (size) {
        if (size > XATTR_SIZE_MAX)
            return -E2BIG;
        kvalue = kmalloc(size, GFP_KERNEL | __GFP_NOWARN);
        if (!kvalue) {
            vvalue = vmalloc(size);
            if (!vvalue)
                return -ENOMEM;
            kvalue = vvalue;
        }
        error = (long)memcpy(kvalue, value, size);
        if (error == 0) {
            goto out;
        }
        if ((strcmp(kname, XATTR_NAME_POSIX_ACL_ACCESS) == 0) || (strcmp(kname, XATTR_NAME_POSIX_ACL_DEFAULT) == 0))
            posix_acl_fix_xattr_from_user(kvalue, size);
    }

    error = vfs_setxattr(d, kname, kvalue, size, flags);
out:
    if (vvalue)
        vfree(vvalue);
    else
        kfree(kvalue);
    return error;
}

static int path_setxattr(struct path* path, const char* name, void* value, size_t size, int flags, unsigned int lookup_flags)
{
    int error;
retry:
    error = mnt_want_write(path->mnt);
    if (!error) {
        error = setxattr(path->dentry, name, value, size, flags);
        mnt_drop_write(path->mnt);
    }
    if (retry_estale(error, lookup_flags)) {
        lookup_flags |= LOOKUP_REVAL;
        goto retry;
    }
    return error;
}

static inline int long_setxattr(struct path* path, const char* name, long* value)
{
    return path_setxattr(path, name, value, sizeof(*value), 0, LOOKUP_FOLLOW);
}

/***********************************************/

/***********************************************/
/* THIS THE getxattr STUFF RIGHT HERE */
/***********************************************/

static ssize_t getxattr(struct dentry *d, const char *name, void *value, size_t size)
{
    ssize_t error;
    void *kvalue = NULL;
    void *vvalue = NULL;
    char kname[XATTR_NAME_MAX + 1];

    error = (long)strncpy(kname, name, sizeof(kname));
    if (error == 0) {
        return error;
    }

    if (size) {
        if (size > XATTR_SIZE_MAX)
            size = XATTR_SIZE_MAX;
        kvalue = kzalloc(size, GFP_KERNEL | __GFP_NOWARN);
        if (!kvalue) {
            vvalue = vmalloc(size);
            if (!vvalue)
                return -ENOMEM;
            kvalue = vvalue;
        }
    }

    error = vfs_getxattr(d, kname, kvalue, size);
    if (error > 0) {
        if ((strcmp(kname, XATTR_NAME_POSIX_ACL_ACCESS) == 0) || (strcmp(kname, XATTR_NAME_POSIX_ACL_DEFAULT) == 0))
            posix_acl_fix_xattr_to_user(kvalue, size);
        if (size) {
            void* ptr = memcpy(value, kvalue, error);
            if (!ptr)
                error = -EFAULT;
        }
    } else if (error == -ERANGE && size >= XATTR_SIZE_MAX) {
        /* The file system tried to returned a value bigger
           than XATTR_SIZE_MAX bytes. Not possible. */
        error = -E2BIG;
    }
    if (vvalue)
        vfree(vvalue);
    else
        kfree(kvalue);
    return error;
}

static ssize_t path_getxattr(struct path* path, const char* name, void* value, size_t size, unsigned int lookup_flags)
{
    ssize_t error;
retry:
    error = getxattr(path->dentry, name, value, size);
    if (retry_estale(error, lookup_flags)) {
        lookup_flags |= LOOKUP_REVAL;
        goto retry;
    }
    return error;
}

static inline ssize_t long_getxattr(struct path* path, const char* name, long* value)
{
    return path_getxattr(path, name, value, sizeof(*value), LOOKUP_FOLLOW);
}

/************************************************/
