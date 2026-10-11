/*
* Programming Assignment 02: lsv1.5.0
* v1.1.0 added -l (long listing)
* v1.2.0 added column display (down then across) as the default
* v1.3.0 added -x (horizontal display)
* v1.4.0 added alphabetical sorting
* v1.5.0 adds colorized output based on file type
* Usage:
*       $ lsv1.5.0
*       $ lsv1.5.0 -l
*       $ lsv1.5.0 -x
*       $ lsv1.5.0 -x /home /etc/
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

extern int errno;

/* Task 2: ANSI escape codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"   /* directory        */
#define COLOR_GREEN   "\033[0;32m"   /* executable       */
#define COLOR_RED     "\033[0;31m"   /* tarballs/archives */
#define COLOR_PINK    "\033[0;35m"   /* symbolic link    */
#define COLOR_REVERSE "\033[7m"      /* special files    */

enum display_mode
{
    MODE_DEFAULT,      /* down then across */
    MODE_LONG,         /* -l */
    MODE_HORIZONTAL    /* -x */
};

static int use_color = 0;   /* set in main: only color when output is a terminal */

void do_ls(const char *dir, enum display_mode mode);
int  read_names(const char *dir, char ***out, size_t *count, size_t *max_len);
int  compare_names(const void *a, const void *b);
void do_ls_long(const char *dir, char **names, size_t count);
void print_vertical(const char *dir, char **names, size_t count, size_t max_len);
void print_horizontal(const char *dir, char **names, size_t count, size_t max_len);
void print_name(const char *dir, const char *name, size_t pad_to);
const char *get_color(mode_t mode, const char *name);
int  ends_with(const char *name, const char *suffix);
int  get_terminal_width(void);
void mode_to_string(mode_t mode, char *str);

int main(int argc, char *argv[])
{
    int opt;
    enum display_mode mode = MODE_DEFAULT;

    use_color = isatty(STDOUT_FILENO);

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l':
            mode = MODE_LONG;
            break;
        case 'x':
            mode = MODE_HORIZONTAL;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l | -x] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], mode);
            puts("");
        }
    }
    return 0;
}

/* Terminal width via ioctl, fallback to 80 */
int get_terminal_width(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
    return 80;
}

/* Comparison function for qsort(): each argument is a char ** */
int compare_names(const void *a, const void *b)
{
    const char *s1 = *(const char * const *)a;
    const char *s2 = *(const char * const *)b;
    return strcmp(s1, s2);
}

/* Returns 1 if name ends with suffix */
int ends_with(const char *name, const char *suffix)
{
    size_t n = strlen(name), s = strlen(suffix);
    return (n >= s) && (strcmp(name + n - s, suffix) == 0);
}

/* Task 4: choose the color from st_mode (from lstat) and the file name */
const char *get_color(mode_t mode, const char *name)
{
    if (S_ISLNK(mode))
        return COLOR_PINK;
    if (S_ISDIR(mode))
        return COLOR_BLUE;
    if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISFIFO(mode) || S_ISSOCK(mode))
        return COLOR_REVERSE;
    if (ends_with(name, ".tar") || ends_with(name, ".gz") ||
        ends_with(name, ".zip") || ends_with(name, ".tgz"))
        return COLOR_RED;
    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        return COLOR_GREEN;
    return NULL;   /* normal file: no color */
}

/* Task 3 + 4: lstat the file, print its name in color, then pad with
 * spaces up to pad_to visible characters. The padding is added by hand
 * because the invisible color codes would break printf("%-*s"). */
void print_name(const char *dir, const char *name, size_t pad_to)
{
    char path[PATH_MAX];
    struct stat st;
    const char *color = NULL;

    if (use_color)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, name);
        if (lstat(path, &st) == 0)
            color = get_color(st.st_mode, name);
    }

    if (color)
        printf("%s%s%s", color, name, COLOR_RESET);
    else
        printf("%s", name);

    for (size_t i = strlen(name); i < pad_to; i++)
        putchar(' ');
}

/* Read all names (skipping hidden files) into a dynamic array. */
int read_names(const char *dir, char ***out, size_t *count_out, size_t *max_len_out)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return -1;
    }

    char **names = NULL;
    size_t count = 0, capacity = 0, max_len = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity)
        {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (tmp == NULL)
            {
                perror("realloc failed");
                break;
            }
            names = tmp;
        }

        names[count] = strdup(entry->d_name);
        if (names[count] == NULL)
        {
            perror("strdup failed");
            break;
        }

        size_t len = strlen(names[count]);
        if (len > max_len)
            max_len = len;
        count++;
    }
    if (errno != 0)
        perror("readdir failed");
    closedir(dp);

    *out = names;
    *count_out = count;
    *max_len_out = max_len;
    return 0;
}

