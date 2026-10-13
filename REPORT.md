
## Feature 2 Report Questions

### 1. Difference between stat() and lstat()
The `stat()` system call follows symbolic links and returns metadata about the target file that the link points to. In contrast, `lstat()` does NOT follow symbolic links; it returns metadata about the symbolic link file itself. For `ls`, `lstat()` is more appropriate because `ls` needs to display information about the link file itself (such as link size and link permissions) rather than the target file it points to.

### 2. Extracting file type and permissions using bitwise operators
The `st_mode` field in `struct stat` is a bitmask containing file type and permission flags. 
- Bitwise AND (`&`) with bitwise masks/macros is used to inspect specific bits.
- File types are extracted using macros like `S_ISDIR(mode)` or `(mode & S_IFMT) == S_IFDIR`.
- Permissions are checked using predefined bitwise masks, for example: `mode & S_IRUSR` checks if the owner has read permissions, and `mode & S_IXGRP` checks group execution permissions.

## Feature 3 Report Questions

### 1. General logic for printing items in a "down then across" columnar format
In "down then across" order, items are filled vertically column by column. A single sequential loop fails because standard terminal output prints line by line (horizontally row by row). Therefore, to print row `r`, we must calculate and jump across the array to access elements from each column for that row using `index = col * total_rows + row`, padding each item with spaces up to the column width.

### 2. Purpose of ioctl system call and limitations of fixed-width fallback
The `ioctl()` system call with `TIOCGWINSZ` queries the terminal driver to obtain the current terminal window dimensions (`ws_col`). Without it, relying solely on an 80-column fallback would cause output wrapping or ugly truncation when terminal windows are widened or narrowed by the user.

## Feature 4 Report Questions

### 1. Comparison of vertical vs. horizontal display complexity
The vertical ("down then across") layout requires significantly more pre-calculation. In vertical layout, items are indexed column-by-column, requiring explicit calculation of `total_rows` and dynamic offset indexing (`col * total_rows + row`) to print elements across columns line by line. In contrast, horizontal ("across") logic prints items strictly in sequence; it simply prints array elements from index `0` to `N-1` with padding, emitting a newline whenever `(i + 1) % cols == 0`.

### 2. Strategy used to manage different display modes
An `enum DisplayMode` (`MODE_VERTICAL`, `MODE_HORIZONTAL`, `MODE_LONG`) was declared to manage display states. The `getopt()` loop parses command-line arguments and sets the appropriate enum value based on `-l` or `-x`. Inside `do_ls()`, after reading all directory entries into an array, an `if/else` control structure checks the enum state and calls the matching function (`print_long_format`, `print_horizontal_display`, or `print_column_display`).

## Feature 5 Report Questions

### 1. Necessity of reading all entries into memory before sorting and drawbacks
Sorting algorithms require random or sequential access to the full set of items to compare and order them. Because `readdir()` retrieves directory entries incrementally one entry at a time from disk in filesystem order (unsorted), all filenames must first be read and stored in memory before sorting can take place.
**Drawback:** For extremely large directories containing millions of files, buffering all names in memory incurs significant dynamic memory (`RAM`) allocation overhead and noticeable latency delay before displaying any output.

### 2. Purpose and signature of comparison function for qsort
The function signature expected by `qsort()` is: `int compare(const void *a, const void *b);`.
It accepts generic `const void *` pointers so `qsort()` can work with any data type. In our case, `a` and `b` point to elements in an array of `char *` pointers (i.e., `a` and `b` are `char **`). The function casts the arguments to `const char **`, dereferences them to obtain string pointers, and uses `strcmp()` to return a negative integer if `a < b`, zero if `a == b`, or a positive integer if `a > b`.

## Feature 6 Report Questions

### 1. How ANSI escape sequences function in terminal output
ANSI escape sequences are special character sequences beginning with an Escape character (`\033` or `\e`) followed by `[` and formatting codes ending in `m`. When printed to standard output, the terminal emulator intercepts these control sequences to alter text presentation attributes (such as foreground color, background color, or bold styling) rather than rendering the raw characters on screen. Resetting with `\033[0m` restores standard terminal attributes.

### Execution Permission Bits in `st_mode`

To determine if a file is executable, check the three execute permission bit flags in `st_mode` using bitwise AND (`&`) operations:

* **Owner (User):** `S_IXUSR`
* **Group:** `S_IXGRP`
* **Others:** `S_IXOTH`

## Feature 7 Report Questions: Recursive Listing (-R)

### 1. What is a "base case" in a recursive function, and what is the base case in `ls -R`?
A **base case** is the terminating condition in a recursive function that prevents it from making further recursive calls, avoiding infinite loops and stack overflow errors.

In `ls -R`, the base case occurs when `do_ls()` encounters a directory that contains **no further subdirectories** (or a directory where all entries are regular files, devices, or symbolic links). Additionally, ignoring the special relative directory entries (`.` and `..`) acts as a crucial guard condition to prevent infinite self-referential recursion.

