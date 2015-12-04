/************************************************/
/* FORMER CLOSE SYSTEM CALL BODY */
/************************************************/

static int manual_close(unsigned int fd)
{
    int retval = __close_fd(current->files, fd);

    /* can't restart close syscall because file table entry was cleared */
    if (unlikely(retval == -ERESTARTSYS || retval == -ERESTARTNOINTR || retval == -ERESTARTNOHAND || retval == -ERESTART_RESTARTBLOCK))
        retval = -EINTR;

    return retval;
}

/************************************************/
