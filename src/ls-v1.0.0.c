/*
* Programming Assignment 02: lsv1.1.0
* Version 1.1.0 adds the -l (long listing) option
* Usage:
*       $ lsv1.1.0
*       $ lsv1.1.0 -l
*       $ lsv1.1.0 -l /home /etc/
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

extern int errno;

void do_ls(const char *dir);
void do_ls_long(const char *dir);
void mode_to_string(mode_t mode, char *str);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;

    /* Parse command-line options */
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
            do_ls(".");
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            if (long_flag)
                do_ls_long(argv[i]);
            else
                do_ls(argv[i]);
            puts("");
        }
    }
    return 0;
}

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

    /* Special bits: setuid, setgid, sticky */
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

    /* Pass 1: compute column widths and total blocks */
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

    /* Pass 2: print each entry */
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

        /* ctime() returns "Thu Oct 10 12:34:56 2026\n"; skip the weekday */
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
