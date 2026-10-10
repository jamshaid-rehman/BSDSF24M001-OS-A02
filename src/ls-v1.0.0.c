/*
* Programming Assignment 02: lsv1.2.0
* v1.1.0 added -l (long listing)
* v1.2.0 adds column display (down then across) for the default output
* Usage:
*       $ lsv1.2.0
*       $ lsv1.2.0 -l
*       $ lsv1.2.0 /home /etc/
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

void do_ls(const char *dir);
void do_ls_columns(const char *dir);
void do_ls_long(const char *dir);
void mode_to_string(mode_t mode, char *str);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        if (long_flag)
            do_ls_long(".");
        else
            do_ls_columns(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            if (long_flag)
                do_ls_long(argv[i]);
            else
                do_ls_columns(argv[i]);
            puts("");
        }
    }
    return 0;
}

/* Original simple listing (one name per line), kept for reference */
void do_ls(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

/* Feature 3: column display, "down then across" */
void do_ls_columns(const char *dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    /* Task 2: read all names into a dynamic array, track longest name */
    char **names = NULL;
    size_t count = 0, capacity = 0;
    size_t max_len = 0;

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
                for (size_t i = 0; i < count; i++)
                    free(names[i]);
                free(names);
                closedir(dp);
                return;
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

    if (count == 0)
    {
        free(names);
        return;
    }

    /* Task 3: terminal width via ioctl, fallback to 80 */
    int term_width = 80;
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        term_width = ws.ws_col;

    size_t spacing = 2;
    size_t col_width = max_len + spacing;
    size_t num_cols = term_width / col_width;
    if (num_cols < 1)
        num_cols = 1;
    size_t num_rows = (count + num_cols - 1) / num_cols;   /* ceiling */

    /* Task 4: print row by row, going down each column first */
    for (size_t row = 0; row < num_rows; row++)
    {
        for (size_t col = 0; col < num_cols; col++)
        {
            size_t idx = row + col * num_rows;
            if (idx >= count)
                break;

            /* pad every item except the last one on the row */
            if (idx + num_rows < count && col < num_cols - 1)
                printf("%-*s", (int)col_width, names[idx]);
            else
                printf("%s", names[idx]);
        }
        printf("\n");
    }

    for (size_t i = 0; i < count; i++)
        free(names[i]);
    free(names);
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

/* Long listing (-l) */
void do_ls_long(const char *dir)
{
    struct dirent *entry;
    struct stat st;
    char path[PATH_MAX];
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    int w_link = 1, w_user = 1, w_group = 1, w_size = 1;
    long long total = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
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
    rewinddir(dp);

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
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

        printf("%s %*lu %-*s %-*s %*lld %s %s",
               mode,
               w_link, (unsigned long)st.st_nlink,
               w_user, user,
               w_group, group,
               w_size, (long long)st.st_size,
               timestr,
               entry->d_name);

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
    closedir(dp);
}
