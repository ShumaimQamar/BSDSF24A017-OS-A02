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

#define COLOR_RESET     "\033[0m"
#define COLOR_BLUE      "\033[1;34m" // Directories
#define COLOR_GREEN     "\033[1;32m" // Executables
#define COLOR_RED       "\033[1;31m" // Tarballs (.tar, .gz, .zip)
#define COLOR_PINK      "\033[1;35m" // Symbolic Links
#define COLOR_REVERSE   "\033[7m"    // Special files

enum DisplayMode {
    MODE_VERTICAL,   // Default: down then across
    MODE_HORIZONTAL, // -x flag: row-major across
    MODE_LONG        // -l flag: long format
};

void do_ls(const char *dir, enum DisplayMode mode, int recursive, int is_first_dir);
void print_long_format(const char *path, const char *filename);
void print_column_display(const char *dir, char **filenames, int count, int max_len);
void print_horizontal_display(const char *dir, char **filenames, int count, int max_len);
void mode_to_letters(int mode, char str[]);
int compare_names(const void *a, const void *b);
const char *get_color_code(const char *path, const char *filename);
int is_tarball(const char *filename);

int main(int argc, char *argv[])
{
    int opt;
    enum DisplayMode mode = MODE_VERTICAL;
    int recursive_flag = 0; // Task 2: Tracking -R option

    while ((opt = getopt(argc, argv, "lxR")) != -1)
    {
        switch (opt)
        {
        case 'l':
            mode = MODE_LONG;
            break;
        case 'x':
            mode = MODE_HORIZONTAL;
            break;
        case 'R':
            recursive_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [-x] [-R] [directory...]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode, recursive_flag, 1);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1 || recursive_flag)
            {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], mode, recursive_flag, 1);
            if (i < argc - 1)
            {
                puts("");
            }
        }
    }
    return 0;
}

// Check for tarball extension
int is_tarball(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot) return 0;

    if (strcmp(dot, ".tar") == 0 || strcmp(dot, ".gz") == 0 || strcmp(dot, ".zip") == 0)
    {
        return 1;
    }
    return 0;
}

// Get color code string based on st_mode and filename
const char *get_color_code(const char *path, const char *filename)
{
    struct stat info;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);

    if (lstat(fullpath, &info) == -1)
    {
        return COLOR_RESET;
    }

    if (S_ISLNK(info.st_mode)) return COLOR_PINK;
    if (S_ISDIR(info.st_mode)) return COLOR_BLUE;

    if (S_ISCHR(info.st_mode) || S_ISBLK(info.st_mode) || 
        S_ISFIFO(info.st_mode) || S_ISSOCK(info.st_mode))
    {
        return COLOR_REVERSE;
    }

    if (is_tarball(filename)) return COLOR_RED;
    if (info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) return COLOR_GREEN;

    return COLOR_RESET;
}

int compare_names(const void *a, const void *b)
{
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;
    return strcmp(str1, str2);
}

// Task 3: Modified Core Function for Recursion
void do_ls(const char *dir, enum DisplayMode mode, int recursive, int is_first_dir)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    // Print header if in recursive mode and not the very first directory printed by main
    if (recursive && !is_first_dir)
    {
        printf("\n%s:\n", dir);
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
        // Ignore hidden files and standard relative entries (. and ..)
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

    // Sort entries alphabetically
    qsort(filenames, count, sizeof(char *), compare_names);

    // Pass 1: Print immediate contents of current directory
    if (mode == MODE_LONG)
    {
        for (int i = 0; i < count; i++)
        {
            print_long_format(dir, filenames[i]);
        }
    }
    else if (mode == MODE_HORIZONTAL)
    {
        print_horizontal_display(dir, filenames, count, max_len);
    }
    else
    {
        print_column_display(dir, filenames, count, max_len);
    }

    // Pass 2: If recursive mode enabled, descend into subdirectories
    if (recursive)
    {
        for (int i = 0; i < count; i++)
        {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, filenames[i]);

            struct stat info;
            if (lstat(fullpath, &info) == 0 && S_ISDIR(info.st_mode))
            {
                // Recursive call with full path
                do_ls(fullpath, mode, recursive, 0);
            }
        }
    }

    // Cleanup allocated memory
    for (int i = 0; i < count; i++)
    {
        free(filenames[i]);
    }
    free(filenames);
}

void print_horizontal_display(const char *dir, char **filenames, int count, int max_len)
{
    struct winsize w;
    int term_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        term_width = w.ws_col;
    }

    int col_width = max_len + 2;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    for (int i = 0; i < count; i++)
    {
        const char *color = get_color_code(dir, filenames[i]);
        int name_len = strlen(filenames[i]);
        int padding = col_width - name_len;

        printf("%s%s%s", color, filenames[i], COLOR_RESET);

        if ((i + 1) % cols == 0 || i == count - 1)
        {
            printf("\n");
        }
        else
        {
            printf("%*s", padding, "");
        }
    }
}

void print_column_display(const char *dir, char **filenames, int count, int max_len)
{
    struct winsize w;
    int term_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        term_width = w.ws_col;
    }

    int col_width = max_len + 2;
    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    int rows = (count + cols - 1) / cols;

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < cols; col++)
        {
            int index = col * rows + row;
            if (index < count)
            {
                const char *color = get_color_code(dir, filenames[index]);
                int name_len = strlen(filenames[index]);

                if (col == cols - 1 || index + rows >= count)
                {
                    printf("%s%s%s", color, filenames[index], COLOR_RESET);
                }
                else
                {
                    int padding = col_width - name_len;
                    printf("%s%s%s%*s", color, filenames[index], COLOR_RESET, padding, "");
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

    const char *color = get_color_code(path, filename);

    printf("%s %2ld %-8s %-8s %8lld %s %s%s%s\n",
           mode_str,
           (long)info.st_nlink,
           user_name,
           group_name,
           (long long)info.st_size,
           time_buf,
           color,
           filename,
           COLOR_RESET);
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
