#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

void do_ls(const char *dir, int l_flag);
void print_long_format(const char *path, const char *filename);
void mode_to_letters(int mode, char str[]);

int main(int argc, char *argv[])
{
    int opt;
    int l_flag = 0;

    // Parse command line arguments for -l flag
    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            l_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // If no directories specified, default to current directory "."
    if (optind == argc)
    {
        do_ls(".", l_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
            {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], l_flag);
            if (i < argc - 1)
            {
                puts("");
            }
        }
    }
    return 0;
}

void do_ls(const char *dir, int l_flag)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        // Skip hidden files
        if (entry->d_name[0] == '.')
            continue;

        if (l_flag)
        {
            print_long_format(dir, entry->d_name);
        }
        else
        {
            printf("%s\n", entry->d_name);
        }
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

void print_long_format(const char *path, const char *filename)
{
    struct stat info;
    char fullpath[1024];

    // Construct full path for stat call
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);

    if (lstat(fullpath, &info) == -1)
    {
        perror("lstat failed");
        return;
    }

    char mode_str[11];
    mode_to_letters(info.st_mode, mode_str);

    // Get Owner User Name
    struct passwd *pw = getpwuid(info.st_uid);
    char *user_name = pw ? pw->pw_name : "UNKNOWN";

    // Get Group Name
    struct group *gr = getgrgid(info.st_gid);
    char *group_name = gr ? gr->gr_name : "UNKNOWN";

    // Format time modification string
    char time_buf[20];
    struct tm *tm_info = localtime(&info.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm_info);

    // Print in standard long listing format
    printf("%s %2ld %-8s %-8s %8lld %s %s\n",
           mode_str,
           (long)info.st_nlink,
           user_name,
           group_name,
           (long long)info.st_size,
           time_buf,
           filename);
}

void mode_to_letters(int mode, char str[])
{
    strcpy(str, "----------");

    // File type determination
    if (S_ISDIR(mode))  str[0] = 'd';
    if (S_ISCHR(mode))  str[0] = 'c';
    if (S_ISBLK(mode))  str[0] = 'b';
    if (S_ISLNK(mode))  str[0] = 'l';
    if (S_ISFIFO(mode)) str[0] = 'p';
    if (S_ISSOCK(mode)) str[0] = 's';

    // Owner permissions
    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';

    // Group permissions
    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';

    // Others permissions
    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
}