### 2. Why is constructing a full path ("parent_dir/subdir") essential before making a recursive call?
System calls like `opendir()` and `lstat()` operate relative to the process's current working directory (CWD) unless provided with an absolute or fully qualified relative path.

If `do_ls("parent_dir")` simply called `do_ls("subdir")` without constructing `"parent_dir/subdir"`, `opendir()` would search for `"subdir"` directly inside the process's working directory rather than inside `"parent_dir"`. This would fail with `ENOENT` ("No such file or directory") or mistakenly read a completely different directory of the same name at the root execution level. Constructing the full path ensures accurate file identification across arbitrary tree depths.


## Feature 7 Report Questions: Recursive Listing (-R)

### Question 1: In a recursive function, what is a "base case"? In the context of your recursive `ls`, what is the base case that stops the recursion from continuing forever?

**Answer:**
A **base case** is the terminating condition in a recursive function that stops it from making further calls to itself, preventing infinite recursion and stack overflow errors.

In the context of the custom recursive `ls` implementation (`do_ls`), the base case occurs when:
1. **No Subdirectories Remain:** `do_ls` processes a directory that contains no subdirectories (i.e., it contains only regular files, devices, symbolic links, or is empty). When the loop checking for `S_ISDIR` finds zero subdirectories, no further recursive calls are made, and the function returns up the call stack.
2. **Ignoring Relative Entries (`.` and `..`):** Explicitly ignoring `.` (current directory) and `..` (parent directory) acts as a critical guard condition. If these were processed as normal subdirectories, `do_ls(".")` would call `do_ls("./.")` infinitely, leading to immediate stack exhaustion.

---

### Question 2: Explain why it is essential to construct a full path (e.g., "parent_dir/subdir") before making a recursive call. What would happen if you simply called `do_ls("subdir")` from within the `do_ls("parent_dir")` function call?

**Answer:**
Constructing the full path (e.g., `"parent_dir/subdir"`) is essential because POSIX system calls like `opendir()` and `lstat()` resolve relative file paths based on the **Process Current Working Directory (CWD)**, not relative to the directory currently being processed inside `do_ls()`.

If you were to simply call `do_ls("subdir")` from inside `do_ls("parent_dir")`:
1. `opendir("subdir")` would look for `subdir` in the current working directory from where the program was launched, rather than inside `parent_dir/`.
2. **Path Resolution Failure (`ENOENT`):** Unless a folder named `subdir` happens to exist in the root execution directory, `opendir()` will fail and return `NULL` with "No such file or directory".
3. **Incorrect Traversal:** If a directory named `subdir` happens to exist in the execution folder, `do_ls` would read the wrong directory entirely, resulting in corrupted listing output and broken file tree navigation.

Combining the parent directory string with the child entry name (using `snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry)`) guarantees that `opendir()` and `lstat()` receive a valid path relative to the process execution point, enabling proper traversal across arbitrary folder depths.

## Feature 7 Report Questions: Recursive Listing (-R)

### Question 1: In a recursive function, what is a "base case"? In the context of your recursive `ls`, what is the base case that stops the recursion from continuing forever?

**Answer:**
A **base case** is the terminating condition in a recursive function that stops it from making further calls to itself, preventing infinite recursion and stack overflow errors.

In the context of the custom recursive `ls` implementation (`do_ls`), the base case occurs when:
1. **No Subdirectories Remain:** `do_ls` processes a directory that contains no subdirectories (i.e., it contains only regular files, devices, symbolic links, or is empty). When the loop checking for `S_ISDIR` finds zero subdirectories, no further recursive calls are made, and the function returns up the call stack.
2. **Ignoring Relative Entries (`.` and `..`):** Explicitly ignoring `.` (current directory) and `..` (parent directory) acts as a critical guard condition. If these were processed as normal subdirectories, `do_ls(".")` would call `do_ls("./.")` infinitely, leading to immediate stack exhaustion.

---

### Question 2: Explain why it is essential to construct a full path (e.g., "parent_dir/subdir") before making a recursive call. What would happen if you simply called `do_ls("subdir")` from within the `do_ls("parent_dir")` function call?

**Answer:**
Constructing the full path (e.g., `"parent_dir/subdir"`) is essential because POSIX system calls like `opendir()` and `lstat()` resolve relative file paths based on the **Process Current Working Directory (CWD)**, not relative to the directory currently being processed inside `do_ls()`.

If you were to simply call `do_ls("subdir")` from inside `do_ls("parent_dir")`:
1. `opendir("subdir")` would look for `subdir` in the current working directory from where the program was launched, rather than inside `parent_dir/`.
2. **Path Resolution Failure (`ENOENT`):** Unless a folder named `subdir` happens to exist in the root execution directory, `opendir()` will fail and return `NULL` with "No such file or directory".
3. **Incorrect Traversal:** If a directory named `subdir` happens to exist in the execution folder, `do_ls` would read the wrong directory entirely, resulting in corrupted listing output and broken file tree navigation.

Combining the parent directory string with the child entry name (using `snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry)`) guarantees that `opendir()` and `lstat()` receive a valid path relative to the process execution point, enabling proper traversal across arbitrary folder depths.
