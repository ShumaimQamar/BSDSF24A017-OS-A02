#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

enum DisplayMode {
    MODE_VERTICAL,   // Default: down then across
    MODE_HORIZONTAL, // -x flag: row-major across
    MODE_LONG        // -l flag: long format
};

void do_ls(const char *dir, enum DisplayMode mode);
void print_long_format(const char *path, const char *filename);
void print_column_display(char **filenames, int count, int max_len);
void print_horizontal_display(char **filenames, int count, int max_len);
void mode_to_letters(int mode, char str[]);

int main(int argc, char *argv[])
{
    int opt;
    enum DisplayMode mode = MODE_VERTICAL;

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
            fprintf(stderr, "Usage: %s [-l] [-x] [directory...]\n", argv[0]);
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
            if (argc - optind > 1)
            {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], mode);
            if (i < argc - 1)
            {
                puts("");
            }
        }
    }
    return 0;
}

void do_ls(const char *dir, enum DisplayMode mode)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    int capacity = 10;
    int count = 0;
    char **filenames = malloc(capacity * sizeof(char *));
    int max_len = 0;

    if (!filenames)
    {
        perror("malloc failed");
        closedir(dp);
        return;
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count >= capacity)
        {
            capacity *= 2;
            filenames = realloc(filenames, capacity * sizeof(char *));
            if (!filenames)
            {
                perror("realloc failed");
                closedir(dp);
                return;
            }
        }

        filenames[count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len)
        {
            max_len = len;
        }
        count++;
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);

    if (count == 0)
    {
        free(filenames);
        return;
    }

    // Call display function based on selected mode
    if (mode == MODE_LONG)
    {
        for (int i = 0; i < count; i++)
        {
            print_long_format(dir, filenames[i]);
        }
    }
    else if (mode == MODE_HORIZONTAL)
    {
        print_horizontal_display(filenames, count, max_len);
    }
    else
    {
        print_column_display(filenames, count, max_len);
    }

    for (int i = 0; i < count; i++)
    {
        free(filenames[i]);
    }
    free(filenames);
}

void print_horizontal_display(char **filenames, int count, int max_len)
{
    struct winsize w;
    int term_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        term_width = w.ws_col;
    }

    int col_width = max_len + 2;
    int cols = term_width / col_width;
    if (cols < 1)
        cols = 1;

    for (int i = 0; i < count; i++)
    {
        printf("%-*s", col_width, filenames[i]);
        if ((i + 1) % cols == 0 || i == count - 1)
        {
            printf("\n");
        }
    }
}

void print_column_display(char **filenames, int count, int max_len)
{
    struct winsize w;
    int term_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        term_width = w.ws_col;
    }

    int col_width = max_len + 2;
    int cols = term_width / col_width;
    if (cols < 1)
        cols = 1;

    int rows = (count + cols - 1) / cols;

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            int index = col * rows + row;
            if (index < count)
            {
                if (col == cols - 1 || index + rows >= count)
                {
                    printf("%s", filenames[index]);
                }
                else
                {
                    printf("%-*s", col_width, filenames[index]);
                }
            }
        }
        printf("\n");
    }
}

void print_long_format(const char *path, const char *filename)
{
    struct stat info;
    char fullpath[1024];

    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);

    if (lstat(fullpath, &info) == -1)
    {
        perror("lstat failed");
        return;
    }

    char mode_str[11];
    mode_to_letters(info.st_mode, mode_str);

    struct passwd *pw = getpwuid(info.st_uid);
    char *user_name = pw ? pw->pw_name : "UNKNOWN";

    struct group *gr = getgrgid(info.st_gid);
    char *group_name = gr ? gr->gr_name : "UNKNOWN";

    char time_buf[20];
    struct tm *tm_info = localtime(&info.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm_info);

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

    if (S_ISDIR(mode))  str[0] = 'd';
    if (S_ISCHR(mode))  str[0] = 'c';
    if (S_ISBLK(mode))  str[0] = 'b';
    if (S_ISLNK(mode))  str[0] = 'l';
    if (S_ISFIFO(mode)) str[0] = 'p';
    if (S_ISSOCK(mode)) str[0] = 's';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';

    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';

    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
}
