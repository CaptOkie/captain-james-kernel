/************************************************/
/* CUSTOM FUNCTIONS */
/************************************************/

#define MAX_OPEN_TIME 30
#define OPEN_TIME_TOTAL "user.open_time_total" // secpnds
#define OPEN_TIME_FIRST "user.open_time_first" // seconds
#define OPEN_COUNT_ATTR "user.open_count_attr"
static char* restricted_to[] = { "/", "home", "student", "crazy" };
static int restricted_to_length = sizeof(restricted_to) / sizeof(restricted_to[0]);

static bool in_restricted_path(struct file* f)
{
    const char* directories[restricted_to_length];
    int i;
    struct dentry* curr = NULL;
    struct dentry* prev = NULL;

    memset(directories, 0, sizeof(directories));

    curr = f->f_path.dentry;
    while (curr && prev != curr) {
        for (i = restricted_to_length - 1; i > 0; --i) {
            directories[i] = directories[i-1];
        }
        directories[0] = curr->d_name.name;
        prev = curr;
        curr = curr->d_parent;
    }

    for (i = restricted_to_length - 1; i >= 0; --i) {
        if (directories[i] == NULL || strcmp(directories[i], restricted_to[i]) != 0) {
            return false;
        }
    }

    return true;
}

static inline long calc_open_time(long curr_time, long open_time_total, long open_time_first)
{
    return open_time_total + (curr_time - open_time_first);
}

static inline bool is_old(struct tm* curr, struct tm* open)
{
    return (curr->tm_yday > open->tm_yday) || (curr->tm_year > open->tm_year);
}

static inline long to_midnight(long curr_time, struct tm* curr)
{
    return curr_time - (((curr->tm_hour * 60) + curr->tm_min) * 60) + curr->tm_sec;
}

static bool allow_open(struct file* f)
{
    struct timespec curr_time;
    struct tm curr, open;
    long open_time_total;
    long open_time_first;

    long open_count;

    if (long_getxattr(&(f->f_path), OPEN_TIME_TOTAL, &open_time_total) <= 0) {
        open_time_total = 0;
    }

    if (long_getxattr(&(f->f_path), OPEN_COUNT_ATTR, &open_count) <= 0) {
        open_count = 0;
    }

    curr_time = CURRENT_TIME;
    if (long_getxattr(&(f->f_path), OPEN_TIME_FIRST, &open_time_first) <= 0) {
        open_time_first = curr_time.tv_sec;
    }
    time_to_tm(curr_time.tv_sec, 0, &curr);
    time_to_tm(open_time_first, 0, &open);

    if (is_old(&curr, &open)) {
        if (open_count > 0) {            
            open_time_first = to_midnight(curr_time.tv_sec, &curr);
        }
        open_time_total = 0;
        long_setxattr(&(f->f_path), OPEN_TIME_TOTAL, &open_time_total);
    }

    if (open_count > 0) {
        open_time_total = calc_open_time(curr_time.tv_sec, open_time_total, open_time_first);
    }

    if (open_time_total > MAX_OPEN_TIME) {
        printk("Stopped Open: File: %s, Open Count: %ld\n", f->f_path.dentry->d_name.name, open_count);
        return false;
    }

    if (open_count <= 0) {
        long_setxattr(&(f->f_path), OPEN_TIME_FIRST, &(curr_time.tv_sec));
    }

    ++open_count;
    long_setxattr(&(f->f_path), OPEN_COUNT_ATTR, &open_count);

    printk("Allowed Open: File: %s, Open Count: %ld\n", f->f_path.dentry->d_name.name, open_count);
    return true;
}

static void on_close(struct file* f)
{
    long curr_time;
    long open_time_first;
    long open_time_total;

    long open_count;
    ssize_t get_error;

    struct tm curr, open;

    get_error = long_getxattr(&(f->f_path), OPEN_COUNT_ATTR, &open_count);
    if (get_error <= 0) 
        open_count = 0;

    if (open_count > 0) {
        --open_count;
        long_setxattr(&(f->f_path), OPEN_COUNT_ATTR, &open_count);
        if (open_count == 0) {
            curr_time = CURRENT_TIME.tv_sec;
            
            get_error = long_getxattr(&(f->f_path), OPEN_TIME_FIRST, &open_time_first);
            if (get_error <= 0)
                return;

            get_error = long_getxattr(&(f->f_path), OPEN_TIME_TOTAL, &open_time_total);
            if (get_error <= 0) {
                open_time_total = 0;
            }
            
            time_to_tm(curr_time, 0, &curr);
            time_to_tm(open_time_first, 0, &open);
            if (is_old(&curr, &open)) {
                open_time_first = to_midnight(curr_time, &curr);
                open_time_total = 0;
            }
            
            open_time_total = calc_open_time(curr_time, open_time_total, open_time_first);
            long_setxattr(&(f->f_path), OPEN_TIME_TOTAL, &open_time_total);
            printk("File: %s was open for: %ld\n", f->f_path.dentry->d_name.name, open_time_total);
        }
        else {
            printk("File: %s Open elsewhere.\n", f->f_path.dentry->d_name.name);
        }
    }
    return;
}

/***********************************************/