/* Read -> sort -> display -> free */
void do_ls(const char *dir, enum display_mode mode)
{
    char **names = NULL;
    size_t count = 0, max_len = 0;

    if (read_names(dir, &names, &count, &max_len) == -1)
        return;

    if (count > 1)
        qsort(names, count, sizeof(char *), compare_names);

    if (count > 0)
    {
        if (mode == MODE_LONG)
            do_ls_long(dir, names, count);
        else if (mode == MODE_HORIZONTAL)
            print_horizontal(dir, names, count, max_len);
        else
            print_vertical(dir, names, count, max_len);
    }

    for (size_t i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* Default: "down then across" */
void print_vertical(const char *dir, char **names, size_t count, size_t max_len)
{
    int term_width = get_terminal_width();
    size_t spacing = 2;
    size_t col_width = max_len + spacing;
    size_t num_cols = term_width / col_width;
    if (num_cols < 1)
        num_cols = 1;
    size_t num_rows = (count + num_cols - 1) / num_cols;

    for (size_t row = 0; row < num_rows; row++)
    {
        for (size_t col = 0; col < num_cols; col++)
        {
            size_t idx = row + col * num_rows;
            if (idx >= count)
                break;

            if (idx + num_rows < count && col < num_cols - 1)
                print_name(dir, names[idx], col_width);
            else
                print_name(dir, names[idx], 0);
        }
        printf("\n");
    }
}

/* -x: horizontal (row-major) display */
void print_horizontal(const char *dir, char **names, size_t count, size_t max_len)
{
    int term_width = get_terminal_width();
    size_t spacing = 2;
    size_t col_width = max_len + spacing;
    size_t pos = 0;

    for (size_t i = 0; i < count; i++)
    {
        if (pos > 0 && pos + col_width > (size_t)term_width)
        {
            printf("\n");
            pos = 0;
        }
        print_name(dir, names[i], col_width);
        pos += col_width;
    }
    printf("\n");
}

/* Convert st_mode into a string such as "drwxr-xr-x" */
void mode_to_string(mode_t mode, char *str)
{
    if (S_ISREG(mode))       str[0] = '-';
    else if (S_ISDIR(mode))  str[0] = 'd';
    else if (S_ISLNK(mode))  str[0] = 'l';
    else if (S_ISCHR(mode))  str[0] = 'c';
    else if (S_ISBLK(mode))  str[0] = 'b';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';
    else                     str[0] = '?';

    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';

    str[10] = '\0';
}

/* Long listing (-l), works from the sorted array of names */
void do_ls_long(const char *dir, char **names, size_t count)
{
    struct stat st;
    char path[PATH_MAX];

    int w_link = 1, w_user = 1, w_group = 1, w_size = 1;
    long long total = 0;

    for (size_t i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        if (lstat(path, &st) == -1)
            continue;

        char tmp[64];
        int len;

        len = snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)st.st_nlink);
        if (len > w_link) w_link = len;

        struct passwd *pw = getpwuid(st.st_uid);
        len = pw ? (int)strlen(pw->pw_name)
                 : snprintf(tmp, sizeof(tmp), "%u", st.st_uid);
        if (len > w_user) w_user = len;

        struct group *gr = getgrgid(st.st_gid);
        len = gr ? (int)strlen(gr->gr_name)
                 : snprintf(tmp, sizeof(tmp), "%u", st.st_gid);
        if (len > w_group) w_group = len;

        len = snprintf(tmp, sizeof(tmp), "%lld", (long long)st.st_size);
        if (len > w_size) w_size = len;

        total += st.st_blocks;
    }

    printf("total %lld\n", total / 2);

    for (size_t i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        if (lstat(path, &st) == -1)
        {
            perror(path);
            continue;
        }

        char mode[11];
        mode_to_string(st.st_mode, mode);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        char user[64], group[64];
        if (pw) snprintf(user, sizeof(user), "%s", pw->pw_name);
        else    snprintf(user, sizeof(user), "%u", st.st_uid);
        if (gr) snprintf(group, sizeof(group), "%s", gr->gr_name);
        else    snprintf(group, sizeof(group), "%u", st.st_gid);

        time_t t = st.st_mtime;
        char timestr[32];
        snprintf(timestr, sizeof(timestr), "%.12s", ctime(&t) + 4);

        printf("%s %*lu %-*s %-*s %*lld %s ",
               mode,
               w_link, (unsigned long)st.st_nlink,
               w_user, user,
               w_group, group,
               w_size, (long long)st.st_size,
               timestr);

        /* the file name is printed in color */
        print_name(dir, names[i], 0);

        if (S_ISLNK(st.st_mode))
        {
            char target[PATH_MAX];
            ssize_t n = readlink(path, target, sizeof(target) - 1);
            if (n != -1)
            {
                target[n] = '\0';
                printf(" -> %s", target);
            }
        }
        printf("\n");
    }
}
